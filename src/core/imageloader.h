#pragma once

#include <QCache>
#include <QImage>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QSize>

class QNetworkAccessManager;

namespace qiptv {

/**
 * Async image fetcher with an in-memory cache and in-flight de-duplication.
 *
 * load(url) emits loaded(url, image) once the bytes arrive (or immediately
 * from cache), or failed(url) on error. Multiple load() calls for the same
 * URL while a request is in flight share that single request. Emits QImage
 * (not QPixmap) so it stays usable without a running GUI, which also makes
 * it testable headless.
 *
 * Requests are capped at a small concurrency limit with the rest queued: an
 * IPTV provider serving a category of hundreds of posters will refuse a flood
 * of simultaneous connections ("excessive load detected") and return truncated
 * images. HTTP/2 is disabled for the same reason — these CDNs multiplex poorly.
 *
 * Images are decoded no larger than maxImageSize() (aspect ratio kept, never
 * upscaled). Provider posters are often 600x900 up to multi-megapixel
 * originals; holding them decoded at full size cost gigabytes of RAM for a
 * grid that only shows small thumbnails.
 */
class ImageLoader : public QObject {
    Q_OBJECT
public:
    explicit ImageLoader(QObject *parent = nullptr);
    ImageLoader(QNetworkAccessManager *nam, QObject *parent = nullptr);

    /// Cap on decoded bytes held by the cache (least recently used evicted).
    void setCacheLimitBytes(qint64 bytes) { m_cache.setMaxCost(costFor(bytes)); }
    /// Decoded bytes currently held by the cache (KiB granularity).
    qint64 cachedBytes() const { return qint64(m_cache.totalCost()) * 1024; }
    /// Decoded images are downscaled to fit within this size (aspect kept,
    /// never upscaled). An invalid size disables the bound.
    void setMaxImageSize(const QSize &size) { m_maxImageSize = size; }
    QSize maxImageSize() const { return m_maxImageSize; }
    /// Maximum simultaneous network requests; the rest queue.
    void setMaxConcurrent(int n) { m_maxConcurrent = n; }
    bool isCached(const QString &url) const { return m_cache.contains(url); }

    void load(const QString &url);

Q_SIGNALS:
    void loaded(const QString &url, const QImage &image);
    void failed(const QString &url);

private:
    static qsizetype costFor(qint64 bytes) { return qsizetype((bytes + 1023) / 1024); }
    void cacheInsert(const QString &url, const QImage &image);
    QImage decode(const QByteArray &bytes) const;
    void startRequest(const QString &url);
    void pumpQueue();

    QNetworkAccessManager *m_nam = nullptr;
    QCache<QString, QImage> m_cache{costFor(64 * 1024 * 1024)}; // cost = KiB
    QSet<QString> m_inFlight;   // requested or queued (for dedup)
    QQueue<QString> m_queue;    // waiting for a free slot
    int m_active = 0;           // requests currently in flight
    int m_maxConcurrent = 6;
    int m_timeoutMs = 15000;
    QSize m_maxImageSize{320, 480}; // 2x the largest on-screen poster (160x240)
};

} // namespace qiptv
