#include <QApplication>

#include "app_controller/app_controller.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    mo_ecat_pc::AppController controller;
    controller.Show();

    return app.exec();
}

