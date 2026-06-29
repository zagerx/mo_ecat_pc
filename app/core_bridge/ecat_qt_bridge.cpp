#include "core_bridge/ecat_qt_bridge.h"

#include "core_bridge/ecat_worker.h"

namespace mo_ecat_pc
{

EcatQtBridge::EcatQtBridge(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<UiRuntimeState>();
    qRegisterMetaType<UiMasterConfig>();
    qRegisterMetaType<UiSlaveInfo>();
    qRegisterMetaType<QVector<UiSlaveInfo>>();
    qRegisterMetaType<UiLogRecord>();
    qRegisterMetaType<SdoReadRequest>();
    qRegisterMetaType<SdoReadResult>();

    worker_ = new EcatWorker();
    worker_->moveToThread(&worker_thread_);

    connect(&worker_thread_, &QThread::started,
            worker_, &EcatWorker::StartServiceLoop);
    connect(&worker_thread_, &QThread::finished,
            worker_, &QObject::deleteLater);

    connect(this, &EcatQtBridge::EnterPrepareRequested,
            worker_, &EcatWorker::EnterPrepare);
    connect(this, &EcatQtBridge::DiscoverTopologyRequested,
            worker_, &EcatWorker::DiscoverTopology);
    connect(this, &EcatQtBridge::EnterPreOpMaintenanceRequested,
            worker_, &EcatWorker::EnterPreOpMaintenance);
    connect(this, &EcatQtBridge::EnterSafeOpReadyRequested,
            worker_, &EcatWorker::EnterSafeOpReady);
    connect(this, &EcatQtBridge::EnterRunRequested,
            worker_, &EcatWorker::EnterRun);
    connect(this, &EcatQtBridge::BackToMaintenanceRequested,
            worker_, &EcatWorker::BackToMaintenance);
    connect(this, &EcatQtBridge::ShutdownRequested,
            worker_, &EcatWorker::Shutdown);

    connect(worker_, &EcatWorker::MasterStateChanged,
            this, &EcatQtBridge::MasterStateChanged);
    connect(worker_, &EcatWorker::SlaveListUpdated,
            this, &EcatQtBridge::SlaveListUpdated);
    connect(worker_, &EcatWorker::LogReceived,
            this, &EcatQtBridge::LogReceived);
    connect(worker_, &EcatWorker::OperationFailed,
            this, &EcatQtBridge::OperationFailed);

    worker_thread_.start();
}

EcatQtBridge::~EcatQtBridge()
{
    emit ShutdownRequested();
    worker_thread_.quit();
    worker_thread_.wait();
}

void EcatQtBridge::EnterPrepare(const UiMasterConfig &config)
{
    emit EnterPrepareRequested(config);
}

void EcatQtBridge::DiscoverTopology()
{
    emit DiscoverTopologyRequested();
}

void EcatQtBridge::EnterPreOpMaintenance()
{
    emit EnterPreOpMaintenanceRequested();
}

void EcatQtBridge::EnterSafeOpReady()
{
    emit EnterSafeOpReadyRequested();
}

void EcatQtBridge::EnterRun()
{
    emit EnterRunRequested();
}

void EcatQtBridge::BackToMaintenance()
{
    emit BackToMaintenanceRequested();
}

void EcatQtBridge::Shutdown()
{
    emit ShutdownRequested();
}

} // namespace mo_ecat_pc
