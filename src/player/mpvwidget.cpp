#include "player/mpvwidget.h"

#include "core/useragent.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <QOpenGLContext>
#include <QtGlobal>

#include <clocale>

namespace qiptv {

namespace {
constexpr quint64 kTimePos = 1;
constexpr quint64 kDuration = 2;
constexpr quint64 kPause = 3;
} // namespace

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(320, 180);

    // libmpv requires the C numeric locale; Qt applies the user's locale
    // (e.g. comma decimals), which mpv warns about and which breaks parsing.
    std::setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv) {
        return;
    }

    // Critical for render-API embedding: force the libmpv video output.
    // Without it mpv defaults to vo=gpu and opens its OWN top-level window for
    // the video (separate from our controls). With vo=libmpv, video appears
    // only through mpv_render_context_render() into this widget's FBO.
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    // Network resilience for live IPTV streams.
    mpv_set_option_string(m_mpv, "cache", "yes");
    mpv_set_option_string(m_mpv, "demuxer-max-bytes", "50MiB");
    // Same reason as the HTTP client: no UA -> provider 403s the stream.
    mpv_set_option_string(m_mpv, "useragent", kUserAgent);

    if (mpv_initialize(m_mpv) < 0) {
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    mpv_observe_property(m_mpv, kTimePos, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, kDuration, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, kPause, "pause", MPV_FORMAT_FLAG);

    // Surface mpv's own diagnostics (network, decoder, VO errors) in stderr;
    // without this a failed stream is indistinguishable from a black frame.
    mpv_request_log_messages(m_mpv, "info");

    mpv_set_wakeup_callback(m_mpv, &MpvWidget::onWakeup, this);
}

MpvWidget::~MpvWidget()
{
    if (m_renderCtx) {
        // The render context must be freed with the GL context current.
        makeCurrent();
        mpv_render_context_free(m_renderCtx);
        m_renderCtx = nullptr;
        doneCurrent();
    }
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

void *MpvWidget::getProcAddress(void *ctx, const char *name)
{
    Q_UNUSED(ctx)
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx) {
        return nullptr;
    }
    return reinterpret_cast<void *>(glctx->getProcAddress(name));
}

void MpvWidget::initializeGL()
{
    if (!m_mpv || m_renderCtx) {
        return;
    }
    mpv_opengl_init_params glInit{&MpvWidget::getProcAddress, this};
    int advanced = 1;
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_create(&m_renderCtx, m_mpv, params) < 0) {
        m_renderCtx = nullptr;
        Q_EMIT playbackError(QStringLiteral("Failed to create mpv render context."));
        return;
    }
    mpv_render_context_set_update_callback(m_renderCtx, &MpvWidget::onRenderUpdate, this);
}

void MpvWidget::paintGL()
{
    if (!m_renderCtx) {
        return;
    }
    const qreal dpr = devicePixelRatioF();
    mpv_opengl_fbo fbo{static_cast<int>(defaultFramebufferObject()),
                       static_cast<int>(width() * dpr),
                       static_cast<int>(height() * dpr),
                       0};
    int flipY = 1;
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    mpv_render_context_render(m_renderCtx, params);
}

void MpvWidget::onRenderUpdate(void *ctx)
{
    // Called on mpv's render thread; bounce to the GUI thread.
    auto *self = static_cast<MpvWidget *>(ctx);
    QMetaObject::invokeMethod(self, "doUpdate", Qt::QueuedConnection);
}

void MpvWidget::doUpdate()
{
    if (m_renderCtx && (mpv_render_context_update(m_renderCtx) & MPV_RENDER_UPDATE_FRAME)) {
        update();
    }
}

void MpvWidget::onWakeup(void *ctx)
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
        case MPV_EVENT_LOG_MESSAGE: {
            auto *msg = static_cast<mpv_event_log_message *>(event->data);
            qWarning("%s: %s", msg->prefix, msg->text);
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

} // namespace qiptv
