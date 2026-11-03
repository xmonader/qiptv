#include "player/playerwidget.h"

#include "player/mpvwidget.h"


#include <QCloseEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace qiptv {

namespace {
constexpr int kSeekStepSeconds = 10;
constexpr int kVolumeStep = 5;
constexpr int kVolumeMax = 150;

QString formatTime(double seconds)
{
    if (seconds < 0 || !qIsFinite(seconds)) {
        seconds = 0;
    }
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
}

QToolButton *iconButton(const QString &theme, const QString &tip, QWidget *parent)
{
    auto *b = new QToolButton(parent);
    b->setIcon(QIcon::fromTheme(theme));
    b->setToolTip(tip);
    b->setAutoRaise(true);
    // Controls must never hold keyboard focus: the player handles Space/arrows
    // itself, and a focused button/slider would swallow them.
    b->setFocusPolicy(Qt::NoFocus);
    return b;
}
} // namespace

PlayerWidget::PlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    m_mpv = new MpvWidget(this);
    m_mpv->setMouseTracking(true);
    m_mpv->installEventFilter(this);

    m_title = new QLabel(this);
    m_title->setStyleSheet(QStringLiteral("font-weight: bold;"));

    m_back = iconButton(QStringLiteral("go-previous"), tr("Back"), this);
    m_previous = iconButton(QStringLiteral("media-skip-backward"), tr("Previous (P)"), this);
    m_playPause = iconButton(QStringLiteral("media-playback-pause"), tr("Play/Pause"), this);
    m_stop = iconButton(QStringLiteral("media-playback-stop"), tr("Stop"), this);
    m_next = iconButton(QStringLiteral("media-skip-forward"), tr("Next (N)"), this);
    m_seekBack = iconButton(QStringLiteral("media-seek-backward"),
                            tr("Back %1 seconds (Left)").arg(kSeekStepSeconds), this);
    m_seekForward = iconButton(QStringLiteral("media-seek-forward"),
                               tr("Forward %1 seconds (Right)").arg(kSeekStepSeconds), this);
    m_previous->setObjectName(QStringLiteral("previous"));
    m_next->setObjectName(QStringLiteral("next"));
    m_seekBack->setObjectName(QStringLiteral("seekBack"));
    m_seekForward->setObjectName(QStringLiteral("seekForward"));
    m_fullscreen = iconButton(QStringLiteral("view-fullscreen"), tr("Fullscreen"), this);

    m_position = new QSlider(Qt::Horizontal, this);
    m_position->setRange(0, 0);
    m_position->setFocusPolicy(Qt::NoFocus);
    m_position->setObjectName(QStringLiteral("position"));
    m_timeLabel = new QLabel(QStringLiteral("0:00 / 0:00"), this);

    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, kVolumeMax);
    m_volume->setValue(100);
    m_volume->setSingleStep(kVolumeStep);
    m_volume->setPageStep(kVolumeStep * 2);
    m_volume->setMaximumWidth(120);
    m_volume->setToolTip(tr("Volume (Up/Down, mouse wheel)"));
    m_volume->setFocusPolicy(Qt::NoFocus);
    m_volume->setObjectName(QStringLiteral("volume"));
    m_volumeLabel = new QLabel(this);
    m_volumeLabel->setObjectName(QStringLiteral("volumeLabel"));
    // Reserve room for "150%" so the layout doesn't jitter while adjusting.
    m_volumeLabel->setMinimumWidth(m_volumeLabel->fontMetrics().horizontalAdvance(QStringLiteral("150%")));
    m_volumeLabel->setText(QStringLiteral("100%"));

    auto *row = new QHBoxLayout;
    row->addWidget(m_back);
    row->addWidget(m_previous);
    row->addWidget(m_seekBack);
    row->addWidget(m_playPause);
    row->addWidget(m_stop);
    row->addWidget(m_seekForward);
    row->addWidget(m_next);
    row->addWidget(m_position, 1);
    row->addWidget(m_timeLabel);
    row->addWidget(new QLabel(QStringLiteral("🔊"), this));
    row->addWidget(m_volume);
    row->addWidget(m_volumeLabel);
    row->addWidget(m_fullscreen);

    auto *controlsLayout = new QVBoxLayout;
    controlsLayout->setContentsMargins(6, 2, 6, 4);
    controlsLayout->addWidget(m_title);
    controlsLayout->addLayout(row);

    m_controls = new QWidget(this);
    m_controls->setLayout(controlsLayout);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_mpv, 1);
    root->addWidget(m_controls);

    setMouseTracking(true);
    // The player owns keyboard input; clicks anywhere inside (all children are
    // NoFocus) give focus back here.
    setFocusPolicy(Qt::StrongFocus);
    setSeekable(false);
    setNavigation(false, false);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(3000);
    connect(m_hideTimer, &QTimer::timeout, this, [this] {
        if (m_isFullscreen) {
            m_controls->hide();
            m_title->hide();
        }
    });

    connect(m_back, &QToolButton::clicked, this, [this] {
        stop();
        leaveFullscreen();
        Q_EMIT backRequested();
    });
    connect(m_previous, &QToolButton::clicked, this, &PlayerWidget::previousRequested);
    connect(m_next, &QToolButton::clicked, this, &PlayerWidget::nextRequested);
    connect(m_seekBack, &QToolButton::clicked, this, [this] { m_mpv->seekRelative(-kSeekStepSeconds); });
    connect(m_seekForward, &QToolButton::clicked, this, [this] { m_mpv->seekRelative(kSeekStepSeconds); });
    connect(m_playPause, &QToolButton::clicked, m_mpv, &MpvWidget::togglePause);
    connect(m_stop, &QToolButton::clicked, this, &PlayerWidget::stop);
    connect(m_fullscreen, &QToolButton::clicked, this, &PlayerWidget::toggleFullscreen);
    connect(m_volume, &QSlider::valueChanged, m_mpv, &MpvWidget::setVolume);
    connect(m_volume, &QSlider::valueChanged, this, &PlayerWidget::volumeChanged);
    connect(m_volume, &QSlider::valueChanged, this,
            [this](int v) { m_volumeLabel->setText(QStringLiteral("%1%").arg(v)); });

    connect(m_position, &QSlider::sliderPressed, this, [this] { m_userSeeking = true; });
    connect(m_position, &QSlider::sliderReleased, this, [this] {
        m_userSeeking = false;
        m_mpv->seekAbsolute(m_position->value());
    });
    connect(m_position, &QSlider::sliderMoved, this, &PlayerWidget::onSliderMoved);

    connect(m_mpv, &MpvWidget::positionChanged, this, &PlayerWidget::onPositionChanged);
    connect(m_mpv, &MpvWidget::durationChanged, this, &PlayerWidget::onDurationChanged);
    connect(m_mpv, &MpvWidget::pausedChanged, this, &PlayerWidget::onPausedChanged);
    connect(m_mpv, &MpvWidget::endReached, this, &PlayerWidget::onEndReached);
    connect(m_mpv, &MpvWidget::playbackError, this, &PlayerWidget::errorOccurred);
}

