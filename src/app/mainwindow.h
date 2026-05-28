#pragma once

#include "config/profile.h"
#include "config/profilestore.h"
#include "config/settings.h"

#include <KXmlGuiWindow>

class QStackedWidget;
class QSystemTrayIcon;

namespace kiptv {

class LoginWidget;
class BrowseWidget;
class SeriesDetail;
class PlayerWidget;
class XtreamClient;
class ImageLoader;
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

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupActions();
    void setupSystemTray();
    void setupGlobalShortcuts();
    void notify(const QString &eventId, const QString &title, const QString &text);
    void openSettings();
    void applySettings();
    void tryAutoLogin();

    void onLoginRequested(const Profile &profile);
    void onAuthenticated(const ServerInfo &info);
    void onError(const QString &message);
    void onPlayRequested(const QString &url, const QString &title);
    void onOpenSeries(const QString &seriesId, const QString &name);
    void showLogin();

    QStackedWidget *m_stack = nullptr;
    LoginWidget *m_login = nullptr;
    BrowseWidget *m_browse = nullptr;
    SeriesDetail *m_seriesDetail = nullptr;
    PlayerWidget *m_player = nullptr;
    QWidget *m_lastBrowsePage = nullptr; ///< page to return to after playback
    QSystemTrayIcon *m_tray = nullptr;
    XtreamClient *m_client = nullptr;
    ImageLoader *m_images = nullptr;
    ProfileStore m_store;
    Settings m_settings;
    Profile m_pending;        ///< profile awaiting save once auth succeeds
    bool m_forceQuit = false; ///< set by the tray Quit action to bypass close-to-tray
};

} // namespace kiptv
