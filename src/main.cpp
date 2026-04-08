#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("nixlycc");
    app.setApplicationDisplayName("NixlyCC");
    app.setApplicationVersion("0.2");
    app.setDesktopFileName("nixlycc");

    MainWindow window;
    window.show();

    return app.exec();
}
