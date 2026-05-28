#include "core/imageloader.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace kiptv {

ImageLoader::ImageLoader(QObject *parent)
    : ImageLoader(nullptr, parent)
{
}

ImageLoader::ImageLoader(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , m_nam(nam ? nam : new QNetworkAccessManager(this))
{
}

void ImageLoader::cacheInsert(const QString &url, const QImage &image)
{
    if (m_cacheLimit > 0 && m_cache.size() >= m_cacheLimit && !m_cache.contains(url)) {
        // Cheap eviction: drop an arbitrary existing entry. Good enough for a
        // poster cache; a strict LRU isn't worth the bookkeeping here.
        m_cache.erase(m_cache.begin());
    }
    m_cache.insert(url, image);
}

void ImageLoader::load(const QString &url)
{
    if (url.isEmpty()) {
        Q_EMIT failed(url);
        return;
    }
    const auto it = m_cache.constFind(url);
    if (it != m_cache.constEnd()) {
        Q_EMIT loaded(url, it.value());
        return;
    }
    if (m_inFlight.contains(url)) {
        return; // a request is already running; its result will be cached/emitted
    }
    m_inFlight.insert(url);

    QNetworkRequest req((QUrl(url)));
    req.setTransferTimeout(m_timeoutMs);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        m_inFlight.remove(url);

        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT failed(url);
            return;
        }
        QImage image;
        if (!image.loadFromData(reply->readAll())) {
            Q_EMIT failed(url);
            return;
        }
        cacheInsert(url, image);
        Q_EMIT loaded(url, image);
    });
}

} // namespace kiptv
