#pragma once

#include <QWidget>

struct mpv_handle;

namespace kiptv {

/**
 * A QWidget that embeds libmpv for video playback.
 *
 * Embedding uses mpv's "wid" option (mpv renders into this widget's native
 * window). This is simple and robust on X11/XWayland; a native Wayland setup
 * may fall back through XWayland. A GPU render-API path is a future upgrade.
 *
 * libmpv delivers events on its own thread; we marshal them to the GUI thread
 * via a queued signal before touching any Qt state.
 */
class MpvWidget : public QWidget {
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget() override;

    bool isValid() const { return m_mpv != nullptr; }
    bool isPaused() const { return m_paused; }

    void play(const QString &url);
    void stop();
    void setPaused(bool paused);
    void togglePause();
    void setVolume(int percent); ///< 0–150
    void seekRelative(double seconds);
    void seekAbsolute(double seconds);

Q_SIGNALS:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void pausedChanged(bool paused);
    void endReached(const QString &error); ///< error empty on clean end
    void playbackError(const QString &message);

protected:
    // mpv owns rendering into our native window; Qt must not paint over it.
    QPaintEngine *paintEngine() const override { return nullptr; }

private Q_SLOTS:
    void onMpvEvents();

private:
    void setProperty(const char *name, double value);
    static void wakeup(void *ctx);

    mpv_handle *m_mpv = nullptr;
    bool m_paused = false;
};

} // namespace kiptv
