#include "player/playerwidget.h"

#include "player/mpvwidget.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace kiptv {

namespace {
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

    m_back = iconButton(QStringLiteral("go-previous"), i18n("Back"), this);
    m_previous = iconButton(QStringLiteral("media-skip-backward"), i18n("Previous channel"), this);
    m_playPause = iconButton(QStringLiteral("media-playback-pause"), i18n("Play/Pause"), this);
    m_stop = iconButton(QStringLiteral("media-playback-stop"), i18n("Stop"), this);
    m_next = iconButton(QStringLiteral("media-skip-forward"), i18n("Next channel"), this);
    m_fullscreen = iconButton(QStringLiteral("view-fullscreen"), i18n("Fullscreen"), this);

    m_position = new QSlider(Qt::Horizontal, this);
    m_position->setRange(0, 0);
    m_timeLabel = new QLabel(QStringLiteral("0:00 / 0:00"), this);

    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 150);
    m_volume->setValue(100);
    m_volume->setMaximumWidth(120);
    m_volume->setToolTip(i18n("Volume"));

    auto *row = new QHBoxLayout;
    row->addWidget(m_back);
    row->addWidget(m_previous);
    row->addWidget(m_playPause);
    row->addWidget(m_stop);
    row->addWidget(m_next);
    row->addWidget(m_position, 1);
    row->addWidget(m_timeLabel);
    row->addWidget(new QLabel(QStringLiteral("🔊"), this));
    row->addWidget(m_volume);
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
    connect(m_playPause, &QToolButton::clicked, m_mpv, &MpvWidget::togglePause);
    connect(m_stop, &QToolButton::clicked, this, &PlayerWidget::stop);
    connect(m_fullscreen, &QToolButton::clicked, this, &PlayerWidget::toggleFullscreen);
    connect(m_volume, &QSlider::valueChanged, m_mpv, &MpvWidget::setVolume);

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

void PlayerWidget::play(const QString &url, const QString &title)
{
    m_title->setText(title);
    m_duration = 0.0;
    m_position->setRange(0, 0);
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
    const bool seekable = seconds > 0;
    m_position->setEnabled(seekable);
    m_position->setRange(0, seekable ? static_cast<int>(seconds) : 0);
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
        Q_EMIT errorOccurred(i18n("Playback error: %1", error));
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
        m_mpv->seekRelative(-10);
        break;
    case Qt::Key_Right:
        m_mpv->seekRelative(10);
        break;
    case Qt::Key_Up:
        m_volume->setValue(m_volume->value() + 5);
        break;
    case Qt::Key_Down:
        m_volume->setValue(m_volume->value() - 5);
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

} // namespace kiptv
