#include "core_bridge/ecat_worker.h"

#include <fstream>
#include <sstream>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#endif

#include "mo_ecat/config.h"
#include "mo_ecat/master.h"
#include "mo_ecat/types.h"

namespace mo_ecat_pc
{

namespace
{

UiLogRecord MakeLogRecord(const QString &level, const QString &source,
                          const QString &message)
{
    UiLogRecord record;
    record.level = level;
    record.source = source;
    record.message = message;
    record.timestamp = QDateTime::currentDateTime();
    return record;
}

bool HasRawSocketPermission()
{
#ifdef __linux__
    if (geteuid() == 0) {
        return true;
    }

    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("CapEff:", 0) != 0) {
            continue;
        }

        std::istringstream iss(line.substr(7));
        unsigned long long caps = 0;
        iss >> std::hex >> caps;
        constexpr unsigned long long kCapNetRaw = 1ULL << 13;
        return (caps & kCapNetRaw) != 0;
    }
#endif
    return false;
}

} // namespace

QString ToDisplayString(const UiRuntimeState &state)
{
    mo_ecat::MasterRuntimeState core_state;

    switch (state.mode) {
    case UiMasterMode::kInit:
        core_state.mode = mo_ecat::MasterMode::kInit;
        break;
    case UiMasterMode::kPrepare:
        core_state.mode = mo_ecat::MasterMode::kPrepare;
        break;
    case UiMasterMode::kRun:
        core_state.mode = mo_ecat::MasterMode::kRun;
        break;
    case UiMasterMode::kFault:
        core_state.mode = mo_ecat::MasterMode::kFault;
        break;
    case UiMasterMode::kEmergencyStop:
        core_state.mode = mo_ecat::MasterMode::kEmergencyStop;
        break;
    case UiMasterMode::kUnknown:
        return QStringLiteral("未知状态");
    }

    switch (state.transition) {
    case UiTransitionStage::kEntering:
        core_state.transition = mo_ecat::TransitionStage::kEntering;
        break;
    case UiTransitionStage::kStable:
        core_state.transition = mo_ecat::TransitionStage::kStable;
        break;
    case UiTransitionStage::kExiting:
        core_state.transition = mo_ecat::TransitionStage::kExiting;
        break;
    case UiTransitionStage::kUnknown:
        return QStringLiteral("未知状态");
    }

    switch (state.prepare_stage) {
    case UiPrepareStage::kNone:
        core_state.prepare_stage = mo_ecat::PrepareStage::kNone;
        break;
    case UiPrepareStage::kAdapterReady:
        core_state.prepare_stage = mo_ecat::PrepareStage::kAdapterReady;
        break;
    case UiPrepareStage::kTopologyDiscovered:
        core_state.prepare_stage = mo_ecat::PrepareStage::kTopologyDiscovered;
        break;
    case UiPrepareStage::kPreOpMaintenance:
        core_state.prepare_stage = mo_ecat::PrepareStage::kPreOpMaintenance;
        break;
    case UiPrepareStage::kSafeOpReady:
        core_state.prepare_stage = mo_ecat::PrepareStage::kSafeOpReady;
        break;
    case UiPrepareStage::kUnknown:
        return QStringLiteral("未知状态");
    }

    return QString::fromStdString(mo_ecat::ToDisplayString(core_state));
}

EcatWorker::EcatWorker(QObject *parent)
    : QObject(parent)
{
}

EcatWorker::~EcatWorker()
{
    if (service_timer_ != nullptr) {
        service_timer_->stop();
    }
    if (master_ != nullptr) {
        master_->Shutdown();
    }
}

void EcatWorker::StartServiceLoop()
{
    EnsureMaster();

    if (service_timer_ == nullptr) {
        service_timer_ = new QTimer(this);
        connect(service_timer_, &QTimer::timeout,
                this, &EcatWorker::ServiceOnce);
    }

    service_timer_->start(1);
    EmitRuntimeState();
}

void EcatWorker::EnterPrepare(UiMasterConfig config)
{
    EnsureMaster();

    if (!HasRawSocketPermission()) {
        emit LogReceived(MakeLogRecord(
            QStringLiteral("warn"),
            QStringLiteral("core_bridge"),
            QStringLiteral("Raw socket access usually requires sudo or CAP_NET_RAW.")));
    }

    mo_ecat::EcMasterConfig core_config;
    core_config.ifname = config.interface_name.toStdString();
    core_config.cycle_time_us = config.cycle_time_us;
    core_config.use_dc = config.use_dc;
    core_config.log_sink = mo_ecat::LogSinkMode::kCallback;

    const bool ok = master_->EnterPrepare(core_config);
    ReportCommandResult(QStringLiteral("EnterPrepare"), ok);
    RefreshSnapshot();
}

void EcatWorker::DiscoverTopology()
{
    EnsureMaster();
    const bool ok = master_->DiscoverTopology();
    ReportCommandResult(QStringLiteral("DiscoverTopology"), ok);
    RefreshSnapshot();
}

void EcatWorker::EnterPreOpMaintenance()
{
    EnsureMaster();
    const bool ok = master_->EnterPreOpMaintenance();
    ReportCommandResult(QStringLiteral("EnterPreOpMaintenance"), ok);
    RefreshSnapshot();
}

void EcatWorker::EnterSafeOpReady()
{
    EnsureMaster();
    const bool ok = master_->EnterSafeOpReady();
    ReportCommandResult(QStringLiteral("EnterSafeOpReady"), ok);
    RefreshSnapshot();
}

