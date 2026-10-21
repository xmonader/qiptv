#pragma once

#include <QString>
#include <QUrl>

namespace qiptv {

/**
 * Builds Xtream Codes URLs from a server base + credentials.
 *
 * Two URL families:
 *  - API:    {base}/player_api.php?username=U&password=P&action=...
 *  - Stream: {base}/{live|movie|series}/U/P/{id}.{ext}
 *
 * The base is normalized: a missing scheme defaults to http, and any
 * trailing slashes are stripped so path joins are predictable.
 */
class UrlBuilder {
public:
    UrlBuilder() = default;
    UrlBuilder(const QString &serverUrl, const QString &username, const QString &password);

    /// The normalized base (scheme + host + optional port, no trailing slash).
    QString base() const { return m_base; }
    bool isValid() const { return !m_base.isEmpty() && !m_username.isEmpty(); }

    /// player_api.php URL. action empty -> bare auth request. Extra query
    /// params (e.g. category_id, series_id) are appended and percent-encoded.
    QUrl api(const QString &action = QString(),
             const QList<QPair<QString, QString>> &params = {}) const;

    QString liveUrl(int streamId, const QString &ext = QStringLiteral("ts")) const;
    QString vodUrl(int streamId, const QString &ext) const;
    QString seriesUrl(const QString &episodeId, const QString &ext) const;

    /// Normalize a raw user-entered server string (adds scheme, trims slashes).
    static QString normalizeBase(const QString &serverUrl);

private:
    QString streamUrl(const QString &kind, const QString &idAndExt) const;

    QString m_base;
    QString m_username;
    QString m_password;
};

} // namespace qiptv
