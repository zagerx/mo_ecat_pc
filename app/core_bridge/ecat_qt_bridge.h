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
    void InitializeAdapter(const UiMasterConfig &config);
    void Scan();
    void EnterMaintenance();
    void PrepareRun();
    void StartOperation();
    void BackToMaintenance();
    void Stop();

signals:
    void MasterStateChanged(UiRuntimeState state);
    void SlaveListUpdated(QVector<UiSlaveInfo> slaves);
    void LogReceived(UiLogRecord record);
    void OperationFailed(QString command, QString reason);

    void InitializeAdapterRequested(UiMasterConfig config);
    void ScanRequested();
    void EnterMaintenanceRequested();
    void PrepareRunRequested();
    void StartOperationRequested();
    void BackToMaintenanceRequested();
    void StopRequested();

private:
    QThread worker_thread_;
    EcatWorker *worker_ = nullptr;
};

} // namespace mo_ecat_pc
