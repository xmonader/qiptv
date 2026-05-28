#pragma once

#include <QHash>
#include <QImage>
#include <QObject>
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
 */
class ImageLoader : public QObject {
    Q_OBJECT
public:
    explicit ImageLoader(QObject *parent = nullptr);
    ImageLoader(QNetworkAccessManager *nam, QObject *parent = nullptr);

    /// Cap on cached entries (LRU-ish: oldest cleared when exceeded).
    void setCacheLimit(int entries) { m_cacheLimit = entries; }
    bool isCached(const QString &url) const { return m_cache.contains(url); }

    void load(const QString &url);

Q_SIGNALS:
    void loaded(const QString &url, const QImage &image);
    void failed(const QString &url);

private:
    void cacheInsert(const QString &url, const QImage &image);

    QNetworkAccessManager *m_nam = nullptr;
    QHash<QString, QImage> m_cache;
    QSet<QString> m_inFlight;
    int m_cacheLimit = 512;
    int m_timeoutMs = 15000;
};

} // namespace kiptv
