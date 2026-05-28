#include "app/mainwindow.h"

#include "api/xtreamclient.h"
#include "auth/loginwidget.h"
#include "browse/browsewidget.h"
#include "browse/seriesdetail.h"
#include "core/imageloader.h"
#include "player/playerwidget.h"
#include "models/serverinfo.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>

#include <QStackedWidget>

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
        KMessageBox::error(this, msg, i18nc("@title:window", "Playback"));
    });

    setupActions();
    setupGUI(Default, QStringLiteral("kiptvui.rc"));

    m_login->setProfiles(m_store.profiles(), m_store.lastUsedId());
    tryAutoLogin();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupActions()
{
    KStandardAction::quit(this, &QWidget::close, actionCollection());

    auto *logout = actionCollection()->addAction(QStringLiteral("logout"));
    logout->setText(i18n("Switch Profile…"));
    logout->setIcon(QIcon::fromTheme(QStringLiteral("system-switch-user")));
    connect(logout, &QAction::triggered, this, &MainWindow::showLogin);
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