void PlayerWidget::setNavigation(bool canPrevious, bool canNext)
{
    const bool any = canPrevious || canNext;
    m_previous->setVisible(any);
    m_next->setVisible(any);
    m_previous->setEnabled(canPrevious);
    m_next->setEnabled(canNext);
}

void PlayerWidget::play(const QString &url, const QString &title)
{
    m_title->setText(title);
    m_duration = 0.0;
    setSeekable(false);
    m_timeLabel->setText(QStringLiteral("0:00 / 0:00"));
    m_mpv->setVolume(m_volume->value());
    m_mpv->play(url);
    setFocus();
}

void PlayerWidget::stop()
{
    m_mpv->stop();
}

void PlayerWidget::togglePause()
{
    m_mpv->togglePause();
}

void PlayerWidget::setInitialVolume(int percent)
{
    m_volume->setValue(qBound(0, percent, kVolumeMax));
}

void PlayerWidget::onPositionChanged(double seconds)
{
    if (!m_userSeeking) {
        m_position->setValue(static_cast<int>(seconds));
    }
    m_timeLabel->setText(formatTime(seconds) + QStringLiteral(" / ") + formatTime(m_duration));
}

void PlayerWidget::onDurationChanged(double seconds)
{
    m_duration = seconds;
    // Live streams report 0 duration; disable seeking for those.
    setSeekable(seconds > 0);
}

