#include "config/settings.h"

#include <KConfigGroup>

#include <QtGlobal>

namespace kiptv {

namespace {
constexpr char kPlayback[] = "Playback";
constexpr char kUi[] = "UI";
} // namespace

Settings::Settings(const QString &configPath)
{
    if (configPath.isEmpty()) {
        m_config = KSharedConfig::openConfig(QStringLiteral("kiptvrc"));
    } else {
        m_config = KSharedConfig::openConfig(configPath, KConfig::SimpleConfig);
    }
}

QString Settings::liveContainer() const
{
    return m_config->group(QString::fromLatin1(kPlayback))
        .readEntry("LiveContainer", QStringLiteral("ts"));
}

void Settings::setLiveContainer(const QString &ext)
{
    m_config->group(QString::fromLatin1(kPlayback)).writeEntry("LiveContainer", ext);
}

bool Settings::minimizeToTray() const
{
    return m_config->group(QString::fromLatin1(kUi)).readEntry("MinimizeToTray", false);
}

void Settings::setMinimizeToTray(bool on)
{
    m_config->group(QString::fromLatin1(kUi)).writeEntry("MinimizeToTray", on);
}

bool Settings::networkCaching() const
{
    return m_config->group(QString::fromLatin1(kPlayback)).readEntry("NetworkCaching", true);
}

void Settings::setNetworkCaching(bool on)
{
    m_config->group(QString::fromLatin1(kPlayback)).writeEntry("NetworkCaching", on);
}

int Settings::lastVolume() const
{
    return qBound(0, m_config->group(QString::fromLatin1(kPlayback)).readEntry("Volume", 100), 150);
}

void Settings::setLastVolume(int volume)
{
    m_config->group(QString::fromLatin1(kPlayback)).writeEntry("Volume", qBound(0, volume, 150));
}

void Settings::save()
{
    m_config->sync();
}

} // namespace kiptv
