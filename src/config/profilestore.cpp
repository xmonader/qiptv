#include "config/profilestore.h"

#include <QStandardPaths>

#include <QUuid>

namespace kiptv {

namespace {
constexpr char kProfilesGroup[] = "Profiles";
constexpr char kOrderKey[] = "order";
constexpr char kLastKey[] = "LastProfile";

QString groupFor(const QString &id)
{
    return QStringLiteral("Profile-") + id;
}

QStringList stringListValue(const QSettings *config, const QString &key)
{
    // KConfig and QSettings both persist lists as comma-separated entries, but
    // the QVariant may arrive as either QStringList or a plain string depending
    // on which library wrote the file. Handle both so existing kiptvrc files
    // keep working.
    const QVariant v = config->value(key);
    if (v.typeId() == QMetaType::QStringList) {
        return v.toStringList();
    }
    return v.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
}
// Light obfuscation only — see header. Reversible, not secure.
QString obfuscate(const QString &s)
{
    return QString::fromLatin1(s.toUtf8().toBase64());
}
QString deobfuscate(const QString &s)
{
    return QString::fromUtf8(QByteArray::fromBase64(s.toLatin1()));
}
} // namespace

ProfileStore::ProfileStore(const QString &configPath)
{
    // Same default file KConfig used: $XDG_CONFIG_HOME/kiptvrc. QSettings'
    // org/app defaults would nest it under a subdirectory, so build the path
    // explicitly.
    const QString path = configPath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
              + QStringLiteral("/kiptvrc")
        : configPath;
    m_config = std::make_unique<QSettings>(path, QSettings::IniFormat);
}

QStringList ProfileStore::orderedIds() const
{
    return stringListValue(m_config.get(),
                           QStringLiteral("%1/%2").arg(QLatin1String(kProfilesGroup),
                                                       QLatin1String(kOrderKey)));
}

void ProfileStore::setOrderedIds(const QStringList &ids)
{
    m_config->setValue(QStringLiteral("%1/%2").arg(QLatin1String(kProfilesGroup),
                                                   QLatin1String(kOrderKey)),
                       ids);
}

QVector<Profile> ProfileStore::profiles() const
{
    QVector<Profile> out;
    const QStringList ids = orderedIds();
    out.reserve(ids.size());
    for (const QString &id : ids) {
        out.push_back(profile(id));
    }
    return out;
}

Profile ProfileStore::profile(const QString &id) const
{
    Profile p;
    const QString group = groupFor(id);
    if (id.isEmpty() || !m_config->contains(group + QStringLiteral("/Name"))) {
        return p;
    }
    p.id = id;
    p.name = m_config->value(group + QStringLiteral("/Name")).toString();
    p.serverUrl = m_config->value(group + QStringLiteral("/Server")).toString();
    p.username = m_config->value(group + QStringLiteral("/Username")).toString();
    p.password = deobfuscate(m_config->value(group + QStringLiteral("/Password")).toString());
    return p;
}

QString ProfileStore::upsert(Profile profile)
{
    if (profile.id.isEmpty()) {
        profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    const QString group = groupFor(profile.id);
    m_config->setValue(group + QStringLiteral("/Name"), profile.name);
    m_config->setValue(group + QStringLiteral("/Server"), profile.serverUrl);
    m_config->setValue(group + QStringLiteral("/Username"), profile.username);
    m_config->setValue(group + QStringLiteral("/Password"), obfuscate(profile.password));

    QStringList ids = orderedIds();
    if (!ids.contains(profile.id)) {
        ids.append(profile.id);
        setOrderedIds(ids);
    }
    return profile.id;
}

void ProfileStore::remove(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }
    m_config->remove(groupFor(id));

    QStringList ids = orderedIds();
    if (ids.removeAll(id) > 0) {
        setOrderedIds(ids);
    }
    if (lastUsedId() == id) {
        setLastUsedId(QString());
    }
}

QString ProfileStore::lastUsedId() const
{
    // Top-level key: QSettings' INI backend treats a [General] section as the
    // root namespace, which is where KConfig wrote this entry.
    return m_config->value(QLatin1String(kLastKey)).toString();
}

void ProfileStore::setLastUsedId(const QString &id)
{
    m_config->setValue(QLatin1String(kLastKey), id);
}

void ProfileStore::save()
{
    m_config->sync();
}

} // namespace kiptv
