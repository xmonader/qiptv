#include "app/mainwindow.h"

#include "api/xtreamclient.h"
#include "app/settingsdialog.h"
#include "auth/loginwidget.h"
#include "browse/browsewidget.h"
#include "browse/seriesdetail.h"
#include "core/imageloader.h"
#include "player/playerwidget.h"
#include "models/serverinfo.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QToolBar>

#include <functional>
#include <vector>

namespace kiptv {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_client = new XtreamClient(this);
    m_images = new ImageLoader(this);

    m_stack = new QStackedWidget(this);
    m_login = new LoginWidget(this);
    m_browse = new BrowseWidget(m_client, m_images, this);
    m_seriesDetail = new SeriesDetail(m_client, m_images, this);

    m_stack->addWidget(m_login);        // index 0
    m_stack->addWidget(m_browse);       // index 1
    m_stack->addWidget(m_seriesDetail); // index 2
    setCentralWidget(m_stack);

    connect(m_login, &LoginWidget::loginRequested, this, &MainWindow::onLoginRequested);
    connect(m_client, &XtreamClient::authenticated, this, &MainWindow::onAuthenticated);
    connect(m_client, &XtreamClient::errorOccurred, this, &MainWindow::onError);
    connect(m_browse, &BrowseWidget::playRequested, this, &MainWindow::onPlayRequested);
    connect(m_browse, &BrowseWidget::openSeriesRequested, this, &MainWindow::onOpenSeries);
    connect(m_seriesDetail, &SeriesDetail::playEpisode, this, &MainWindow::onPlayRequested);
    connect(m_seriesDetail, &SeriesDetail::backRequested, this, [this] {
        m_stack->setCurrentWidget(m_browse);
    });

    setupActions();
    setupSystemTray();
    setupMediaKeys();
    applySettings();

    m_login->setProfiles(m_store.profiles(), m_store.lastUsedId());
    tryAutoLogin();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupActions()
{
    auto *quit = new QAction(QIcon::fromTheme(QStringLiteral("application-exit")),
                             tr("Quit"), this);
    quit->setObjectName(QStringLiteral("quit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, [this] {
        m_forceQuit = true;
        close();
    });
    addAction(quit);

    auto *logout = new QAction(QIcon::fromTheme(QStringLiteral("system-switch-user")),
                               tr("Switch Profile…"), this);
    logout->setObjectName(QStringLiteral("logout"));
    connect(logout, &QAction::triggered, this, &MainWindow::showLogin);
    addAction(logout);

    auto *preferences = new QAction(QIcon::fromTheme(QStringLiteral("preferences-system")),
                                    tr("Preferences"), this);
    preferences->setObjectName(QStringLiteral("preferences"));
    preferences->setShortcut(QKeySequence::Preferences);
    connect(preferences, &QAction::triggered, this, &MainWindow::openSettings);
    addAction(preferences);

    // Menus replace the KXmlGui kiptvui.rc layout (File > Switch Profile,
    // toolbar entry) with the equivalent QMainWindow structure.
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(logout);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);

    auto *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(preferences);

    auto *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setObjectName(QStringLiteral("mainToolBar"));
    toolBar->setMovable(false);
    toolBar->addAction(logout);
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_settings, this);
    if (dialog.exec() == QDialog::Accepted) {
        applySettings();
    }
}

void MainWindow::applySettings()
{
    m_browse->setLiveContainer(m_settings.liveContainer());
    if (m_player) {
        m_player->setInitialVolume(m_settings.lastVolume());
    }
}

PlayerWidget *MainWindow::ensurePlayer()
{
    if (m_player) {
        return m_player;
    }
    // The player is a dedicated top-level window (Qt::Window child of the main
    // window, so it does not count as an independent app for quit purposes).
    // Video + controls live together inside it. The earlier "video appeared in
    // its own OS window" bug was NOT a windowing issue: mpv reverts to a
    // windowed VO when loadfile runs before the render context exists — fixed
    // by vo=libmpv in MpvWidget plus show()-before-play() in onPlayRequested.
    m_player = new PlayerWidget(this);
    m_player->setWindowFlag(Qt::Window);
    m_player->setWindowTitle(tr("kiptv — Player"));
    m_player->resize(960, 540);

    connect(m_player, &PlayerWidget::backRequested, this, [this] {
        m_player->hide();
        raise();
        activateWindow();
    });
    connect(m_player, &PlayerWidget::nextRequested, m_browse, [this] { m_browse->zap(1); });
    connect(m_player, &PlayerWidget::previousRequested, m_browse, [this] { m_browse->zap(-1); });
    connect(m_player, &PlayerWidget::volumeChanged, this,
            [this](int v) { m_settings.setLastVolume(v); });
    // Playback errors are informational; just report them.
    connect(m_player, &PlayerWidget::errorOccurred, this, [this](const QString &msg) {
        notify(tr("Playback error"), msg);
        QMessageBox::critical(this, tr("Playback"), msg);
    });

    m_player->setInitialVolume(m_settings.lastVolume());
    return m_player;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Minimize to tray instead of quitting, unless the user explicitly quit.
    if (!m_forceQuit && m_tray && m_settings.minimizeToTray() && m_tray->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    m_settings.save(); // persist last volume and any pending prefs
    QMainWindow::closeEvent(event);
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon::fromTheme(QStringLiteral("video-television")));
    m_tray->setToolTip(tr("kiptv"));

    auto *menu = new QMenu(this);
    auto *toggle = menu->addAction(tr("Show/Hide Window"));
    connect(toggle, &QAction::triggered, this, [this] {
        setVisible(!isVisible());
        if (isVisible()) {
            raise();
            activateWindow();
        }
    });
    auto *playpause = menu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                                      tr("Play/Pause"));
    connect(playpause, &QAction::triggered, this, [this] {
        if (m_player) {
            m_player->togglePause();
        }
    });
    menu->addSeparator();
    auto *quit = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), tr("Quit"));
    connect(quit, &QAction::triggered, this, [this] {
        m_forceQuit = true;
        qApp->quit();
    });

    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger) {
            setVisible(!isVisible());
            if (isVisible()) {
                raise();
                activateWindow();
            }
        }
    });
    m_tray->show();
}

