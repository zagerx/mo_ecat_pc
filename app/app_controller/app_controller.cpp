#include "app_controller/app_controller.h"

namespace mo_ecat_pc
{

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    connect(&main_window_, &MainWindow::initializeRequested,
            &bridge_, &EcatQtBridge::InitializeAdapter);
    connect(&main_window_, &MainWindow::scanRequested,
            &bridge_, &EcatQtBridge::Scan);
    connect(&main_window_, &MainWindow::enterMaintenanceRequested,
            &bridge_, &EcatQtBridge::EnterMaintenance);
    connect(&main_window_, &MainWindow::prepareRunRequested,
            &bridge_, &EcatQtBridge::PrepareRun);
    connect(&main_window_, &MainWindow::startOperationRequested,
            &bridge_, &EcatQtBridge::StartOperation);
    connect(&main_window_, &MainWindow::backToMaintenanceRequested,
            &bridge_, &EcatQtBridge::BackToMaintenance);
    connect(&main_window_, &MainWindow::stopRequested,
            &bridge_, &EcatQtBridge::Stop);

    connect(&bridge_, &EcatQtBridge::MasterStateChanged,
            &main_window_, &MainWindow::OnMasterStateChanged);
    connect(&bridge_, &EcatQtBridge::SlaveListUpdated,
            &main_window_, &MainWindow::OnSlaveListUpdated);
    connect(&bridge_, &EcatQtBridge::LogReceived,
            &main_window_, &MainWindow::OnLogReceived);
    connect(&bridge_, &EcatQtBridge::OperationFailed,
            &main_window_, &MainWindow::OnOperationFailed);
}

void AppController::Show()
{
    main_window_.show();
}

} // namespace mo_ecat_pc

