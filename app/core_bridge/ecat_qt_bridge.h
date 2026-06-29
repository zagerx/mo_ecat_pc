#pragma once

#include <QObject>
#include <QThread>

#include "core_bridge/ecat_types.h"

namespace mo_ecat_pc
{

class EcatWorker;

class EcatQtBridge : public QObject {
    Q_OBJECT

public:
    explicit EcatQtBridge(QObject *parent = nullptr);
    ~EcatQtBridge() override;

public slots:
    void EnterPrepare(const UiMasterConfig &config);
    void DiscoverTopology();
    void EnterPreOpMaintenance();
    void EnterSafeOpReady();
    void EnterRun();
    void BackToMaintenance();
    void Shutdown();

signals:
    void MasterStateChanged(UiRuntimeState state);
    void SlaveListUpdated(QVector<UiSlaveInfo> slaves);
    void LogReceived(UiLogRecord record);
    void OperationFailed(QString command, QString reason);

    void EnterPrepareRequested(UiMasterConfig config);
    void DiscoverTopologyRequested();
    void EnterPreOpMaintenanceRequested();
    void EnterSafeOpReadyRequested();
    void EnterRunRequested();
    void BackToMaintenanceRequested();
    void ShutdownRequested();

private:
    QThread worker_thread_;
    EcatWorker *worker_ = nullptr;
};

} // namespace mo_ecat_pc