void MainWindow::setupMediaKeys()
{
    // Bind multimedia keys to playback control. Qt has no API for system-wide
    // key grabbing (KGlobalAccel delegated that to the desktop daemon), so
    // these are application-scope shortcuts: they fire whenever any kiptv
    // window has focus, not while other apps are in the foreground.
    struct Binding {
        Qt::Key key;
        std::function<void()> slot;
    };
    const std::vector<Binding> bindings = {
        {Qt::Key_MediaTogglePlayPause, [this] { if (m_player) m_player->togglePause(); }},
        {Qt::Key_MediaStop, [this] { if (m_player) m_player->stop(); }},
        {Qt::Key_MediaNext, [this] { m_browse->zap(1); }},
        {Qt::Key_MediaPrevious, [this] { m_browse->zap(-1); }},
    };
    for (const auto &b : bindings) {
        auto *shortcut = new QShortcut(QKeySequence(b.key), this);
        shortcut->setContext(Qt::ApplicationShortcut);
        connect(shortcut, &QShortcut::activated, this, b.slot);
    }
}

void MainWindow::notify(const QString &title, const QString &text)
{
    // Desktop notifications ride on the tray icon's D-Bus backend; without a
    // tray there is no Qt-only notification channel, so drop the message.
    if (m_tray && m_tray->isVisible()) {
        m_tray->showMessage(title, text, QSystemTrayIcon::Information, 5000);
    }
}

void MainWindow::tryAutoLogin()
{
    const QString lastId = m_store.lastUsedId();
    if (lastId.isEmpty()) {
        return;
    }
    const Profile p = m_store.profile(lastId);
    if (!p.isValid() || p.password.isEmpty()) {
        return; // nothing to auto-submit
    }
    onLoginRequested(p);
}

void MainWindow::onLoginRequested(const Profile &profile)
{
    m_pending = profile;
    m_login->setBusy(true);
    m_client->setCredentials(profile.serverUrl, profile.username, profile.password);
    m_client->authenticate();
}

void MainWindow::onAuthenticated(const ServerInfo &info)
{
    m_login->setBusy(false);

    if (m_login->saveRequested()) {
        const QString id = m_store.upsert(m_pending);
        m_store.setLastUsedId(id);
        m_store.save();
        m_login->setProfiles(m_store.profiles(), id);
    }

    setWindowTitle(tr("kiptv — %1").arg(info.user.username));
    m_browse->reload();
    m_stack->setCurrentWidget(m_browse);
}

void MainWindow::onPlayRequested(const QString &url, const QString &title)
{
    // Remember whether we came from the browser or a series detail so Back
    // returns to the right place. When zapping (already on the player), keep
    // the remembered page.
    PlayerWidget *player = ensurePlayer();
    // Show BEFORE loading: loadfile triggers video init, which reverts to a
    // windowed VO (a separate OS window) unless the render context already
    // exists. Showing first runs initializeGL() -> mpv_render_context_create().
    player->show();
    player->raise();
    player->activateWindow();
    player->play(url, title);
    notify(tr("Now playing"), title);
    if (m_tray) {
        m_tray->setToolTip(tr("kiptv — %1").arg(title));
    }
}

void MainWindow::onOpenSeries(const QString &seriesId, const QString &name)
{
    m_seriesDetail->open(seriesId, name);
    m_stack->setCurrentWidget(m_seriesDetail);
}

void MainWindow::onError(const QString &message)
{
    m_login->setBusy(false);
    m_login->showError(message);
    m_stack->setCurrentWidget(m_login);
}

void MainWindow::showLogin()
{
    m_login->clearError();
    m_login->setBusy(false);
    m_stack->setCurrentWidget(m_login);
}

} // namespace kiptv
