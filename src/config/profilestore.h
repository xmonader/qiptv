#pragma once

#include "config/profile.h"

#include <KSharedConfig>

#include <QString>
#include <QVector>

namespace kiptv {

/**
 * Persists provider profiles via KConfig.
 *
 * Layout in the config file:
 *   [Profiles]      order=id1,id2,...
 *   [Profile-<id>]  Name=, Server=, Username=, Password=
 *   [General]       LastProfile=<id>
 *
 * KWallet is the preferred secret store but is unavailable on this host, so
 * passwords fall back to KConfig with light base64 obfuscation. This is NOT
 * encryption — it only avoids plaintext grep hits. Documented as a known
 * limitation.
 */
class ProfileStore {
public:
    /// Default: standard kiptvrc location. Pass an absolute path for tests.
    explicit ProfileStore(const QString &configPath = QString());

    QVector<Profile> profiles() const;
    Profile profile(const QString &id) const;

    /// Add or update by id. If profile.id is empty, a new id is generated and
    /// returned; otherwise the existing entry is replaced.
    QString upsert(Profile profile);
    void remove(const QString &id);

    QString lastUsedId() const;
    void setLastUsedId(const QString &id);

    /// Flush to disk.
    void save();

private:
    QStringList orderedIds() const;
    void setOrderedIds(const QStringList &ids);

    KSharedConfig::Ptr m_config;
};

} // namespace kiptv
