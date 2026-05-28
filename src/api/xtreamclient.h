#pragma once

#include "api/urlbuilder.h"
#include "models/category.h"
#include "models/livestream.h"
#include "models/series.h"
#include "models/serverinfo.h"
#include "models/vodstream.h"

#include <QObject>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace kiptv {

/**
 * Async client for the Xtream Codes player_api.php.
 *
 * Each fetch* method issues one GET and emits a typed *Ready signal on
 * success or errorOccurred() on any failure (network, HTTP, empty body).
 * Requests carry a transfer timeout so a dead server surfaces as an error
 * rather than hanging the UI.
 *
 * The QNetworkAccessManager is injectable to allow testing against a mock
 * HTTP server; when not supplied the client owns a default one.
 */
class XtreamClient : public QObject {
    Q_OBJECT
public:
    explicit XtreamClient(QObject *parent = nullptr);
    XtreamClient(QNetworkAccessManager *nam, QObject *parent = nullptr);

    void setCredentials(const QString &serverUrl, const QString &username, const QString &password);
    const UrlBuilder &urls() const { return m_urls; }

    /// Request timeout in milliseconds (default 15000).
    void setTimeout(int ms) { m_timeoutMs = ms; }

    void authenticate();
    void fetchLiveCategories();
    void fetchVodCategories();
    void fetchSeriesCategories();
    void fetchLiveStreams(const QString &categoryId = QString());
    void fetchVodStreams(const QString &categoryId = QString());
    void fetchSeries(const QString &categoryId = QString());
    void fetchSeriesInfo(const QString &seriesId);
    void fetchVodInfo(const QString &vodId);

Q_SIGNALS:
    void authenticated(const kiptv::ServerInfo &info);
    void liveCategoriesReady(const QVector<kiptv::Category> &categories);
    void vodCategoriesReady(const QVector<kiptv::Category> &categories);
    void seriesCategoriesReady(const QVector<kiptv::Category> &categories);
    void liveStreamsReady(const QVector<kiptv::LiveStream> &streams);
    void vodStreamsReady(const QVector<kiptv::VodStream> &streams);
    void seriesReady(const QVector<kiptv::Series> &series);
    void seriesInfoReady(const kiptv::SeriesInfo &info);
    void vodInfoReady(const kiptv::VodInfo &info);
    void errorOccurred(const QString &message);

private:
    using Handler = std::function<void(const QByteArray &)>;
    void get(const QUrl &url, Handler onSuccess);

    QNetworkAccessManager *m_nam = nullptr;
    UrlBuilder m_urls;
    int m_timeoutMs = 15000;
};

} // namespace kiptv
