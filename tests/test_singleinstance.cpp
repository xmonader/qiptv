#include "core/singleinstance.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace qiptv;

class SingleInstanceTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void secondInstanceActivatesFirst()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        SingleInstance first(dir.path(), QStringLiteral("qiptv-test"));
        QVERIFY(first.tryBecomePrimary());
        int activations = 0;
        connect(&first, &SingleInstance::activationRequested, [&] { ++activations; });

        SingleInstance second(dir.path(), QStringLiteral("qiptv-test"));
        QVERIFY(!second.tryBecomePrimary());
        QTRY_COMPARE_WITH_TIMEOUT(activations, 1, 3000);
    }

    void lockReleasedWhenPrimaryExits()
    {
        QTemporaryDir dir;
        {
            SingleInstance first(dir.path(), QStringLiteral("k"));
            QVERIFY(first.tryBecomePrimary());
        }
        SingleInstance next(dir.path(), QStringLiteral("k"));
        QVERIFY(next.tryBecomePrimary());
    }

    void staleSocketFileDoesNotBlockPrimary()
    {
        QTemporaryDir dir;
        SingleInstance probe(dir.path(), QStringLiteral("k"));
        QFile stale(probe.socketPath()); // leftover from a crashed instance
        QVERIFY(stale.open(QIODevice::WriteOnly));
        stale.close();

        QVERIFY(probe.tryBecomePrimary());
        int activations = 0;
        connect(&probe, &SingleInstance::activationRequested, [&] { ++activations; });
        SingleInstance second(dir.path(), QStringLiteral("k"));
        QVERIFY(!second.tryBecomePrimary());
        QTRY_COMPARE_WITH_TIMEOUT(activations, 1, 3000);
    }
};

QTEST_MAIN(SingleInstanceTest)
#include "test_singleinstance.moc"
