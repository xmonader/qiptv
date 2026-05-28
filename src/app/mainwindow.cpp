#include "app/mainwindow.h"

#include "api/xtreamclient.h"
#include "auth/loginwidget.h"
#include "models/serverinfo.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KStandardAction>

#include <QLabel>
#include <QStackedWidget>

namespace kiptv {

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    m_client = new XtreamClient(this);

    m_stack = new QStackedWidget(this);
    m_login = new LoginWidget(this);
    m_connected = new QLabel(this);
    m_connected->setAlignment(Qt::AlignCenter);
    m_connected->setWordWrap(true);

    m_stack->addWidget(m_login);     // index 0
    m_stack->addWidget(m_connected); // index 1
    setCentralWidget(m_stack);

    connect(m_login, &LoginWidget::loginRequested, this, &MainWindow::onLoginRequested);
    connect(m_client, &XtreamClient::authenticated, this, &MainWindow::onAuthenticated);
    connect(m_client, &XtreamClient::errorOccurred, this, &MainWindow::onError);

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

    m_connected->setText(i18n("Connected as <b>%1</b>.\n\nChannel browsing arrives in Phase 4.",
                              info.user.username));
    m_stack->setCurrentWidget(m_connected);
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
