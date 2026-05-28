#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace kiptv {

/// A series from get_series (list view).
struct Series {
    int seriesId = 0;
    QString name;
    QString cover;       ///< poster URL
    QString categoryId;
    QString plot;
    QString cast;
    QString genre;
    QString releaseDate;
    double rating = 0.0;
    qint64 lastModified = 0;

    static Series fromJson(const QJsonObject &o);
};

/// One episode within a series season (from get_series_info episodes map).
struct Episode {
    QString id;                 ///< episode id (string per Xtream)
    int episodeNum = 0;
    int season = 0;
    QString title;
    QString containerExtension; ///< needed to build the stream URL
    QString plot;
    QString duration;

    static Episode fromJson(const QJsonObject &o, int seasonNumber);
};

/// Series detail from get_series_info: the "info" block plus a flat list of
/// episodes across all seasons (each tagged with its season number).
struct SeriesInfo {
    int seriesId = 0;
    QString name;
    QString cover;
    QString plot;
    QString cast;
    QString genre;
    QString releaseDate;
    double rating = 0.0;
    QVector<Episode> episodes; ///< sorted by (season, episodeNum)

    /// Distinct season numbers present, ascending.
    QVector<int> seasons() const;

    static SeriesInfo fromJson(const QByteArray &json);
};

/// Parse a get_series response (JSON array).
QVector<Series> seriesListFromJson(const QByteArray &json);

} // namespace kiptv
