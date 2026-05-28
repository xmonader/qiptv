#include "app/mainwindow.h"

#include "api/xtreamclient.h"
#include "auth/loginwidget.h"
#include "browse/browsewidget.h"
#include "core/imageloader.h"
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

    m_stack->addWidget(m_login);  // index 0
    m_stack->addWidget(m_browse); // index 1
    setCentralWidget(m_stack);

    connect(m_login, &LoginWidget::loginRequested, this, &MainWindow::onLoginRequested);
    connect(m_client, &XtreamClient::authenticated, this, &MainWindow::onAuthenticated);
    connect(m_client, &XtreamClient::errorOccurred, this, &MainWindow::onError);
    connect(m_browse, &BrowseWidget::playRequested, this, &MainWindow::onPlayRequested);
    connect(m_browse, &BrowseWidget::openSeriesRequested, this, &MainWindow::onOpenSeries);

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
    // Phase 5 embeds libmpv. For now confirm the stream URL resolves correctly.
    KMessageBox::information(this,
                             i18n("Would play <b>%1</b>:\n%2\n\n(Player arrives in Phase 5.)",
                                  title, url),
                             i18nc("@title:window", "Play"));
}

void MainWindow::onOpenSeries(const QString &seriesId, const QString &name)
{
    Q_UNUSED(seriesId)
    // Phase 4 series detail view is wired in a follow-up step.
    KMessageBox::information(this, i18n("Series detail for <b>%1</b> arrives next.", name),
                             i18nc("@title:window", "Series"));
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
