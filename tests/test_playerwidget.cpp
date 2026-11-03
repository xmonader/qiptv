#include "player/mpvwidget.h"
#include "player/playerwidget.h"

#include <QApplication>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>
#include <QToolButton>
#include <QWheelEvent>

using qiptv::MpvWidget;
using qiptv::PlayerWidget;

class TestPlayerWidget : public QObject {
    Q_OBJECT

    static QSlider *volume(PlayerWidget &p) { return p.findChild<QSlider *>("volume"); }
    static QToolButton *button(PlayerWidget &p, const char *name)
    {
        return p.findChild<QToolButton *>(QString::fromLatin1(name));
    }
    static void wheel(PlayerWidget &p, int deltaY)
    {
        const QPointF pos(10, 10);
        QWheelEvent ev(pos, p.mapToGlobal(pos), QPoint(), QPoint(0, deltaY), Qt::NoButton,
                       Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&p, &ev);
    }

private Q_SLOTS:
    void controlsNeverStealKeyboardFocus()
    {
        PlayerWidget p;
        QCOMPARE(p.focusPolicy(), Qt::StrongFocus);
        for (QSlider *s : p.findChildren<QSlider *>()) {
            QVERIFY2(s->focusPolicy() == Qt::NoFocus, qPrintable(s->objectName()));
        }
        for (QToolButton *b : p.findChildren<QToolButton *>()) {
            QVERIFY2(b->focusPolicy() == Qt::NoFocus, qPrintable(b->toolTip()));
        }
    }

    void arrowKeysStepVolumeByFive()
    {
        PlayerWidget p;
        p.setInitialVolume(100);
        QSignalSpy spy(&p, &PlayerWidget::volumeChanged);
        QTest::keyClick(&p, Qt::Key_Up);
        QCOMPARE(volume(p)->value(), 105);
        QTest::keyClick(&p, Qt::Key_Plus);
        QCOMPARE(volume(p)->value(), 110);
        QTest::keyClick(&p, Qt::Key_Down);
        QTest::keyClick(&p, Qt::Key_Minus);
        QCOMPARE(volume(p)->value(), 100);
        QCOMPARE(spy.count(), 4);
        QCOMPARE(spy.last().at(0).toInt(), 100);
    }

    void volumeClampsAtBounds()
    {
        PlayerWidget p;
        p.setInitialVolume(148);
        QTest::keyClick(&p, Qt::Key_Up);
        QCOMPARE(volume(p)->value(), 150);
        QTest::keyClick(&p, Qt::Key_Up);
        QCOMPARE(volume(p)->value(), 150);
        p.setInitialVolume(3);
        QTest::keyClick(&p, Qt::Key_Down);
        QCOMPARE(volume(p)->value(), 0);
    }

    void wheelStepsVolumePerNotch()
    {
        PlayerWidget p;
        p.setInitialVolume(100);
        wheel(p, 120);
        QCOMPARE(volume(p)->value(), 105);
        // Touchpad-style half notches accumulate instead of each stepping.
        wheel(p, -60);
        QCOMPARE(volume(p)->value(), 105);
        wheel(p, -60);
        QCOMPARE(volume(p)->value(), 100);
    }

    void seekButtonsFollowSeekability()
    {
        PlayerWidget p;
        QVERIFY(!button(p, "seekBack")->isEnabled());
        QVERIFY(!button(p, "seekForward")->isEnabled());
        auto *mpv = p.findChild<MpvWidget *>();
        QVERIFY(mpv);
        Q_EMIT mpv->durationChanged(3600.0); // VOD
        QVERIFY(button(p, "seekBack")->isEnabled());
        QVERIFY(button(p, "seekForward")->isEnabled());
        QCOMPARE(p.findChild<QSlider *>("position")->maximum(), 3600);
        Q_EMIT mpv->durationChanged(0.0); // live
        QVERIFY(!button(p, "seekForward")->isEnabled());
    }

    void navigationButtonsFollowPlaylistPosition()
    {
        PlayerWidget p;
        auto *prev = button(p, "previous");
        auto *next = button(p, "next");
        // Nothing to navigate (fresh player or single movie): hidden.
        QVERIFY(!prev->isVisibleTo(&p));
        QVERIFY(!next->isVisibleTo(&p));
        // First episode of a series: next only.
        p.setNavigation(false, true);
        QVERIFY(prev->isVisibleTo(&p) && next->isVisibleTo(&p));
        QVERIFY(!prev->isEnabled());
        QVERIFY(next->isEnabled());
        // Middle of a list.
        p.setNavigation(true, true);
        QVERIFY(prev->isEnabled() && next->isEnabled());
        p.setNavigation(false, false);
        QVERIFY(!prev->isVisibleTo(&p));
    }

    void finishedOnlyWhenEofNearTheEnd()
    {
        PlayerWidget p;
        auto *mpv = p.findChild<MpvWidget *>();
        QSignalSpy spy(&p, &PlayerWidget::finished);

        Q_EMIT mpv->durationChanged(1800.0);
        Q_EMIT mpv->positionChanged(600.0);
        Q_EMIT mpv->eofReached(); // stream dropped at 10 min of 30
        QCOMPARE(spy.count(), 0);

        Q_EMIT mpv->positionChanged(1785.0);
        Q_EMIT mpv->eofReached(); // genuine end
        QCOMPARE(spy.count(), 1);
    }

    void liveEofIsNotFinished()
    {
        PlayerWidget p;
        auto *mpv = p.findChild<MpvWidget *>();
        QSignalSpy spy(&p, &PlayerWidget::finished);
        Q_EMIT mpv->durationChanged(0.0);
        Q_EMIT mpv->positionChanged(5000.0);
        Q_EMIT mpv->eofReached();
        QCOMPARE(spy.count(), 0);
    }

    void navigationButtonsEmitRequests()
    {
        PlayerWidget p;
        p.setNavigation(true, true);
        QSignalSpy nextSpy(&p, &PlayerWidget::nextRequested);
        QSignalSpy prevSpy(&p, &PlayerWidget::previousRequested);
        button(p, "next")->click();
        QTest::keyClick(&p, Qt::Key_N);
        button(p, "previous")->click();
        QCOMPARE(nextSpy.count(), 2);
        QCOMPARE(prevSpy.count(), 1);
    }
};

QTEST_MAIN(TestPlayerWidget)
#include "test_playerwidget.moc"
