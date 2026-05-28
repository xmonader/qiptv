#include "player/mpvwidget.h"

#include <mpv/client.h>

#include <QtGlobal>

namespace kiptv {

namespace {
// Property reply ids for observe_property (also used to route events).
constexpr quint64 kTimePos = 1;
constexpr quint64 kDuration = 2;
constexpr quint64 kPause = 3;
} // namespace

MpvWidget::MpvWidget(QWidget *parent)
    : QWidget(parent)
{
    // mpv renders into a native child window keyed off our winId().
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(320, 180);

    m_mpv = mpv_create();
    if (!m_mpv) {
        return;
    }

    // Hardware decoding where safe; keep the OSD off (we draw our own UI).
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpv, "vo", "gpu");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    // Reasonable network resilience for live IPTV streams.
    mpv_set_option_string(m_mpv, "cache", "yes");
    mpv_set_option_string(m_mpv, "demuxer-max-bytes", "50MiB");

    const auto wid = static_cast<qint64>(winId());
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, const_cast<qint64 *>(&wid));

    if (mpv_initialize(m_mpv) < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    mpv_observe_property(m_mpv, kTimePos, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, kDuration, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, kPause, "pause", MPV_FORMAT_FLAG);

    // wakeup() runs on mpv's thread; bounce to the GUI thread via a queued slot.
    mpv_set_wakeup_callback(m_mpv, &MpvWidget::wakeup, this);
}

MpvWidget::~MpvWidget()
{
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

void MpvWidget::wakeup(void *ctx)
{
    auto *self = static_cast<MpvWidget *>(ctx);
    QMetaObject::invokeMethod(self, "onMpvEvents", Qt::QueuedConnection);
}

void MpvWidget::play(const QString &url)
{
    if (!m_mpv) {
        Q_EMIT playbackError(QStringLiteral("Player not initialized."));
        return;
    }
    const QByteArray u = url.toUtf8();
    const char *cmd[] = {"loadfile", u.constData(), nullptr};
    mpv_command(m_mpv, cmd);
}

void MpvWidget::stop()
{
    if (!m_mpv) {
        return;
    }
    const char *cmd[] = {"stop", nullptr};
    mpv_command(m_mpv, cmd);
}

void MpvWidget::setPaused(bool paused)
{
    if (!m_mpv) {
        return;
    }
    int flag = paused ? 1 : 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvWidget::togglePause()
{
    setPaused(!m_paused);
}

void MpvWidget::setVolume(int percent)
{
    setProperty("volume", qBound(0, percent, 150));
}

void MpvWidget::seekRelative(double seconds)
{
    if (!m_mpv) {
        return;
    }
    const QByteArray s = QByteArray::number(seconds);
    const char *cmd[] = {"seek", s.constData(), "relative", nullptr};
    mpv_command(m_mpv, cmd);
}

void MpvWidget::seekAbsolute(double seconds)
{
    if (!m_mpv) {
        return;
    }
    const QByteArray s = QByteArray::number(seconds);
    const char *cmd[] = {"seek", s.constData(), "absolute", nullptr};
    mpv_command(m_mpv, cmd);
}

void MpvWidget::setProperty(const char *name, double value)
{
    if (m_mpv) {
        mpv_set_property(m_mpv, name, MPV_FORMAT_DOUBLE, &value);
    }
}

void MpvWidget::onMpvEvents()
{
    if (!m_mpv) {
        return;
    }
    // Drain all pending events without blocking.
    while (true) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *prop = static_cast<mpv_event_property *>(event->data);
            if (event->reply_userdata == kTimePos && prop->format == MPV_FORMAT_DOUBLE) {
                Q_EMIT positionChanged(*static_cast<double *>(prop->data));
            } else if (event->reply_userdata == kDuration && prop->format == MPV_FORMAT_DOUBLE) {
                Q_EMIT durationChanged(*static_cast<double *>(prop->data));
            } else if (event->reply_userdata == kPause && prop->format == MPV_FORMAT_FLAG) {
                m_paused = *static_cast<int *>(prop->data) != 0;
                Q_EMIT pausedChanged(m_paused);
            }
            break;
        }
        case MPV_EVENT_END_FILE: {
            auto *ef = static_cast<mpv_event_end_file *>(event->data);
            if (ef->reason == MPV_END_FILE_REASON_ERROR) {
                Q_EMIT endReached(QString::fromUtf8(mpv_error_string(ef->error)));
            } else {
                Q_EMIT endReached(QString());
            }
            break;
        }
        default:
            break;
        }
    }
}

} // namespace kiptv
