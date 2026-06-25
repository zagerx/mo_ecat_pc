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

QString ToDisplayString(UiMasterState state)
{
    switch (state) {
    case UiMasterState::kUninitialized:
        return QStringLiteral("Uninitialized");
    case UiMasterState::kAdapterReady:
        return QStringLiteral("AdapterReady");
    case UiMasterState::kScanned:
        return QStringLiteral("Scanned");
    case UiMasterState::kMaintenance:
        return QStringLiteral("Maintenance");
    case UiMasterState::kReadyToRun:
        return QStringLiteral("ReadyToRun");
    case UiMasterState::kOperational:
        return QStringLiteral("Operational");
    case UiMasterState::kFault:
        return QStringLiteral("Fault");
    case UiMasterState::kEmergencyStop:
        return QStringLiteral("EmergencyStop");
    case UiMasterState::kUnknown:
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
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
        master_->Stop();
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
    emit MasterStateChanged(ConvertState(master_->GetState()));
}

void EcatWorker::InitializeAdapter(UiMasterConfig config)
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

    const bool ok = master_->InitializeAdapter(core_config);
    ReportCommandResult(QStringLiteral("InitializeAdapter"), ok);
    RefreshSnapshot();
}

void EcatWorker::Scan()
{
    EnsureMaster();
    const bool ok = master_->Scan();
    ReportCommandResult(QStringLiteral("Scan"), ok);
    RefreshSnapshot();
}

void EcatWorker::EnterMaintenance()
{
    EnsureMaster();
    const bool ok = master_->EnterMaintenance();
    ReportCommandResult(QStringLiteral("EnterMaintenance"), ok);
    RefreshSnapshot();
}

void EcatWorker::PrepareRun()
{
    EnsureMaster();
    const bool ok = master_->PrepareRun();
    ReportCommandResult(QStringLiteral("PrepareRun"), ok);
    RefreshSnapshot();
}

void EcatWorker::StartOperation()
{
    EnsureMaster();
    const bool ok = master_->StartOperation();
    ReportCommandResult(QStringLiteral("StartOperation"), ok);
    RefreshSnapshot();
}

void EcatWorker::BackToMaintenance()
{
    EnsureMaster();
    const bool ok = master_->BackToMaintenance();
    ReportCommandResult(QStringLiteral("BackToMaintenance"), ok);
    RefreshSnapshot();
}

void EcatWorker::StopMaster()
{
    if (master_ == nullptr) {
        return;
    }

    master_->Stop();
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
    master_->on_state_changed = [this](mo_ecat::MasterState,
                                       mo_ecat::MasterState new_state) {
        emit MasterStateChanged(ConvertState(new_state));
        RefreshSnapshot();
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

    emit MasterStateChanged(ConvertState(master_->GetState()));

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
    if (command == QStringLiteral("InitializeAdapter") && !HasRawSocketPermission()) {
        reason += QStringLiteral(
            ". EtherCAT raw socket access requires sudo or CAP_NET_RAW.");
    }
    emit OperationFailed(command, reason);
}

UiMasterState EcatWorker::ConvertState(mo_ecat::MasterState state)
{
    switch (state) {
    case mo_ecat::MasterState::kUninitialized:
        return UiMasterState::kUninitialized;
    case mo_ecat::MasterState::kAdapterReady:
        return UiMasterState::kAdapterReady;
    case mo_ecat::MasterState::kScanned:
        return UiMasterState::kScanned;
    case mo_ecat::MasterState::kMaintenance:
        return UiMasterState::kMaintenance;
    case mo_ecat::MasterState::kReadyToRun:
        return UiMasterState::kReadyToRun;
    case mo_ecat::MasterState::kOperational:
        return UiMasterState::kOperational;
    case mo_ecat::MasterState::kFault:
        return UiMasterState::kFault;
    case mo_ecat::MasterState::kEmergencyStop:
        return UiMasterState::kEmergencyStop;
    }
    return UiMasterState::kUnknown;
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