void PlayerWidget::setSeekable(bool seekable)
{
    m_position->setEnabled(seekable);
    m_position->setRange(0, seekable ? static_cast<int>(m_duration) : 0);
    m_seekBack->setEnabled(seekable);
    m_seekForward->setEnabled(seekable);
}

void PlayerWidget::stepVolume(int delta)
{
    // QSlider clamps to its range, and valueChanged drives mpv + persistence.
    m_volume->setValue(m_volume->value() + delta);
}

void PlayerWidget::onPausedChanged(bool paused)
{
    updatePlayIcon(paused);
}

void PlayerWidget::updatePlayIcon(bool paused)
{
    m_playPause->setIcon(QIcon::fromTheme(paused ? QStringLiteral("media-playback-start")
                                                  : QStringLiteral("media-playback-pause")));
}

void PlayerWidget::onEndReached(const QString &error)
{
    if (!error.isEmpty()) {
        Q_EMIT errorOccurred(tr("Playback error: %1").arg(error));
    }
}

void PlayerWidget::onSliderMoved(int value)
{
    m_timeLabel->setText(formatTime(value) + QStringLiteral(" / ") + formatTime(m_duration));
}

void PlayerWidget::toggleFullscreen()
{
    QWidget *win = window();
    if (!win) {
        return;
    }
    if (m_isFullscreen) {
        leaveFullscreen();
    } else {
        m_isFullscreen = true;
        win->showFullScreen();
        m_fullscreen->setIcon(QIcon::fromTheme(QStringLiteral("view-restore")));
        nudgeControls();
    }
}

void PlayerWidget::leaveFullscreen()
{
    if (!m_isFullscreen) {
        return;
    }
    m_isFullscreen = false;
    if (QWidget *win = window()) {
        win->showNormal();
    }
    m_fullscreen->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
    m_controls->show();
    m_title->show();
    m_hideTimer->stop();
}

void PlayerWidget::nudgeControls()
{
    m_controls->show();
    m_title->show();
    if (m_isFullscreen) {
        m_hideTimer->start();
    } else {
        m_hideTimer->stop();
    }
}

void PlayerWidget::mouseMoveEvent(QMouseEvent *event)
{
    nudgeControls();
    QWidget::mouseMoveEvent(event);
}

void PlayerWidget::wheelEvent(QWheelEvent *event)
{
    // One notch = 120 units; accumulate so high-resolution touchpads, which
    // send many small deltas, don't jump the volume on every tiny movement.
    m_wheelAccum += event->angleDelta().y();
    const int notches = m_wheelAccum / 120;
    if (notches != 0) {
        m_wheelAccum -= notches * 120;
        stepVolume(notches * kVolumeStep);
        nudgeControls();
    }
    event->accept();
}

void PlayerWidget::closeEvent(QCloseEvent *event)
{
    // The player is its own top-level window; closing it must return to the
    // browser, not quit the application. Hide and stop instead of destroying.
    stop();
    leaveFullscreen();
    hide();
    event->ignore();
    Q_EMIT backRequested();
}

bool PlayerWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_mpv && event->type() == QEvent::MouseMove) {
        nudgeControls();
    }
    return QWidget::eventFilter(watched, event);
}

void PlayerWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        m_mpv->togglePause();
        break;
    case Qt::Key_Left:
        m_mpv->seekRelative(-kSeekStepSeconds);
        break;
    case Qt::Key_Right:
        m_mpv->seekRelative(kSeekStepSeconds);
        break;
    case Qt::Key_Up:
    case Qt::Key_Plus:
    case Qt::Key_Equal:
    case Qt::Key_VolumeUp:
        stepVolume(kVolumeStep);
        break;
    case Qt::Key_Down:
    case Qt::Key_Minus:
    case Qt::Key_VolumeDown:
        stepVolume(-kVolumeStep);
        break;
    case Qt::Key_F:
        toggleFullscreen();
        break;
    case Qt::Key_N:
    case Qt::Key_MediaNext:
        Q_EMIT nextRequested();
        break;
    case Qt::Key_P:
    case Qt::Key_MediaPrevious:
        Q_EMIT previousRequested();
        break;
    case Qt::Key_Escape:
        if (m_isFullscreen) {
            leaveFullscreen();
        }
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    nudgeControls();
    event->accept();
}

} // namespace qiptv
