#include "models/category.h"
#include "models/livestream.h"
#include "models/serverinfo.h"
#include "models/vodstream.h"

#include <QtTest>

using namespace kiptv;

// NOTE: do not put "//" inside R"(...)" raw strings here. This Qt moc version
// mis-lexes "//" within a raw string as a line comment and silently drops the
// class ("No relevant classes found"). Fixture URLs are therefore scheme-less
// (e.g. "cdn/cnn.png"); the coercion tests only check the string round-trips.

// Parses realistic Xtream response bodies, including the string/int field
// inconsistencies and empty-response edge cases servers actually return.
class ModelsTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void categoriesParse()
    {
        const QByteArray json = R"([
            {"category_id":"1","category_name":"News","parent_id":0},
            {"category_id":2,"category_name":"Sports","parent_id":"0"}
        ])";
        const auto cats = categoriesFromJson(json);
        QCOMPARE(cats.size(), 2);
        QCOMPARE(cats[0].id, QStringLiteral("1"));
        QCOMPARE(cats[0].name, QStringLiteral("News"));
        QCOMPARE(cats[1].id, QStringLiteral("2")); // numeric id coerced
        QCOMPARE(cats[1].parentId, QStringLiteral("0"));
    }

    void categoriesEmptyObjectIsSafe()
    {
        // Some servers return {} instead of [] when a section is empty.
        QCOMPARE(categoriesFromJson("{}").size(), 0);
        QCOMPARE(categoriesFromJson("garbage").size(), 0);
        QCOMPARE(categoriesFromJson("").size(), 0);
    }

    void authSuccess()
    {
        const QByteArray json = R"({
            "user_info":{"username":"bob","password":"x","status":"Active",
                "auth":1,"is_trial":"0","active_cons":"1","max_connections":"2",
                "exp_date":"1700000000","allowed_output_formats":["m3u8","ts"]},
            "server_info":{"url":"example.com","port":"8080","https_port":"443",
                "server_protocol":"http","timezone":"UTC"}
        })";
        const ServerInfo info = ServerInfo::fromJson(json);
        QVERIFY(info.authValid);
        QCOMPARE(info.user.username, QStringLiteral("bob"));
        QVERIFY(info.user.isActive());
        QCOMPARE(info.user.maxConnections, 2);
        QCOMPARE(info.user.allowedOutputFormats.size(), 2);
        QCOMPARE(info.server.port, QStringLiteral("8080"));
    }

    void authFailureWhenAuthZero()
    {
        const QByteArray json = R"({"user_info":{"auth":0}})";
        const ServerInfo info = ServerInfo::fromJson(json);
        QVERIFY(!info.authValid);
    }

    void authFailureOnEmptyBody()
    {
        QVERIFY(!ServerInfo::fromJson("").authValid);
        QVERIFY(!ServerInfo::fromJson("[]").authValid);
    }

    void liveStreamsParse()
    {
        const QByteArray json = R"([
            {"num":1,"name":"CNN","stream_id":"1001","stream_icon":"cdn/cnn.png",
             "epg_channel_id":"cnn.us","category_id":"5","tv_archive":1,"added":"1700000000"},
            {"num":"2","name":"BBC","stream_id":1002,"category_id":5,"tv_archive":"0"}
        ])";
        const auto live = liveStreamsFromJson(json);
        QCOMPARE(live.size(), 2);
        QCOMPARE(live[0].streamId, 1001);
        QCOMPARE(live[0].name, QStringLiteral("CNN"));
        QVERIFY(live[0].tvArchive);
        QCOMPARE(live[0].added, Q_INT64_C(1700000000));
        QCOMPARE(live[1].num, 2);          // quoted num coerced
        QVERIFY(!live[1].tvArchive);
    }

    void vodStreamsParse()
    {
        const QByteArray json = R"([
            {"num":1,"name":"Movie A","stream_id":"2001","cover":"cdn/a.jpg",
             "category_id":"9","container_extension":"mkv","rating":"7.8"}
        ])";
        const auto vod = vodStreamsFromJson(json);
        QCOMPARE(vod.size(), 1);
        QCOMPARE(vod[0].streamId, 2001);
        QCOMPARE(vod[0].icon, QStringLiteral("cdn/a.jpg")); // falls back to cover
        QCOMPARE(vod[0].containerExtension, QStringLiteral("mkv"));
        QCOMPARE(vod[0].rating, 7.8);
    }

    void vodInfoParse()
    {
        const QByteArray json = R"({
            "info":{"plot":"A story","cast":"X, Y","director":"Z","genre":"Drama",
                "releasedate":"2020-01-01","duration":"01:42:00","rating":"8.1",
                "cover_big":"cdn/big.jpg","backdrop_path":["cdn/bd.jpg"],
                "youtube_trailer":"abc123"},
            "movie_data":{"stream_id":"2001","name":"Movie A","container_extension":"mp4"}
        })";
        const VodInfo v = VodInfo::fromJson(json);
        QCOMPARE(v.streamId, 2001);
        QCOMPARE(v.name, QStringLiteral("Movie A"));
        QCOMPARE(v.containerExtension, QStringLiteral("mp4"));
        QCOMPARE(v.plot, QStringLiteral("A story"));
        QCOMPARE(v.rating, 8.1);
        QCOMPARE(v.backdropPath, QStringLiteral("cdn/bd.jpg")); // array -> first
    }
};

QTEST_MAIN(ModelsTest)
#include "test_models.moc"
