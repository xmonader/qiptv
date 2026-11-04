#include "core/imageloader.h"

#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace qiptv {

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
    // QCache owns the copy (implicitly shared, so no pixel copy) and evicts
    // least-recently-used entries once the byte budget is exceeded. An image
    // larger than the whole budget is simply not cached.
    m_cache.insert(url, new QImage(image), costFor(image.sizeInBytes()));
}

QImage ImageLoader::decode(const QByteArray &bytes) const
{
    QBuffer buffer;
    buffer.setData(bytes);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return {};
    }
    QImageReader reader(&buffer);
    const bool bounded = m_maxImageSize.isValid();
    const QSize source = reader.size();
    // Ask the decoder to scale while decoding (JPEG can skip most of the work)
    // so the full-size bitmap never exists.
    if (bounded && source.isValid()
        && (source.width() > m_maxImageSize.width() || source.height() > m_maxImageSize.height())) {
        reader.setScaledSize(source.scaled(m_maxImageSize, Qt::KeepAspectRatio));
    }
    QImage image = reader.read();
    // Formats that can't report their size up front: scale after decoding.
    if (bounded && !image.isNull()
        && (image.width() > m_maxImageSize.width() || image.height() > m_maxImageSize.height())) {
        image = image.scaled(m_maxImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image;
}

void ImageLoader::load(const QString &url)
{
    if (url.isEmpty()) {
        Q_EMIT failed(url);
        return;
    }
    if (const QImage *cached = m_cache.object(url)) { // also marks it recently used
        Q_EMIT loaded(url, *cached);
        return;
    }
    if (m_inFlight.contains(url)) {
        return; // already requested or queued; its result will be emitted
    }
    m_inFlight.insert(url);

    if (m_active < m_maxConcurrent) {
        startRequest(url);
    } else {
        m_queue.enqueue(url);
    }
}

void ImageLoader::startRequest(const QString &url)
{
    ++m_active;

    QNetworkRequest req((QUrl(url)));
    req.setTransferTimeout(m_timeoutMs);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    // These provider CDNs handle many multiplexed HTTP/2 streams badly; force
    // HTTP/1.1 so the concurrency cap actually governs the connection count.
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        m_inFlight.remove(url);
        --m_active;

        if (reply->error() == QNetworkReply::NoError) {
            const QImage image = decode(reply->readAll());
            if (!image.isNull()) {
                cacheInsert(url, image);
                Q_EMIT loaded(url, image);
            } else {
                Q_EMIT failed(url);
            }
        } else {
            Q_EMIT failed(url);
        }
        pumpQueue();
    });
}

void ImageLoader::pumpQueue()
{
    while (m_active < m_maxConcurrent && !m_queue.isEmpty()) {
        startRequest(m_queue.dequeue());
    }
}

} // namespace qiptv
