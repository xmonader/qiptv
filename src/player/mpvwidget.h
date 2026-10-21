#pragma once

#include <QOpenGLWidget>

struct mpv_handle;
struct mpv_render_context;

namespace kiptv {

/**
 * A QOpenGLWidget that renders libmpv video through mpv's render API.
 *
 * Unlike the older "wid" embedding, this is an ordinary Qt widget with no
 * native child window: overlay/sibling controls stack correctly, input is
 * delivered by Qt, and there is no compositor flicker. mpv draws into the
 * widget's FBO from paintGL(); a render-update callback (delivered on mpv's
 * thread) requests a repaint on the GUI thread.
 */
class MpvWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget() override;

    bool isValid() const { return m_mpv != nullptr; }
    bool isPaused() const { return m_paused; }
    mpv_handle *handle() const { return m_mpv; } ///< for diagnostics/integration

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
    void initializeGL() override;
    void paintGL() override;

private Q_SLOTS:
    void onMpvEvents();
    void doUpdate();

private:
    void setProperty(const char *name, double value);
    static void onWakeup(void *ctx);
    static void onRenderUpdate(void *ctx);
    static void *getProcAddress(void *ctx, const char *name);

    mpv_handle *m_mpv = nullptr;
    mpv_render_context *m_renderCtx = nullptr;
    bool m_paused = false;
};

} // namespace kiptv
