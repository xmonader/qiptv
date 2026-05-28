#include "models/category.h"
#include "models/serverinfo.h"

#include <QtTest>

using namespace kiptv;

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
};

QTEST_MAIN(ModelsTest)
#include "test_models.moc"
