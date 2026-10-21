#include "models/series.h"

#include "core/parse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

namespace qiptv {

Series Series::fromJson(const QJsonObject &o)
{
    Series s;
    s.seriesId = parse::integer(o.value(QStringLiteral("series_id")));
    s.name = parse::str(o.value(QStringLiteral("name")));
    s.cover = parse::str(o.value(QStringLiteral("cover")));
    s.categoryId = parse::str(o.value(QStringLiteral("category_id")));
    s.plot = parse::str(o.value(QStringLiteral("plot")));
    s.cast = parse::str(o.value(QStringLiteral("cast")));
    s.genre = parse::str(o.value(QStringLiteral("genre")));
    s.releaseDate = parse::str(o.value(QStringLiteral("releaseDate")));
    if (s.releaseDate.isEmpty()) {
        s.releaseDate = parse::str(o.value(QStringLiteral("release_date")));
    }
    s.rating = parse::number(o.value(QStringLiteral("rating")));
    s.lastModified = parse::integer64(o.value(QStringLiteral("last_modified")));
    return s;
}

Episode Episode::fromJson(const QJsonObject &o, int seasonNumber)
{
    Episode e;
    e.id = parse::str(o.value(QStringLiteral("id")));
    e.episodeNum = parse::integer(o.value(QStringLiteral("episode_num")));
    // Trust the episodes-map key, but fall back to an inline season field.
    e.season = seasonNumber > 0 ? seasonNumber : parse::integer(o.value(QStringLiteral("season")));
    e.title = parse::str(o.value(QStringLiteral("title")));
    e.containerExtension = parse::str(o.value(QStringLiteral("container_extension")));

    const QJsonObject info = o.value(QStringLiteral("info")).toObject();
    e.plot = parse::str(info.value(QStringLiteral("plot")));
    e.duration = parse::str(info.value(QStringLiteral("duration")));
    return e;
}

QVector<int> SeriesInfo::seasons() const
{
    QVector<int> out;
    for (const Episode &e : episodes) {
        if (!out.contains(e.season)) {
            out.push_back(e.season);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

SeriesInfo SeriesInfo::fromJson(const QByteArray &json)
{
    SeriesInfo si;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return si;
    }
    const QJsonObject root = doc.object();
    const QJsonObject info = root.value(QStringLiteral("info")).toObject();

    si.name = parse::str(info.value(QStringLiteral("name")));
    si.cover = parse::str(info.value(QStringLiteral("cover")));
    si.plot = parse::str(info.value(QStringLiteral("plot")));
    si.cast = parse::str(info.value(QStringLiteral("cast")));
    si.genre = parse::str(info.value(QStringLiteral("genre")));
    si.releaseDate = parse::str(info.value(QStringLiteral("releaseDate")));
    if (si.releaseDate.isEmpty()) {
        si.releaseDate = parse::str(info.value(QStringLiteral("release_date")));
    }
    si.rating = parse::number(info.value(QStringLiteral("rating")));

    // "episodes" is an object keyed by season number (as a string), each
    // mapping to an array of episode objects.
    const QJsonObject epMap = root.value(QStringLiteral("episodes")).toObject();
    for (auto it = epMap.constBegin(); it != epMap.constEnd(); ++it) {
        const int seasonNum = it.key().toInt();
        const QJsonArray eps = it.value().toArray();
        for (const QJsonValue &v : eps) {
            if (v.isObject()) {
                si.episodes.push_back(Episode::fromJson(v.toObject(), seasonNum));
            }
        }
    }

    std::sort(si.episodes.begin(), si.episodes.end(), [](const Episode &a, const Episode &b) {
        if (a.season != b.season) {
            return a.season < b.season;
        }
        return a.episodeNum < b.episodeNum;
    });
    return si;
}

QVector<Series> seriesListFromJson(const QByteArray &json)
{
    QVector<Series> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) {
        return out;
    }
    const QJsonArray arr = doc.array();
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (v.isObject()) {
            out.push_back(Series::fromJson(v.toObject()));
        }
    }
    return out;
}

} // namespace qiptv
