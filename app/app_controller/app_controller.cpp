#include "app_controller/app_controller.h"

namespace mo_ecat_pc
{

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    connect(&main_window_, &MainWindow::enterPrepareRequested,
            &bridge_, &EcatQtBridge::EnterPrepare);
    connect(&main_window_, &MainWindow::discoverTopologyRequested,
            &bridge_, &EcatQtBridge::DiscoverTopology);
    connect(&main_window_, &MainWindow::enterPreOpMaintenanceRequested,
            &bridge_, &EcatQtBridge::EnterPreOpMaintenance);
    connect(&main_window_, &MainWindow::enterSafeOpReadyRequested,
            &bridge_, &EcatQtBridge::EnterSafeOpReady);
    connect(&main_window_, &MainWindow::enterRunRequested,
            &bridge_, &EcatQtBridge::EnterRun);
    connect(&main_window_, &MainWindow::backToMaintenanceRequested,
            &bridge_, &EcatQtBridge::BackToMaintenance);
    connect(&main_window_, &MainWindow::shutdownRequested,
            &bridge_, &EcatQtBridge::Shutdown);

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

