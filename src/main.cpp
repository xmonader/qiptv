#include "app/mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("kiptv"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("xmonader"));
    QApplication::setOrganizationDomain(QStringLiteral("github.io.xmonader"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.xmonader.kiptv"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Native Qt IPTV client for Xtream Codes providers"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    auto *window = new kiptv::MainWindow();
    window->show();

    return app.exec();
}
