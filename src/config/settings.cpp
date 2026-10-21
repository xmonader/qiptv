#include "config/settings.h"

#include <QStandardPaths>

#include <QtGlobal>

namespace qiptv {

namespace {
constexpr char kPlayback[] = "Playback";
constexpr char kUi[] = "UI";
} // namespace

Settings::Settings(const QString &configPath)
{
    // Shares the legacy qiptvrc file with ProfileStore (same path logic).
    const QString path = configPath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
              + QStringLiteral("/qiptvrc")
        : configPath;
    m_config = std::make_unique<QSettings>(path, QSettings::IniFormat);
}

QString Settings::liveContainer() const
{
    return m_config->value(QStringLiteral("%1/LiveContainer").arg(QLatin1String(kPlayback)),
                           QStringLiteral("ts"))
        .toString();
}

void Settings::setLiveContainer(const QString &ext)
{
    m_config->setValue(QStringLiteral("%1/LiveContainer").arg(QLatin1String(kPlayback)), ext);
}

bool Settings::minimizeToTray() const
{
    return m_config->value(QStringLiteral("%1/MinimizeToTray").arg(QLatin1String(kUi)), false)
        .toBool();
}

void Settings::setMinimizeToTray(bool on)
{
    m_config->setValue(QStringLiteral("%1/MinimizeToTray").arg(QLatin1String(kUi)), on);
}

bool Settings::networkCaching() const
{
    return m_config->value(QStringLiteral("%1/NetworkCaching").arg(QLatin1String(kPlayback)), true)
        .toBool();
}

void Settings::setNetworkCaching(bool on)
{
    m_config->setValue(QStringLiteral("%1/NetworkCaching").arg(QLatin1String(kPlayback)), on);
}

int Settings::lastVolume() const
{
    return qBound(0,
                  m_config->value(QStringLiteral("%1/Volume").arg(QLatin1String(kPlayback)), 100)
                      .toInt(),
                  150);
}

void Settings::setLastVolume(int volume)
{
    m_config->setValue(QStringLiteral("%1/Volume").arg(QLatin1String(kPlayback)),
                       qBound(0, volume, 150));
}

void Settings::save()
{
    m_config->sync();
}

} // namespace qiptv
