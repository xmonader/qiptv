#include "app/mainwindow.h"

#include "api/xtreamclient.h"
#include "app/settingsdialog.h"
#include "auth/loginwidget.h"
#include "browse/browsewidget.h"
#include "browse/seriesdetail.h"
#include "core/imageloader.h"
#include "player/playerwidget.h"
#include "models/serverinfo.h"

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KStandardAction>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QStackedWidget>
#include <QSystemTrayIcon>

#include <functional>
#include <vector>

namespace kiptv {

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    m_client = new XtreamClient(this);
    m_images = new ImageLoader(this);

    m_stack = new QStackedWidget(this);
    m_login = new LoginWidget(this);
    m_browse = new BrowseWidget(m_client, m_images, this);
    m_seriesDetail = new SeriesDetail(m_client, m_images, this);
    m_player = new PlayerWidget(this);

    m_stack->addWidget(m_login);        // index 0
    m_stack->addWidget(m_browse);       // index 1
    m_stack->addWidget(m_seriesDetail); // index 2
    m_stack->addWidget(m_player);       // index 3
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
    connect(m_player, &PlayerWidget::backRequested, this, [this] {
        m_stack->setCurrentWidget(m_lastBrowsePage ? m_lastBrowsePage : m_browse);
    });
    connect(m_player, &PlayerWidget::nextRequested, m_browse, [this] { m_browse->zap(1); });
    connect(m_player, &PlayerWidget::previousRequested, m_browse, [this] { m_browse->zap(-1); });
    // Playback errors are informational; they must not bounce to the login page.
    connect(m_player, &PlayerWidget::errorOccurred, this, [this](const QString &msg) {
        notify(QStringLiteral("playbackError"), i18n("Playback error"), msg);
        KMessageBox::error(this, msg, i18nc("@title:window", "Playback"));
    });

    setupActions();
    setupGUI(Default, QStringLiteral("kiptvui.rc"));
    setupSystemTray();
    setupGlobalShortcuts();
    applySettings();

    m_login->setProfiles(m_store.profiles(), m_store.lastUsedId());
    tryAutoLogin();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupActions()
{
    KStandardAction::quit(this, [this] {
        m_forceQuit = true;
        close();
    }, actionCollection());

    auto *logout = actionCollection()->addAction(QStringLiteral("logout"));
    logout->setText(i18n("Switch Profile…"));
    logout->setIcon(QIcon::fromTheme(QStringLiteral("system-switch-user")));
    connect(logout, &QAction::triggered, this, &MainWindow::showLogin);

    KStandardAction::preferences(this, &MainWindow::openSettings, actionCollection());
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
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Minimize to tray instead of quitting, unless the user explicitly quit.
    if (!m_forceQuit && m_tray && m_settings.minimizeToTray() && m_tray->isVisible()) {
        hide();
        event->ignore();
        return;
    }
    KXmlGuiWindow::closeEvent(event);
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon::fromTheme(QStringLiteral("video-television")));
    m_tray->setToolTip(i18n("kiptv"));

    auto *menu = new QMenu(this);
    auto *toggle = menu->addAction(i18n("Show/Hide Window"));
    connect(toggle, &QAction::triggered, this, [this] {
        setVisible(!isVisible());
        if (isVisible()) {
            raise();
            activateWindow();
        }
    });
    auto *playpause = menu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                                      i18n("Play/Pause"));
    connect(playpause, &QAction::triggered, m_player, &PlayerWidget::togglePause);
    menu->addSeparator();
    auto *quit = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Quit"));
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

void MainWindow::setupGlobalShortcuts()
{
    // Bind multimedia keys to playback control via KGlobalAccel. Registration
    // is a no-op (with a warning) where the global-accel daemon is absent.
    struct Binding {
        const char *name;
        QString text;
        Qt::Key key;
        std::function<void()> slot;
    };
    const std::vector<Binding> bindings = {
        {"media_playpause", i18n("Play/Pause"), Qt::Key_MediaTogglePlayPause,
         [this] { m_player->togglePause(); }},
        {"media_stop", i18n("Stop"), Qt::Key_MediaStop, [this] { m_player->stop(); }},
        {"media_next", i18n("Next Channel"), Qt::Key_MediaNext, [this] { m_browse->zap(1); }},
        {"media_previous", i18n("Previous Channel"), Qt::Key_MediaPrevious,
         [this] { m_browse->zap(-1); }},
    };
    for (const auto &b : bindings) {
        auto *action = new QAction(b.text, this);
        action->setObjectName(QString::fromLatin1(b.name));
        connect(action, &QAction::triggered, this, b.slot);
        KGlobalAccel::setGlobalShortcut(action, QList<QKeySequence>{QKeySequence(b.key)});
    }
}

void MainWindow::notify(const QString &eventId, const QString &title, const QString &text)
{
    KNotification *n = new KNotification(eventId);
    n->setComponentName(QStringLiteral("kiptv"));
    n->setTitle(title);
    n->setText(text);
    n->sendEvent();
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

    setWindowTitle(i18nc("@title:window", "kiptv — %1", info.user.username));
    m_browse->reload();
    m_stack->setCurrentWidget(m_browse);
}

void MainWindow::onPlayRequested(const QString &url, const QString &title)
{
    // Remember whether we came from the browser or a series detail so Back
    // returns to the right place. When zapping (already on the player), keep
    // the remembered page.
    QWidget *cur = m_stack->currentWidget();
    if (cur == m_browse || cur == m_seriesDetail) {
        m_lastBrowsePage = cur;
    }
    m_player->play(url, title);
    m_stack->setCurrentWidget(m_player);
    notify(QStringLiteral("nowPlaying"), i18n("Now playing"), title);
    if (m_tray) {
        m_tray->setToolTip(i18n("kiptv — %1", title));
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
