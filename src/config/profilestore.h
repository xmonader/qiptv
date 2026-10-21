#pragma once

#include "config/profile.h"

#include <QSettings>
#include <QString>
#include <QVector>

#include <memory>

namespace kiptv {

/**
 * Persists provider profiles via QSettings (INI format).
 *
 * Layout in the config file (~/.config/kiptvrc, shared with Settings):
 *   [Profiles]      order=id1,id2,...
 *   [Profile-<id>]  Name=, Server=, Username=, Password=
 *   LastProfile=<id>  (top-level key; QSettings stores root keys in the
 *                     [General] section, matching the legacy KConfig layout)
 *
 * Note: QSettings' INI parser maps a [General] section to the root namespace,
 * so LastProfile must be accessed as a top-level key, not "General/LastProfile".
 *
 * Passwords are stored with light base64 obfuscation. This is NOT
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

    std::unique_ptr<QSettings> m_config; ///< value() is const-safe; writers go through get()
};

} // namespace kiptv
