#include "config/profilestore.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace qiptv;

// Exercises persistence round-trips against a throwaway config file.
class ProfileStoreTest : public QObject {
    Q_OBJECT
private:
    QTemporaryDir m_dir;
    // KSharedConfig caches by path, so give every test its own file to keep
    // them isolated.
    QString cfg() const
    {
        return m_dir.path() + "/" + QString::fromLatin1(QTest::currentTestFunction()) + ".rc";
    }

private Q_SLOTS:
    void upsertGeneratesIdAndRoundTrips()
    {
        ProfileStore store(cfg());
        Profile p;
        p.name = "Home";
        p.serverUrl = "http://x.tv:8080";
        p.username = "bob";
        p.password = "s3cret";
        const QString id = store.upsert(p);
        QVERIFY(!id.isEmpty());
        store.save();

        // Re-open from disk: data must survive.
        ProfileStore reopened(cfg());
        const auto all = reopened.profiles();
        QCOMPARE(all.size(), 1);
        QCOMPARE(all[0].id, id);
        QCOMPARE(all[0].name, QStringLiteral("Home"));
        QCOMPARE(all[0].username, QStringLiteral("bob"));
        QCOMPARE(all[0].password, QStringLiteral("s3cret")); // de-obfuscated
    }

    void passwordNotStoredInPlaintext()
    {
        ProfileStore store(cfg());
        Profile p;
        p.serverUrl = "http://x.tv";
        p.username = "u";
        p.password = "PLAINTEXTMARKER";
        store.upsert(p);
        store.save();

        QFile f(cfg());
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray contents = f.readAll();
        QVERIFY(!contents.contains("PLAINTEXTMARKER"));
    }

    void upsertUpdatesExisting()
    {
        ProfileStore store(cfg());
        Profile p;
        p.name = "A";
        p.serverUrl = "http://x.tv";
        p.username = "u";
        const QString id = store.upsert(p);

        p.id = id;
        p.name = "A-renamed";
        store.upsert(p);

        QCOMPARE(store.profiles().size(), 1); // no duplicate
        QCOMPARE(store.profile(id).name, QStringLiteral("A-renamed"));
    }

    void removeDropsProfileAndClearsLastUsed()
    {
        ProfileStore store(cfg());
        Profile p;
        p.serverUrl = "http://x.tv";
        p.username = "u";
        const QString id = store.upsert(p);
        store.setLastUsedId(id);

        store.remove(id);
        QCOMPARE(store.profiles().size(), 0);
        QVERIFY(store.lastUsedId().isEmpty());
        QVERIFY(!store.profile(id).isValid());
    }

    void orderPreserved()
    {
        ProfileStore store(cfg());
        QStringList names{"one", "two", "three"};
        for (const QString &n : names) {
            Profile p;
            p.name = n;
            p.serverUrl = "http://x.tv";
            p.username = "u";
            store.upsert(p);
        }
        const auto all = store.profiles();
        QCOMPARE(all.size(), 3);
        QCOMPARE(all[0].name, QStringLiteral("one"));
        QCOMPARE(all[2].name, QStringLiteral("three"));
    }

    void lastUsedRoundTrips()
    {
        ProfileStore store(cfg());
        store.setLastUsedId("abc");
        store.save();
        ProfileStore reopened(cfg());
        QCOMPARE(reopened.lastUsedId(), QStringLiteral("abc"));
    }
};

QTEST_MAIN(ProfileStoreTest)
#include "test_profilestore.moc"
