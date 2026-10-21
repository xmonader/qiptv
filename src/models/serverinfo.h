#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace qiptv {

/// The "user_info" block of the player_api.php auth response.
struct UserInfo {
    QString username;
    QString status;            ///< "Active", "Expired", "Banned", ...
    QString expDate;           ///< unix timestamp (string) or empty
    bool isTrial = false;
    int activeConnections = 0;
    int maxConnections = 0;
    QStringList allowedOutputFormats; ///< e.g. ["m3u8","ts","rtmp"]

    bool isActive() const { return status.compare(QStringLiteral("Active"), Qt::CaseInsensitive) == 0; }

    static UserInfo fromJson(const QJsonObject &o);
};

/// The "server_info" block of the auth response.
struct ServerDetail {
    QString url;
    QString port;
    QString httpsPort;
    QString serverProtocol;    ///< "http" / "https"
    QString timezone;

    static ServerDetail fromJson(const QJsonObject &o);
};

/// Full auth response. authValid is false when credentials are rejected.
struct ServerInfo {
    bool authValid = false;
    UserInfo user;
    ServerDetail server;

    /// Parse a full player_api.php auth response body.
    static ServerInfo fromJson(const QByteArray &json);
};

} // namespace qiptv
