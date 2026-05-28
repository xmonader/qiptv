#include "api/xtreamclient.h"

#include <KLocalizedString>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace kiptv {

namespace {
void registerMetaTypes()
{
    static bool done = false;
    if (done) {
        return;
    }
    done = true;
    qRegisterMetaType<ServerInfo>("kiptv::ServerInfo");
    qRegisterMetaType<SeriesInfo>("kiptv::SeriesInfo");
    qRegisterMetaType<VodInfo>("kiptv::VodInfo");
    qRegisterMetaType<QVector<Category>>("QVector<kiptv::Category>");
    qRegisterMetaType<QVector<LiveStream>>("QVector<kiptv::LiveStream>");
    qRegisterMetaType<QVector<VodStream>>("QVector<kiptv::VodStream>");
    qRegisterMetaType<QVector<Series>>("QVector<kiptv::Series>");
}
} // namespace

XtreamClient::XtreamClient(QObject *parent)
    : XtreamClient(nullptr, parent)
{
}

XtreamClient::XtreamClient(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , m_nam(nam ? nam : new QNetworkAccessManager(this))
{
    registerMetaTypes();
}

void XtreamClient::setCredentials(const QString &serverUrl, const QString &username, const QString &password)
{
    m_urls = UrlBuilder(serverUrl, username, password);
}

void XtreamClient::get(const QUrl &url, Handler onSuccess)
{
    if (!m_urls.isValid()) {
        Q_EMIT errorOccurred(i18n("No server configured."));
        return;
    }

    QNetworkRequest req(url);
    req.setTransferTimeout(m_timeoutMs);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT errorOccurred(reply->errorString());
            return;
        }

        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 0 && (status < 200 || status >= 300)) {
            Q_EMIT errorOccurred(i18n("Server returned HTTP %1.", status));
            return;
        }

        const QByteArray body = reply->readAll();
        if (body.isEmpty()) {
            Q_EMIT errorOccurred(i18n("Empty response from server."));
            return;
        }
        onSuccess(body);
    });
}

void XtreamClient::authenticate()
{
    get(m_urls.api(), [this](const QByteArray &body) {
        const ServerInfo info = ServerInfo::fromJson(body);
        if (!info.authValid) {
            Q_EMIT errorOccurred(i18n("Authentication failed. Check your credentials."));
            return;
        }
        Q_EMIT authenticated(info);
    });
}

void XtreamClient::fetchLiveCategories()
{
    get(m_urls.api(QStringLiteral("get_live_categories")), [this](const QByteArray &b) {
        Q_EMIT liveCategoriesReady(categoriesFromJson(b));
    });
}

void XtreamClient::fetchVodCategories()
{
    get(m_urls.api(QStringLiteral("get_vod_categories")), [this](const QByteArray &b) {
        Q_EMIT vodCategoriesReady(categoriesFromJson(b));
    });
}

void XtreamClient::fetchSeriesCategories()
{
    get(m_urls.api(QStringLiteral("get_series_categories")), [this](const QByteArray &b) {
        Q_EMIT seriesCategoriesReady(categoriesFromJson(b));
    });
}

void XtreamClient::fetchLiveStreams(const QString &categoryId)
{
    QList<QPair<QString, QString>> params;
    if (!categoryId.isEmpty()) {
        params.append({QStringLiteral("category_id"), categoryId});
    }
    get(m_urls.api(QStringLiteral("get_live_streams"), params), [this](const QByteArray &b) {
        Q_EMIT liveStreamsReady(liveStreamsFromJson(b));
    });
}

void XtreamClient::fetchVodStreams(const QString &categoryId)
{
    QList<QPair<QString, QString>> params;
    if (!categoryId.isEmpty()) {
        params.append({QStringLiteral("category_id"), categoryId});
    }
    get(m_urls.api(QStringLiteral("get_vod_streams"), params), [this](const QByteArray &b) {
        Q_EMIT vodStreamsReady(vodStreamsFromJson(b));
    });
}

void XtreamClient::fetchSeries(const QString &categoryId)
{
    QList<QPair<QString, QString>> params;
    if (!categoryId.isEmpty()) {
        params.append({QStringLiteral("category_id"), categoryId});
    }
    get(m_urls.api(QStringLiteral("get_series"), params), [this](const QByteArray &b) {
        Q_EMIT seriesReady(seriesListFromJson(b));
    });
}

void XtreamClient::fetchSeriesInfo(const QString &seriesId)
{
    get(m_urls.api(QStringLiteral("get_series_info"), {{QStringLiteral("series_id"), seriesId}}),
        [this](const QByteArray &b) {
            Q_EMIT seriesInfoReady(SeriesInfo::fromJson(b));
        });
}

void XtreamClient::fetchVodInfo(const QString &vodId)
{
    get(m_urls.api(QStringLiteral("get_vod_info"), {{QStringLiteral("vod_id"), vodId}}),
        [this](const QByteArray &b) {
            Q_EMIT vodInfoReady(VodInfo::fromJson(b));
        });
}

} // namespace kiptv
