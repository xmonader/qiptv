#include "core/parse.h"

#include <QtTest>

using namespace qiptv;

// Exercises the coercion helpers against the messy shapes real Xtream
// servers emit: quoted numbers, bare numbers, nulls, bools, empties.
class ParseTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void str_data()
    {
        QTest::addColumn<QJsonValue>("in");
        QTest::addColumn<QString>("out");

        QTest::newRow("string") << QJsonValue(QStringLiteral("hello")) << QStringLiteral("hello");
        QTest::newRow("int-double") << QJsonValue(5) << QStringLiteral("5");
        QTest::newRow("real-double") << QJsonValue(5.5) << QStringLiteral("5.5");
        QTest::newRow("bool-true") << QJsonValue(true) << QStringLiteral("1");
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << QString();
        QTest::newRow("array") << QJsonValue(QJsonArray()) << QString();
    }
    void str()
    {
        QFETCH(QJsonValue, in);
        QFETCH(QString, out);
        QCOMPARE(parse::str(in), out);
    }

    void integer_data()
    {
        QTest::addColumn<QJsonValue>("in");
        QTest::addColumn<int>("out");

        QTest::newRow("bare-int") << QJsonValue(42) << 42;
        QTest::newRow("quoted-int") << QJsonValue(QStringLiteral("42")) << 42;
        QTest::newRow("quoted-float") << QJsonValue(QStringLiteral("42.0")) << 42;
        QTest::newRow("whitespace") << QJsonValue(QStringLiteral("  7 ")) << 7;
        QTest::newRow("empty-string") << QJsonValue(QStringLiteral("")) << 0;
        QTest::newRow("garbage") << QJsonValue(QStringLiteral("N/A")) << 0;
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << 0;
        QTest::newRow("bool") << QJsonValue(true) << 1;
    }
    void integer()
    {
        QFETCH(QJsonValue, in);
        QFETCH(int, out);
        QCOMPARE(parse::integer(in), out);
    }

    void number_data()
    {
        QTest::addColumn<QJsonValue>("in");
        QTest::addColumn<double>("out");

        QTest::newRow("bare") << QJsonValue(3.5) << 3.5;
        QTest::newRow("quoted") << QJsonValue(QStringLiteral("3.5")) << 3.5;
        QTest::newRow("rating-empty") << QJsonValue(QStringLiteral("")) << 0.0;
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << 0.0;
    }
    void number()
    {
        QFETCH(QJsonValue, in);
        QFETCH(double, out);
        QCOMPARE(parse::number(in), out);
    }

    void boolean_data()
    {
        QTest::addColumn<QJsonValue>("in");
        QTest::addColumn<bool>("out");

        QTest::newRow("true") << QJsonValue(true) << true;
        QTest::newRow("one-int") << QJsonValue(1) << true;
        QTest::newRow("one-str") << QJsonValue(QStringLiteral("1")) << true;
        QTest::newRow("zero-str") << QJsonValue(QStringLiteral("0")) << false;
        QTest::newRow("true-str") << QJsonValue(QStringLiteral("true")) << true;
        QTest::newRow("empty") << QJsonValue(QStringLiteral("")) << false;
    }
    void boolean()
    {
        QFETCH(QJsonValue, in);
        QFETCH(bool, out);
        QCOMPARE(parse::boolean(in), out);
    }

    void defaultsRespected()
    {
        QCOMPARE(parse::integer(QJsonValue(QJsonValue::Null), -1), -1);
        QCOMPARE(parse::str(QJsonValue(QJsonValue::Null), QStringLiteral("x")), QStringLiteral("x"));
        QCOMPARE(parse::boolean(QJsonValue(QStringLiteral("maybe")), true), true);
    }

    void integer64HandlesLargeTimestamps()
    {
        QCOMPARE(parse::integer64(QJsonValue(QStringLiteral("1700000000"))), Q_INT64_C(1700000000));
    }
};

QTEST_MAIN(ParseTest)
#include "test_parse.moc"
