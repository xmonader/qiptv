#pragma once

#include <QHash>
#include <QImage>
#include <QObject>
#include <QQueue>
#include <QSet>

class QNetworkAccessManager;

namespace kiptv {

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
 */
class ImageLoader : public QObject {
    Q_OBJECT
public:
    explicit ImageLoader(QObject *parent = nullptr);
    ImageLoader(QNetworkAccessManager *nam, QObject *parent = nullptr);

    /// Cap on cached entries (LRU-ish: oldest cleared when exceeded).
    void setCacheLimit(int entries) { m_cacheLimit = entries; }
    /// Maximum simultaneous network requests; the rest queue.
    void setMaxConcurrent(int n) { m_maxConcurrent = n; }
    bool isCached(const QString &url) const { return m_cache.contains(url); }

    void load(const QString &url);

Q_SIGNALS:
    void loaded(const QString &url, const QImage &image);
    void failed(const QString &url);

private:
    void cacheInsert(const QString &url, const QImage &image);
    void startRequest(const QString &url);
    void pumpQueue();

    QNetworkAccessManager *m_nam = nullptr;
    QHash<QString, QImage> m_cache;
    QSet<QString> m_inFlight;   // requested or queued (for dedup)
    QQueue<QString> m_queue;    // waiting for a free slot
    int m_active = 0;           // requests currently in flight
    int m_maxConcurrent = 6;
    int m_cacheLimit = 512;
    int m_timeoutMs = 15000;
};

} // namespace kiptv
