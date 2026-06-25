#pragma once

#include <memory>

#include <QObject>
#include <QTimer>

#include "core_bridge/ecat_types.h"

namespace mo_ecat
{
class MoEcatMaster;
enum class MasterState;
struct SlaveInfo;
} // namespace mo_ecat

namespace mo_ecat_pc
{

class EcatWorker : public QObject {
    Q_OBJECT

public:
    explicit EcatWorker(QObject *parent = nullptr);
    ~EcatWorker() override;

public slots:
    void StartServiceLoop();
    void InitializeAdapter(UiMasterConfig config);
    void Scan();
    void EnterMaintenance();
    void PrepareRun();
    void StartOperation();
    void BackToMaintenance();
    void StopMaster();

signals:
    void MasterStateChanged(UiMasterState state);
    void SlaveListUpdated(QVector<UiSlaveInfo> slaves);
    void LogReceived(UiLogRecord record);
    void OperationFailed(QString command, QString reason);

private slots:
    void ServiceOnce();

private:
    void EnsureMaster();
    void RefreshSnapshot();
    void ReportCommandResult(const QString &command, bool ok);

    static UiMasterState ConvertState(mo_ecat::MasterState state);
    static UiSlaveInfo ConvertSlaveInfo(const mo_ecat::SlaveInfo &info);

    std::unique_ptr<mo_ecat::MoEcatMaster> master_;
    QTimer *service_timer_ = nullptr;
};

} // namespace mo_ecat_pc