void EcatWorker::EnterRun()
{
    EnsureMaster();
    const bool ok = master_->EnterRun();
    ReportCommandResult(QStringLiteral("EnterRun"), ok);
    RefreshSnapshot();
}

void EcatWorker::BackToMaintenance()
{
    EnsureMaster();
    const bool ok = master_->BackToMaintenance();
    ReportCommandResult(QStringLiteral("BackToMaintenance"), ok);
    RefreshSnapshot();
}

void EcatWorker::Shutdown()
{
    if (master_ == nullptr) {
        return;
    }

    master_->Shutdown();
    RefreshSnapshot();
}

void EcatWorker::ServiceOnce()
{
    if (master_ == nullptr) {
        return;
    }

    master_->Service();
}

void EcatWorker::EnsureMaster()
{
    if (master_ != nullptr) {
        return;
    }

    master_ = std::make_unique<mo_ecat::MoEcatMaster>();
    master_->on_runtime_state_changed = [this](mo_ecat::MasterRuntimeState state) {
        emit MasterStateChanged(ConvertRuntimeState(state));
    };
    master_->on_log_message = [this](const std::string &level,
                                     const std::string &source,
                                     const std::string &message) {
        emit LogReceived(MakeLogRecord(QString::fromStdString(level),
                                       QString::fromStdString(source),
                                       QString::fromStdString(message)));
    };
}

void EcatWorker::RefreshSnapshot()
{
    if (master_ == nullptr) {
        return;
    }

    EmitRuntimeState();

    QVector<UiSlaveInfo> slaves;
    const auto core_slaves = master_->GetSlaveInfos();
    slaves.reserve(static_cast<int>(core_slaves.size()));
    for (const auto &slave : core_slaves) {
        slaves.push_back(ConvertSlaveInfo(slave));
    }
    emit SlaveListUpdated(slaves);
}

void EcatWorker::ReportCommandResult(const QString &command, bool ok)
{
    if (ok) {
        emit LogReceived(MakeLogRecord(QStringLiteral("info"),
                                       QStringLiteral("core_bridge"),
                                       command + QStringLiteral(" succeeded")));
        return;
    }

    QString reason = command + QStringLiteral(" failed");
    if (command == QStringLiteral("EnterPrepare") && !HasRawSocketPermission()) {
        reason += QStringLiteral(
            ". EtherCAT raw socket access requires sudo or CAP_NET_RAW.");
    }
    emit OperationFailed(command, reason);
}

void EcatWorker::EmitRuntimeState()
{
    if (master_ == nullptr) {
        return;
    }
    emit MasterStateChanged(ConvertRuntimeState(master_->GetRuntimeState()));
}

UiRuntimeState EcatWorker::ConvertRuntimeState(const mo_ecat::MasterRuntimeState &state)
{
    UiRuntimeState ui_state;

    switch (state.mode) {
    case mo_ecat::MasterMode::kInit:
        ui_state.mode = UiMasterMode::kInit;
        break;
    case mo_ecat::MasterMode::kPrepare:
        ui_state.mode = UiMasterMode::kPrepare;
        break;
    case mo_ecat::MasterMode::kRun:
        ui_state.mode = UiMasterMode::kRun;
        break;
    case mo_ecat::MasterMode::kFault:
        ui_state.mode = UiMasterMode::kFault;
        break;
    case mo_ecat::MasterMode::kEmergencyStop:
        ui_state.mode = UiMasterMode::kEmergencyStop;
        break;
    }

    switch (state.transition) {
    case mo_ecat::TransitionStage::kEntering:
        ui_state.transition = UiTransitionStage::kEntering;
        break;
    case mo_ecat::TransitionStage::kStable:
        ui_state.transition = UiTransitionStage::kStable;
        break;
    case mo_ecat::TransitionStage::kExiting:
        ui_state.transition = UiTransitionStage::kExiting;
        break;
    }

    switch (state.prepare_stage) {
    case mo_ecat::PrepareStage::kNone:
        ui_state.prepare_stage = UiPrepareStage::kNone;
        break;
    case mo_ecat::PrepareStage::kAdapterReady:
        ui_state.prepare_stage = UiPrepareStage::kAdapterReady;
        break;
    case mo_ecat::PrepareStage::kTopologyDiscovered:
        ui_state.prepare_stage = UiPrepareStage::kTopologyDiscovered;
        break;
    case mo_ecat::PrepareStage::kPreOpMaintenance:
        ui_state.prepare_stage = UiPrepareStage::kPreOpMaintenance;
        break;
    case mo_ecat::PrepareStage::kSafeOpReady:
        ui_state.prepare_stage = UiPrepareStage::kSafeOpReady;
        break;
    }

    return ui_state;
}

UiSlaveInfo EcatWorker::ConvertSlaveInfo(const mo_ecat::SlaveInfo &info)
{
    UiSlaveInfo ui_info;
    ui_info.slave_id = info.slave_id;
    ui_info.vendor_id = info.vendor_id;
    ui_info.product_id = info.product_id;
    ui_info.revision_id = info.revision_id;
    ui_info.serial_id = info.serial_id;
    ui_info.name = QString::fromStdString(info.name);
    ui_info.supports_dc = info.supports_dc;
    ui_info.output_bytes = info.output_bytes;
    ui_info.input_bytes = info.input_bytes;
    return ui_info;
}

} // namespace mo_ecat_pc
