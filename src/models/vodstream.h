#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace kiptv {

/// A movie from get_vod_streams (list view).
struct VodStream {
    int streamId = 0;
    int num = 0;
    QString name;
    QString icon;               ///< stream_icon / cover poster URL
    QString categoryId;
    QString containerExtension; ///< "mkv", "mp4" — needed to build stream URL
    double rating = 0.0;
    qint64 added = 0;

    static VodStream fromJson(const QJsonObject &o);
};

/// Movie detail from get_vod_info (the "info" + "movie_data" blocks).
struct VodInfo {
    int streamId = 0;
    QString name;
    QString containerExtension;
    QString plot;
    QString cast;
    QString director;
    QString genre;
    QString releaseDate;
    QString duration;
    QString coverBig;          ///< large poster
    QString backdropPath;
    QString youtubeTrailer;
    double rating = 0.0;

    static VodInfo fromJson(const QByteArray &json);
};

/// Parse a get_vod_streams response (JSON array).
QVector<VodStream> vodStreamsFromJson(const QByteArray &json);

} // namespace kiptv
