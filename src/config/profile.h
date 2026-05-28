#pragma once

#include <QString>

namespace kiptv {

/// A saved Xtream provider account.
struct Profile {
    QString id;        ///< stable unique key (generated on first save)
    QString name;      ///< user-facing label
    QString serverUrl;
    QString username;
    QString password;

    bool isValid() const
    {
        return !serverUrl.isEmpty() && !username.isEmpty();
    }
};

} // namespace kiptv
