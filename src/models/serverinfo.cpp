#include "models/serverinfo.h"

#include "core/parse.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace qiptv {

UserInfo UserInfo::fromJson(const QJsonObject &o)
{
    UserInfo u;
    u.username = parse::str(o.value(QStringLiteral("username")));
    u.status = parse::str(o.value(QStringLiteral("status")));
    u.expDate = parse::str(o.value(QStringLiteral("exp_date")));
    u.isTrial = parse::boolean(o.value(QStringLiteral("is_trial")));
    u.activeConnections = parse::integer(o.value(QStringLiteral("active_cons")));
    u.maxConnections = parse::integer(o.value(QStringLiteral("max_connections")));

    // allowed_output_formats is an array of strings, but be defensive.
    const QJsonValue fmts = o.value(QStringLiteral("allowed_output_formats"));
    if (fmts.isArray()) {
        for (const QJsonValue &f : fmts.toArray()) {
            const QString s = parse::str(f);
            if (!s.isEmpty()) {
                u.allowedOutputFormats << s;
            }
        }
    }
    return u;
}

ServerDetail ServerDetail::fromJson(const QJsonObject &o)
{
    ServerDetail s;
    s.url = parse::str(o.value(QStringLiteral("url")));
    s.port = parse::str(o.value(QStringLiteral("port")));
    s.httpsPort = parse::str(o.value(QStringLiteral("https_port")));
    s.serverProtocol = parse::str(o.value(QStringLiteral("server_protocol")));
    s.timezone = parse::str(o.value(QStringLiteral("timezone")));
    return s;
}

ServerInfo ServerInfo::fromJson(const QByteArray &json)
{
    ServerInfo info;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return info; // authValid stays false
    }
    const QJsonObject root = doc.object();

    info.user = UserInfo::fromJson(root.value(QStringLiteral("user_info")).toObject());
    info.server = ServerDetail::fromJson(root.value(QStringLiteral("server_info")).toObject());

    // Xtream signals success via user_info.auth == 1 and a populated username.
    const QJsonObject userObj = root.value(QStringLiteral("user_info")).toObject();
    const bool authFlag = parse::boolean(userObj.value(QStringLiteral("auth")), true);
    info.authValid = authFlag && !info.user.username.isEmpty();
    return info;
}

} // namespace qiptv
