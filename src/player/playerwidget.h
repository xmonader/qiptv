#pragma once

#include <QWidget>

class QSlider;
class QToolButton;
class QLabel;
class QTimer;

namespace kiptv {

class MpvWidget;

/**
 * Full playback page: the mpv video surface plus a control bar (back,
 * play/pause, stop, seek, time, volume, fullscreen), keyboard shortcuts, and
 * auto-hiding controls in fullscreen.
 */
class PlayerWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerWidget(QWidget *parent = nullptr);

    void play(const QString &url, const QString &title);
    void stop();
    void togglePause(); ///< exposed for global media keys / tray

Q_SIGNALS:
    void backRequested();
    void errorOccurred(const QString &message);
    void previousRequested();
    void nextRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void onPositionChanged(double seconds);
    void onDurationChanged(double seconds);
    void onPausedChanged(bool paused);
    void onEndReached(const QString &error);
    void onSliderMoved(int value);
    void toggleFullscreen();
    void leaveFullscreen();
    void nudgeControls();
    void updatePlayIcon(bool paused);

    MpvWidget *m_mpv = nullptr;
    QWidget *m_controls = nullptr;
    QToolButton *m_back = nullptr;
    QToolButton *m_previous = nullptr;
    QToolButton *m_playPause = nullptr;
    QToolButton *m_stop = nullptr;
    QToolButton *m_next = nullptr;
    QToolButton *m_fullscreen = nullptr;
    QSlider *m_position = nullptr;
    QSlider *m_volume = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLabel *m_title = nullptr;
    QTimer *m_hideTimer = nullptr;

    double m_duration = 0.0;
    bool m_userSeeking = false;
    bool m_isFullscreen = false;
};

} // namespace kiptv
