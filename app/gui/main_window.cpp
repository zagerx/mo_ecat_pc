#include "gui/main_window.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace mo_ecat_pc
{

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("mo_ecat_pc"));
    resize(980, 640);

    auto *central = new QWidget(this);
    auto *root_layout = new QVBoxLayout(central);

    auto *config_box = new QGroupBox(QStringLiteral("Master"), central);
    auto *config_layout = new QFormLayout(config_box);
    interface_edit_ = new QLineEdit(QStringLiteral("enp0s31f6"), config_box);
    state_value_ = new QLabel(ToDisplayString(UiRuntimeState{}), config_box);
    config_layout->addRow(QStringLiteral("Interface"), interface_edit_);
    config_layout->addRow(QStringLiteral("State"), state_value_);

    auto *command_box = new QGroupBox(QStringLiteral("Lifecycle"), central);
    auto *command_layout = new QGridLayout(command_box);
    initialize_button_ = new QPushButton(QStringLiteral("Initialize"), command_box);
    scan_button_ = new QPushButton(QStringLiteral("Scan"), command_box);
    maintenance_button_ = new QPushButton(QStringLiteral("Maintenance"), command_box);
    prepare_button_ = new QPushButton(QStringLiteral("Prepare"), command_box);
    start_button_ = new QPushButton(QStringLiteral("Start"), command_box);
    back_button_ = new QPushButton(QStringLiteral("Back"), command_box);
    stop_button_ = new QPushButton(QStringLiteral("Stop"), command_box);

    command_layout->addWidget(initialize_button_, 0, 0);
    command_layout->addWidget(scan_button_, 0, 1);
    command_layout->addWidget(maintenance_button_, 0, 2);
    command_layout->addWidget(prepare_button_, 0, 3);
    command_layout->addWidget(start_button_, 1, 0);
    command_layout->addWidget(back_button_, 1, 1);
    command_layout->addWidget(stop_button_, 1, 2);

    slave_table_ = new QTableWidget(0, 6, central);
    slave_table_->setHorizontalHeaderLabels({
        QStringLiteral("ID"),
        QStringLiteral("Name"),
        QStringLiteral("Vendor"),
        QStringLiteral("Product"),
        QStringLiteral("Out"),
        QStringLiteral("In"),
    });
    slave_table_->horizontalHeader()->setStretchLastSection(true);
    slave_table_->verticalHeader()->setVisible(false);
    slave_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    slave_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    log_view_ = new QTextEdit(central);
    log_view_->setReadOnly(true);
    log_view_->setMinimumHeight(160);

    root_layout->addWidget(config_box);
    root_layout->addWidget(command_box);
    root_layout->addWidget(slave_table_, 1);
    root_layout->addWidget(log_view_);
    setCentralWidget(central);

    connect(initialize_button_, &QPushButton::clicked, this, [this] {
        emit initializeRequested(ReadConfig());
    });
    connect(scan_button_, &QPushButton::clicked, this, &MainWindow::scanRequested);
    connect(maintenance_button_, &QPushButton::clicked,
            this, &MainWindow::enterMaintenanceRequested);
    connect(prepare_button_, &QPushButton::clicked, this, &MainWindow::prepareRunRequested);
    connect(start_button_, &QPushButton::clicked, this, &MainWindow::startOperationRequested);
    connect(back_button_, &QPushButton::clicked, this, &MainWindow::backToMaintenanceRequested);
    connect(stop_button_, &QPushButton::clicked, this, &MainWindow::stopRequested);
}

void MainWindow::OnMasterStateChanged(UiRuntimeState state)
{
    state_value_->setText(ToDisplayString(state));
}

void MainWindow::OnSlaveListUpdated(QVector<UiSlaveInfo> slaves)
{
    slave_table_->setRowCount(slaves.size());

    for (int row = 0; row < slaves.size(); ++row) {
        const auto &slave = slaves[row];
        slave_table_->setItem(row, 0, new QTableWidgetItem(QString::number(slave.slave_id)));
        slave_table_->setItem(row, 1, new QTableWidgetItem(slave.name));
        slave_table_->setItem(row, 2, new QTableWidgetItem(
                                      QStringLiteral("0x%1").arg(slave.vendor_id, 8, 16,
                                                                 QLatin1Char('0'))));
        slave_table_->setItem(row, 3, new QTableWidgetItem(
                                      QStringLiteral("0x%1").arg(slave.product_id, 8, 16,
                                                                 QLatin1Char('0'))));
        slave_table_->setItem(row, 4, new QTableWidgetItem(QString::number(slave.output_bytes)));
        slave_table_->setItem(row, 5, new QTableWidgetItem(QString::number(slave.input_bytes)));
    }
}

void MainWindow::OnLogReceived(UiLogRecord record)
{
    const QString time = record.timestamp.toString(QStringLiteral("HH:mm:ss.zzz"));
    log_view_->append(QStringLiteral("[%1] [%2] [%3] %4")
                          .arg(time, record.level, record.source, record.message));
}

void MainWindow::OnOperationFailed(QString command, QString reason)
{
    AppendLog(QStringLiteral("error"), command + QStringLiteral(": ") + reason);
}

UiMasterConfig MainWindow::ReadConfig() const
{
    UiMasterConfig config;
    config.interface_name = interface_edit_->text().trimmed();
    return config;
}

void MainWindow::AppendLog(const QString &level, const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    log_view_->append(QStringLiteral("[%1] [%2] [ui] %3").arg(time, level, message));
}

} // namespace mo_ecat_pc
