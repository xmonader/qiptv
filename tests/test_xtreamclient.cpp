#include "api/xtreamclient.h"

#include "core/useragent.h"

#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

using namespace kiptv;

// Minimal canned-response HTTP server. Routes by the "action" query param
// ("" == auth). Lets us drive XtreamClient end to end without a real provider.
// Not a QObject: QTcpServer signals are wired via lambdas with this as context.
class MockHttpServer {
public:
    QMap<QString, QByteArray> bodies; // action -> JSON body
    int status = 200;
    bool dropConnection = false;      // simulate a network failure
    QByteArray lastUserAgent;         // User-Agent header of the last request

    bool start()
    {
        const bool ok = m_server.listen(QHostAddress::LocalHost, 0);
        QObject::connect(&m_server, &QTcpServer::newConnection, [this] { onConnection(); });
        return ok;
    }

    QString baseUrl() const
    {
        return QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort());
    }

private:
    static QString actionOf(const QByteArray &request)
    {
        // First line: "GET /player_api.php?...&action=foo HTTP/1.1"
        const QByteArray line = request.left(request.indexOf('\r'));
        const int q = line.indexOf('?');
        if (q < 0) {
            return QString();
        }
        const QByteArray path = line.mid(q + 1);
        const QByteArray query = path.left(path.indexOf(' '));
        for (const QByteArray &kv : query.split('&')) {
            if (kv.startsWith("action=")) {
                return QString::fromUtf8(kv.mid(7));
            }
        }
        return QString(); // no action -> auth request
    }

    void onConnection()
    {
        QTcpSocket *sock = m_server.nextPendingConnection();
        QObject::connect(sock, &QTcpSocket::readyRead, [this, sock] {
            const QByteArray request = sock->readAll();
            if (!request.contains("\r\n\r\n")) {
                return; // wait for full headers
            }
            if (dropConnection) {
                sock->abort();
                sock->deleteLater();
                return;
            }
            const QByteArray headers = request.left(request.indexOf("\r\n\r\n"));
            const QList<QByteArray> lines = headers.split('\n');
            for (const QByteArray &line : lines) {
                if (line.left(11).compare("User-Agent:", Qt::CaseInsensitive) == 0) {
                    lastUserAgent = line.mid(line.indexOf(':') + 1).trimmed();
                }
            }
            const QByteArray body = bodies.value(actionOf(request));
            QByteArray resp = "HTTP/1.1 " + QByteArray::number(status) + " X\r\n";
            resp += "Content-Type: application/json\r\n";
            resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
            resp += "Connection: close\r\n\r\n";
            resp += body;
            sock->write(resp);
            sock->flush();
            sock->disconnectFromHost();
            sock->deleteLater();
        });
    }

    QTcpServer m_server;
};

class XtreamClientTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void authenticateSuccess()
    {
        MockHttpServer mock;
        mock.bodies[QString()] =
            R"({"user_info":{"username":"bob","auth":1,"status":"Active"},)"
            R"("server_info":{"url":"127.0.0.1","port":"80"}})";
        QVERIFY(mock.start());

        QNetworkAccessManager nam;
        XtreamClient client(&nam);
        client.setCredentials(mock.baseUrl(), "bob", "secret");

        ServerInfo got;
        bool ok = false;
        connect(&client, &XtreamClient::authenticated, [&](const ServerInfo &i) {
            got = i;
            ok = true;
        });
        client.authenticate();

        QTRY_VERIFY_WITH_TIMEOUT(ok, 3000);
        QVERIFY(got.authValid);
        QCOMPARE(got.user.username, QStringLiteral("bob"));
    }

    void authenticateRejected()
    {
        MockHttpServer mock;
        mock.bodies[QString()] = R"({"user_info":{"auth":0}})";
        QVERIFY(mock.start());

        QNetworkAccessManager nam;
        XtreamClient client(&nam);
        client.setCredentials(mock.baseUrl(), "bob", "wrong");

        QString err;
        connect(&client, &XtreamClient::errorOccurred, [&](const QString &m) { err = m; });
        client.authenticate();

        QTRY_VERIFY_WITH_TIMEOUT(!err.isEmpty(), 3000);
    }

    void fetchLiveStreamsParsesList()
    {
        MockHttpServer mock;
        mock.bodies[QStringLiteral("get_live_streams")] =
            R"([{"name":"CNN","stream_id":"1001","category_id":"5"}])";
        QVERIFY(mock.start());

        QNetworkAccessManager nam;
        XtreamClient client(&nam);
        client.setCredentials(mock.baseUrl(), "bob", "secret");

        QVector<LiveStream> got;
        bool ok = false;
        connect(&client, &XtreamClient::liveStreamsReady, [&](const QVector<LiveStream> &s) {
            got = s;
            ok = true;
        });
        client.fetchLiveStreams("5");

        QTRY_VERIFY_WITH_TIMEOUT(ok, 3000);
        QCOMPARE(got.size(), 1);
        QCOMPARE(got[0].streamId, 1001);
    }

    void httpErrorStatusEmitsError()
    {
        MockHttpServer mock;
        mock.status = 500;
        mock.bodies[QString()] = "oops";
        QVERIFY(mock.start());

        QNetworkAccessManager nam;
        XtreamClient client(&nam);
        client.setCredentials(mock.baseUrl(), "bob", "secret");

        QString err;
        connect(&client, &XtreamClient::errorOccurred, [&](const QString &m) { err = m; });
        client.authenticate();

        QTRY_VERIFY_WITH_TIMEOUT(!err.isEmpty(), 3000);
    }

    void userAgentHeaderSent()
    {
        // Providers (nginx front-ends) 403 requests without a User-Agent;
        // QNetworkAccessManager sends none unless we set one explicitly.
        MockHttpServer mock;
        mock.bodies[QString()] =
            R"({"user_info":{"username":"bob","auth":1,"status":"Active"},)"
            R"("server_info":{"url":"127.0.0.1","port":"80"}})";
        QVERIFY(mock.start());

        QNetworkAccessManager nam;
        XtreamClient client(&nam);
        client.setCredentials(mock.baseUrl(), "bob", "secret");

        bool ok = false;
        connect(&client, &XtreamClient::authenticated, [&](const ServerInfo &) { ok = true; });
        client.authenticate();

        QTRY_VERIFY_WITH_TIMEOUT(ok, 3000);
        QVERIFY2(!mock.lastUserAgent.isEmpty(), "request must carry a User-Agent header");
        QCOMPARE(mock.lastUserAgent, QByteArray(kUserAgent));
    }

    void noServerEmitsError()
    {
        XtreamClient client;
        QString err;
        connect(&client, &XtreamClient::errorOccurred, [&](const QString &m) { err = m; });
        client.authenticate(); // no credentials set
        QTRY_VERIFY_WITH_TIMEOUT(!err.isEmpty(), 1000);
    }
};

QTEST_MAIN(XtreamClientTest)
#include "test_xtreamclient.moc"
