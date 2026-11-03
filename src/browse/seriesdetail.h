#pragma once

#include "core/playlist.h"
#include "models/series.h"

#include <QWidget>

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;

namespace qiptv {

class XtreamClient;
class ImageLoader;

/**
 * Series detail: poster + plot, a season selector, and the episode list for
 * the chosen season. Activating an episode emits playEpisode() with a ready
 * stream URL. backRequested() returns to the browser.
 */
class SeriesDetail : public QWidget {
    Q_OBJECT
public:
    SeriesDetail(XtreamClient *client, ImageLoader *images, QWidget *parent = nullptr);

    /// Show the given series and fetch its detail.
    void open(const QString &seriesId, const QString &name);

Q_SIGNALS:
    /// Every episode of the series in season/episode order, cursor on the
    /// activated one, so next/previous crosses season boundaries.
    void playEpisode(const qiptv::Playlist &playlist);
    void backRequested();

private:
    void onInfoReady(const SeriesInfo &info);
    void onSeasonChanged();
    void onEpisodeActivated(QListWidgetItem *item);
    void onImageLoaded(const QString &url, const QImage &image);

    QPushButton *m_back = nullptr;
    QLabel *m_poster = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_plot = nullptr;
    QComboBox *m_seasonBox = nullptr;
    QListWidget *m_episodes = nullptr;

    XtreamClient *m_client = nullptr;
    ImageLoader *m_images = nullptr;

    QString m_seriesId;
    QString m_name;
    QString m_posterUrl;
    SeriesInfo m_info;
};

} // namespace qiptv
