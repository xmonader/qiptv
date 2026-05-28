#include "config/settings.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace kiptv;

class SettingsTest : public QObject {
    Q_OBJECT
private:
    QTemporaryDir m_dir;
    QString cfg() const
    {
        return m_dir.path() + "/" + QString::fromLatin1(QTest::currentTestFunction()) + ".rc";
    }

private Q_SLOTS:
    void defaults()
    {
        Settings s(cfg());
        QCOMPARE(s.liveContainer(), QStringLiteral("ts"));
        QCOMPARE(s.minimizeToTray(), false);
        QCOMPARE(s.networkCaching(), true);
        QCOMPARE(s.lastVolume(), 100);
    }

    void roundTrip()
    {
        {
            Settings s(cfg());
            s.setLiveContainer("m3u8");
            s.setMinimizeToTray(true);
            s.setNetworkCaching(false);
            s.setLastVolume(80);
            s.save();
        }
        Settings reopened(cfg());
        QCOMPARE(reopened.liveContainer(), QStringLiteral("m3u8"));
        QCOMPARE(reopened.minimizeToTray(), true);
        QCOMPARE(reopened.networkCaching(), false);
        QCOMPARE(reopened.lastVolume(), 80);
    }

    void volumeClamped()
    {
        Settings s(cfg());
        s.setLastVolume(999);
        QCOMPARE(s.lastVolume(), 150);
        s.setLastVolume(-5);
        QCOMPARE(s.lastVolume(), 0);
    }
};

QTEST_MAIN(SettingsTest)
#include "test_settings.moc"
