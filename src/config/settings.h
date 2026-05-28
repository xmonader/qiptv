#pragma once

#include <KSharedConfig>

#include <QString>

namespace kiptv {

/**
 * Application preferences, persisted via KConfig (shares kiptvrc with the
 * profile store but uses its own groups). Plain getters/setters keep it
 * trivially testable; call save() to flush.
 */
class Settings {
public:
    explicit Settings(const QString &configPath = QString());

    QString liveContainer() const;     ///< stream extension for live TV ("ts"/"m3u8")
    void setLiveContainer(const QString &ext);

    bool minimizeToTray() const;       ///< hide to tray on window close
    void setMinimizeToTray(bool on);

    bool networkCaching() const;       ///< enable mpv stream cache
    void setNetworkCaching(bool on);

    int lastVolume() const;            ///< 0–150
    void setLastVolume(int volume);

    void save();

private:
    KSharedConfig::Ptr m_config;
};

} // namespace kiptv
