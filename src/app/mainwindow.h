#pragma once

#include "config/profile.h"
#include "config/profilestore.h"

#include <KXmlGuiWindow>

class QStackedWidget;

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

private:
    void setupActions();
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
    XtreamClient *m_client = nullptr;
    ImageLoader *m_images = nullptr;
    ProfileStore m_store;
    Profile m_pending; ///< profile awaiting save once auth succeeds
};

} // namespace kiptv
