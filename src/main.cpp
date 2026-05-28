#include "app/mainwindow.h"

#include <KAboutData>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>

using namespace kiptv;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kiptv");

    KAboutData about(
        QStringLiteral("kiptv"),
        i18n("kiptv"),
        QStringLiteral("0.1.0"),
        i18n("Native KDE IPTV client for Xtream Codes providers"),
        KAboutLicense::MIT,
        i18n("(c) 2026"));
    about.addAuthor(i18n("xmonader"), QString(), QStringLiteral("xmonader@gmail.com"));
    about.setHomepage(QStringLiteral("https://github.com/xmonader/kiptv"));
    about.setDesktopFileName(QStringLiteral("io.github.xmonader.kiptv"));
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    auto *window = new MainWindow();
    window->show();

    return app.exec();
}
