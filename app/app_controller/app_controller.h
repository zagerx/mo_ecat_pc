#pragma once

#include <QObject>

#include "core_bridge/ecat_qt_bridge.h"
#include "gui/main_window.h"

namespace mo_ecat_pc
{

class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);

    void Show();

private:
    EcatQtBridge bridge_;
    MainWindow main_window_;
};

} // namespace mo_ecat_pc

