#include "core/playlist.h"

#include <QTest>

using qiptv::Playlist;
using qiptv::PlaylistEntry;

namespace {
QVector<PlaylistEntry> entries(int n)
{
    QVector<PlaylistEntry> v;
    for (int i = 0; i < n; ++i) {
        v.push_back({QStringLiteral("http://h/%1").arg(i), QStringLiteral("T%1").arg(i)});
    }
    return v;
}
} // namespace

class TestPlaylist : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void defaultIsEmpty()
    {
        Playlist p;
        QVERIFY(p.isEmpty());
        QVERIFY(!p.canStep(1));
        QVERIFY(!p.step(-1));
    }

    void outOfRangeCurrentIsEmpty()
    {
        QVERIFY(Playlist(entries(3), 3, false).isEmpty());
        QVERIFY(Playlist(entries(3), -1, true).isEmpty());
        QVERIFY(Playlist({}, 0, true).isEmpty());
    }

    void nonWrappingStopsAtEnds()
    {
        Playlist p(entries(3), 1, false);
        QVERIFY(p.step(1));
        QCOMPARE(p.currentIndex(), 2);
        QCOMPARE(p.current().title, QStringLiteral("T2"));
        QVERIFY(!p.canStep(1));
        QVERIFY(!p.step(1));
        QCOMPARE(p.currentIndex(), 2);
        QVERIFY(p.step(-2));
        QCOMPARE(p.currentIndex(), 0);
        QVERIFY(!p.canStep(-1));
    }

    void wrappingCycles()
    {
        Playlist p(entries(3), 2, true);
        QVERIFY(p.step(1));
        QCOMPARE(p.currentIndex(), 0);
        QVERIFY(p.step(-1));
        QCOMPARE(p.currentIndex(), 2);
        QVERIFY(p.step(-7)); // large negative delta still lands in range
        QCOMPARE(p.currentIndex(), 1);
    }

    void singleEntryNeverSteps()
    {
        Playlist wrap(entries(1), 0, true);
        QVERIFY(!wrap.canStep(1));
        QVERIFY(!wrap.canStep(-1));
        Playlist flat(entries(1), 0, false);
        QVERIFY(!flat.step(1));
    }

    void autoAdvanceDefaultsOff()
    {
        QVERIFY(!Playlist(entries(2), 0, false).autoAdvance());
        QVERIFY(Playlist(entries(2), 0, false, true).autoAdvance());
    }

    void wrapByFullCycleIsNotAStep()
    {
        Playlist p(entries(3), 0, true);
        QVERIFY(!p.canStep(3)); // would land on the same entry
        QVERIFY(!p.canStep(0));
    }
};

QTEST_GUILESS_MAIN(TestPlaylist)
#include "test_playlist.moc"
