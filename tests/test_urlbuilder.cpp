#include "api/urlbuilder.h"

#include <QtTest>

using namespace kiptv;

// Normal "" string literals are used here (not R"(...)") so that http:// URLs
// don't trip the moc raw-string '//' lexer quirk documented in test_models.
class UrlBuilderTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void normalizeAddsSchemeAndTrimsSlashes()
    {
        QCOMPARE(UrlBuilder::normalizeBase("example.com:8080"),
                 QString("http://example.com:8080"));
        QCOMPARE(UrlBuilder::normalizeBase("http://example.com:8080/"),
                 QString("http://example.com:8080"));
        QCOMPARE(UrlBuilder::normalizeBase("https://x.tv///"),
                 QString("https://x.tv"));
        QCOMPARE(UrlBuilder::normalizeBase("  http://x.tv  "),
                 QString("http://x.tv"));
        QCOMPARE(UrlBuilder::normalizeBase(""), QString());
    }

    void apiAuthUrl()
    {
        UrlBuilder b("http://x.tv:8080", "bob", "secret");
        const QUrl u = b.api();
        QCOMPARE(u.path(), QString("/player_api.php"));
        QUrlQuery q(u);
        QCOMPARE(q.queryItemValue("username"), QString("bob"));
        QCOMPARE(q.queryItemValue("password"), QString("secret"));
        QVERIFY(!q.hasQueryItem("action"));
    }

    void apiActionWithParams()
    {
        UrlBuilder b("http://x.tv:8080", "bob", "secret");
        const QUrl u = b.api("get_series_info", {{"series_id", "42"}});
        QUrlQuery q(u);
        QCOMPARE(q.queryItemValue("action"), QString("get_series_info"));
        QCOMPARE(q.queryItemValue("series_id"), QString("42"));
    }

    void apiEncodesSpecialChars()
    {
        UrlBuilder b("http://x.tv", "us er", "p@ss&w=rd");
        const QUrl u = b.api("get_live_streams");
        // Round-trip through QUrlQuery must recover the exact credentials.
        QUrlQuery q(u);
        QCOMPARE(q.queryItemValue("username", QUrl::FullyDecoded), QString("us er"));
        QCOMPARE(q.queryItemValue("password", QUrl::FullyDecoded), QString("p@ss&w=rd"));
    }

    void streamUrls()
    {
        UrlBuilder b("http://x.tv:8080", "bob", "secret");
        QCOMPARE(b.liveUrl(1001),
                 QString("http://x.tv:8080/live/bob/secret/1001.ts"));
        QCOMPARE(b.vodUrl(2001, "mkv"),
                 QString("http://x.tv:8080/movie/bob/secret/2001.mkv"));
        QCOMPARE(b.seriesUrl("3001", "mp4"),
                 QString("http://x.tv:8080/series/bob/secret/3001.mp4"));
    }

    void streamUrlsDefaultExtension()
    {
        UrlBuilder b("http://x.tv", "bob", "secret");
        QCOMPARE(b.vodUrl(2001, ""),
                 QString("http://x.tv/movie/bob/secret/2001.mp4"));
        QCOMPARE(b.seriesUrl("3001", ""),
                 QString("http://x.tv/series/bob/secret/3001.mp4"));
    }

    void validity()
    {
        QVERIFY(!UrlBuilder().isValid());
        QVERIFY(!UrlBuilder("", "bob", "p").isValid());
        QVERIFY(!UrlBuilder("http://x.tv", "", "p").isValid());
        QVERIFY(UrlBuilder("http://x.tv", "bob", "p").isValid());
    }
};

QTEST_MAIN(UrlBuilderTest)
#include "test_urlbuilder.moc"
