#pragma once

#include "config/profile.h"
#include "config/profilestore.h"

#include <KXmlGuiWindow>

class QStackedWidget;
class QLabel;

namespace kiptv {

class LoginWidget;
class XtreamClient;
struct ServerInfo;

/**
 * Main application window.
 *
 * Owns the profile store, the Xtream client, and the page stack. Phase 3
 * wires login -> authenticate -> (save profile) -> connected placeholder.
 * Phase 4 replaces the placeholder with the browsing UI.
 */
class MainWindow : public KXmlGuiWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupActions();
    void tryAutoLogin();

    void onLoginRequested(const Profile &profile);
    void onAuthenticated(const ServerInfo &info);
    void onError(const QString &message);
    void showLogin();

    QStackedWidget *m_stack = nullptr;
    LoginWidget *m_login = nullptr;
    QLabel *m_connected = nullptr;
    XtreamClient *m_client = nullptr;
    ProfileStore m_store;
    Profile m_pending; ///< profile awaiting save once auth succeeds
};

} // namespace kiptv
