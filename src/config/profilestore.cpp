#include "config/profilestore.h"

#include <KConfigGroup>

#include <QUuid>

namespace kiptv {

namespace {
constexpr char kProfilesGroup[] = "Profiles";
constexpr char kOrderKey[] = "order";
constexpr char kGeneralGroup[] = "General";
constexpr char kLastKey[] = "LastProfile";

QString groupFor(const QString &id)
{
    return QStringLiteral("Profile-") + id;
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
    if (configPath.isEmpty()) {
        m_config = KSharedConfig::openConfig(QStringLiteral("kiptvrc"));
    } else {
        m_config = KSharedConfig::openConfig(configPath, KConfig::SimpleConfig);
    }
}

QStringList ProfileStore::orderedIds() const
{
    return m_config->group(QString::fromLatin1(kProfilesGroup))
        .readEntry(QString::fromLatin1(kOrderKey), QStringList());
}

void ProfileStore::setOrderedIds(const QStringList &ids)
{
    m_config->group(QString::fromLatin1(kProfilesGroup))
        .writeEntry(QString::fromLatin1(kOrderKey), ids);
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
    if (id.isEmpty() || !m_config->hasGroup(groupFor(id))) {
        return p;
    }
    const KConfigGroup g = m_config->group(groupFor(id));
    p.id = id;
    p.name = g.readEntry("Name", QString());
    p.serverUrl = g.readEntry("Server", QString());
    p.username = g.readEntry("Username", QString());
    p.password = deobfuscate(g.readEntry("Password", QString()));
    return p;
}

QString ProfileStore::upsert(Profile profile)
{
    if (profile.id.isEmpty()) {
        profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    KConfigGroup g = m_config->group(groupFor(profile.id));
    g.writeEntry("Name", profile.name);
    g.writeEntry("Server", profile.serverUrl);
    g.writeEntry("Username", profile.username);
    g.writeEntry("Password", obfuscate(profile.password));

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
    m_config->deleteGroup(groupFor(id));

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
    return m_config->group(QString::fromLatin1(kGeneralGroup))
        .readEntry(QString::fromLatin1(kLastKey), QString());
}

void ProfileStore::setLastUsedId(const QString &id)
{
    m_config->group(QString::fromLatin1(kGeneralGroup))
        .writeEntry(QString::fromLatin1(kLastKey), id);
}

void ProfileStore::save()
{
    m_config->sync();
}

} // namespace kiptv
