#include "core/imageloader.h"

#include <QBuffer>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

using namespace kiptv;

// Serves a fixed body for any request. Counts requests so we can prove the
// cache prevents a second network hit.
class ImageServer {
public:
    QByteArray body;
    int requestCount = 0;

    bool start()
    {
        const bool ok = m_server.listen(QHostAddress::LocalHost, 0);
        QObject::connect(&m_server, &QTcpServer::newConnection, [this] {
            QTcpSocket *s = m_server.nextPendingConnection();
            QObject::connect(s, &QTcpSocket::readyRead, [this, s] {
                const QByteArray req = s->readAll();
                if (!req.contains("\r\n\r\n")) {
                    return;
                }
                ++requestCount;
                QByteArray resp = "HTTP/1.1 200 OK\r\n";
                resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
                resp += "Connection: close\r\n\r\n";
                resp += body;
                s->write(resp);
                s->flush();
                s->disconnectFromHost();
                s->deleteLater();
            });
        });
        return ok;
    }

    QString url() const
    {
        return QStringLiteral("http://127.0.0.1:%1/poster.png").arg(m_server.serverPort());
    }

private:
    QTcpServer m_server;
};

static QByteArray makePng()
{
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::red);
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return bytes;
}

class ImageLoaderTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void loadsAndCaches()
    {
        ImageServer server;
        server.body = makePng();
        QVERIFY(!server.body.isEmpty());
        QVERIFY(server.start());

        QNetworkAccessManager nam;
        ImageLoader loader(&nam);

        QImage got;
        int loadedCount = 0;
        connect(&loader, &ImageLoader::loaded, [&](const QString &, const QImage &img) {
            got = img;
            ++loadedCount;
        });

        loader.load(server.url());
        QTRY_VERIFY_WITH_TIMEOUT(loadedCount == 1, 3000);
        QVERIFY(!got.isNull());
        QCOMPARE(got.width(), 4);
        QVERIFY(loader.isCached(server.url()));

        // Second load is a cache hit: emits again, no new network request.
        loader.load(server.url());
        QTRY_VERIFY_WITH_TIMEOUT(loadedCount == 2, 1000);
        QCOMPARE(server.requestCount, 1);
    }

    void emptyUrlFails()
    {
        ImageLoader loader;
        QString failedUrl = "sentinel";
        connect(&loader, &ImageLoader::failed, [&](const QString &u) { failedUrl = u; });
        loader.load(QString());
        QTRY_COMPARE_WITH_TIMEOUT(failedUrl, QString(), 1000);
    }

    void queueDrainsBeyondConcurrencyCap()
    {
        ImageServer server;
        server.body = makePng();
        QVERIFY(server.start());

        QNetworkAccessManager nam;
        ImageLoader loader(&nam);
        loader.setMaxConcurrent(2);

        int loadedCount = 0;
        connect(&loader, &ImageLoader::loaded, [&](const QString &, const QImage &) { ++loadedCount; });

        // 6 distinct URLs with a cap of 2 — all must still complete.
        for (int i = 0; i < 6; ++i) {
            loader.load(server.url() + QStringLiteral("?i=%1").arg(i));
        }
        QTRY_VERIFY_WITH_TIMEOUT(loadedCount == 6, 4000);
        QCOMPARE(server.requestCount, 6);
    }

    void nonImageDataFails()
    {
        ImageServer server;
        server.body = "this is not an image";
        QVERIFY(server.start());

        QNetworkAccessManager nam;
        ImageLoader loader(&nam);
        bool failed = false;
        connect(&loader, &ImageLoader::failed, [&](const QString &) { failed = true; });
        loader.load(server.url());
        QTRY_VERIFY_WITH_TIMEOUT(failed, 3000);
        QVERIFY(!loader.isCached(server.url()));
    }
};

QTEST_MAIN(ImageLoaderTest)
#include "test_imageloader.moc"
