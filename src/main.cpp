#include "app/mainwindow.h"
#include "core/singleinstance.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("qiptv"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("xmonader"));
    QApplication::setOrganizationDomain(QStringLiteral("github.io.xmonader"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.xmonader.qiptv"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Native Qt IPTV client for Xtream Codes providers"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    // One instance per user: a second launch raises the running window.
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) {
        runtimeDir = QDir::tempPath();
    }
    qiptv::SingleInstance instance(runtimeDir, QStringLiteral("io.github.xmonader.qiptv"));
    if (!instance.tryBecomePrimary()) {
        return 0;
    }

    auto *window = new qiptv::MainWindow();
    QObject::connect(&instance, &qiptv::SingleInstance::activationRequested, window, [window] {
        window->showNormal();
        window->raise();
        window->activateWindow();
    });
    window->show();

    return app.exec();
}
