#include "api/urlbuilder.h"

#include <QUrlQuery>

namespace qiptv {

UrlBuilder::UrlBuilder(const QString &serverUrl, const QString &username, const QString &password)
    : m_base(normalizeBase(serverUrl))
    , m_username(username)
    , m_password(password)
{
}

QString UrlBuilder::normalizeBase(const QString &serverUrl)
{
    QString s = serverUrl.trimmed();
    if (s.isEmpty()) {
        return QString();
    }
    // Default to http when the user omits the scheme.
    if (!s.contains(QStringLiteral("://"))) {
        s.prepend(QStringLiteral("http://"));
    }
    // Drop any trailing slashes so path concatenation is clean.
    while (s.endsWith(QLatin1Char('/'))) {
        s.chop(1);
    }
    return s;
}

QUrl UrlBuilder::api(const QString &action,
                     const QList<QPair<QString, QString>> &params) const
{
    QUrl url(m_base + QStringLiteral("/player_api.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("username"), m_username);
    q.addQueryItem(QStringLiteral("password"), m_password);
    if (!action.isEmpty()) {
        q.addQueryItem(QStringLiteral("action"), action);
    }
    for (const auto &p : params) {
        q.addQueryItem(p.first, p.second);
    }
    url.setQuery(q);
    return url;
}

QString UrlBuilder::streamUrl(const QString &kind, const QString &idAndExt) const
{
    // Credentials live in the path for stream URLs (Xtream convention).
    return m_base + QLatin1Char('/') + kind + QLatin1Char('/')
        + m_username + QLatin1Char('/') + m_password + QLatin1Char('/') + idAndExt;
}

QString UrlBuilder::liveUrl(int streamId, const QString &ext) const
{
    return streamUrl(QStringLiteral("live"),
                     QString::number(streamId) + QLatin1Char('.') + ext);
}

QString UrlBuilder::vodUrl(int streamId, const QString &ext) const
{
    const QString e = ext.isEmpty() ? QStringLiteral("mp4") : ext;
    return streamUrl(QStringLiteral("movie"),
                     QString::number(streamId) + QLatin1Char('.') + e);
}

QString UrlBuilder::seriesUrl(const QString &episodeId, const QString &ext) const
{
    const QString e = ext.isEmpty() ? QStringLiteral("mp4") : ext;
    return streamUrl(QStringLiteral("series"), episodeId + QLatin1Char('.') + e);
}

} // namespace qiptv
