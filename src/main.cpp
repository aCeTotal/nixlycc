#include <QApplication>
#include <QCommandLineParser>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("nixlycc");
    app.setApplicationDisplayName("NixlyCC");
    app.setApplicationVersion("0.2");
    app.setDesktopFileName("nixlycc");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption pageOption("page", "Open directly on the given page (e.g. monitors).", "name");
    parser.addOption(pageOption);
    parser.process(app);

    MainWindow window(parser.value(pageOption));
    window.show();

    return app.exec();
}
