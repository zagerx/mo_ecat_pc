#pragma once

#include <QMainWindow>

#include "core_bridge/ecat_types.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTextEdit;

namespace mo_ecat_pc
{

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:
    void initializeRequested(UiMasterConfig config);
    void scanRequested();
    void enterMaintenanceRequested();
    void prepareRunRequested();
    void startOperationRequested();
    void backToMaintenanceRequested();
    void stopRequested();

public slots:
    void OnMasterStateChanged(UiRuntimeState state);
    void OnSlaveListUpdated(QVector<UiSlaveInfo> slaves);
    void OnLogReceived(UiLogRecord record);
    void OnOperationFailed(QString command, QString reason);

private:
    UiMasterConfig ReadConfig() const;
    void AppendLog(const QString &level, const QString &message);

    QLineEdit *interface_edit_ = nullptr;
    QLabel *state_value_ = nullptr;
    QTableWidget *slave_table_ = nullptr;
    QTextEdit *log_view_ = nullptr;

    QPushButton *initialize_button_ = nullptr;
    QPushButton *scan_button_ = nullptr;
    QPushButton *maintenance_button_ = nullptr;
    QPushButton *prepare_button_ = nullptr;
    QPushButton *start_button_ = nullptr;
    QPushButton *back_button_ = nullptr;
    QPushButton *stop_button_ = nullptr;
};

} // namespace mo_ecat_pc
