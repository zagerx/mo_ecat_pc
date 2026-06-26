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

    connect(this, &EcatQtBridge::InitializeAdapterRequested,
            worker_, &EcatWorker::InitializeAdapter);
    connect(this, &EcatQtBridge::ScanRequested,
            worker_, &EcatWorker::Scan);
    connect(this, &EcatQtBridge::EnterMaintenanceRequested,
            worker_, &EcatWorker::EnterMaintenance);
    connect(this, &EcatQtBridge::PrepareRunRequested,
            worker_, &EcatWorker::PrepareRun);
    connect(this, &EcatQtBridge::StartOperationRequested,
            worker_, &EcatWorker::StartOperation);
    connect(this, &EcatQtBridge::BackToMaintenanceRequested,
            worker_, &EcatWorker::BackToMaintenance);
    connect(this, &EcatQtBridge::StopRequested,
            worker_, &EcatWorker::StopMaster);

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
    emit StopRequested();
    worker_thread_.quit();
    worker_thread_.wait();
}

void EcatQtBridge::InitializeAdapter(const UiMasterConfig &config)
{
    emit InitializeAdapterRequested(config);
}

void EcatQtBridge::Scan()
{
    emit ScanRequested();
}

void EcatQtBridge::EnterMaintenance()
{
    emit EnterMaintenanceRequested();
}

void EcatQtBridge::PrepareRun()
{
    emit PrepareRunRequested();
}

void EcatQtBridge::StartOperation()
{
    emit StartOperationRequested();
}

void EcatQtBridge::BackToMaintenance()
{
    emit BackToMaintenanceRequested();
}

void EcatQtBridge::Stop()
{
    emit StopRequested();
}

} // namespace mo_ecat_pc
