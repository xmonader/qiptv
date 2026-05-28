#include "models/vodstream.h"

#include "core/parse.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace kiptv {

namespace {
/// stream_icon and cover are interchangeable poster fields; prefer whichever
/// is present.
QString posterOf(const QJsonObject &o)
{
    const QString icon = parse::str(o.value(QStringLiteral("stream_icon")));
    if (!icon.isEmpty()) {
        return icon;
    }
    return parse::str(o.value(QStringLiteral("cover")));
}

/// backdrop_path arrives as a string OR an array of URLs depending on server.
QString firstBackdrop(const QJsonValue &v)
{
    if (v.isArray()) {
        const QJsonArray a = v.toArray();
        return a.isEmpty() ? QString() : parse::str(a.first());
    }
    return parse::str(v);
}
} // namespace

VodStream VodStream::fromJson(const QJsonObject &o)
{
    VodStream s;
    s.streamId = parse::integer(o.value(QStringLiteral("stream_id")));
    s.num = parse::integer(o.value(QStringLiteral("num")));
    s.name = parse::str(o.value(QStringLiteral("name")));
    s.icon = posterOf(o);
    s.categoryId = parse::str(o.value(QStringLiteral("category_id")));
    s.containerExtension = parse::str(o.value(QStringLiteral("container_extension")));
    s.rating = parse::number(o.value(QStringLiteral("rating")));
    s.added = parse::integer64(o.value(QStringLiteral("added")));
    return s;
}

QVector<VodStream> vodStreamsFromJson(const QByteArray &json)
{
    QVector<VodStream> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) {
        return out;
    }
    const QJsonArray arr = doc.array();
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (v.isObject()) {
            out.push_back(VodStream::fromJson(v.toObject()));
        }
    }
    return out;
}

VodInfo VodInfo::fromJson(const QByteArray &json)
{
    VodInfo v;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return v;
    }
    const QJsonObject root = doc.object();
    const QJsonObject info = root.value(QStringLiteral("info")).toObject();
    const QJsonObject data = root.value(QStringLiteral("movie_data")).toObject();

    v.streamId = parse::integer(data.value(QStringLiteral("stream_id")));
    v.name = parse::str(data.value(QStringLiteral("name")));
    if (v.name.isEmpty()) {
        v.name = parse::str(info.value(QStringLiteral("name")));
    }
    v.containerExtension = parse::str(data.value(QStringLiteral("container_extension")));
    v.plot = parse::str(info.value(QStringLiteral("plot")));
    v.cast = parse::str(info.value(QStringLiteral("cast")));
    v.director = parse::str(info.value(QStringLiteral("director")));
    v.genre = parse::str(info.value(QStringLiteral("genre")));
    v.releaseDate = parse::str(info.value(QStringLiteral("releasedate")));
    if (v.releaseDate.isEmpty()) {
        v.releaseDate = parse::str(info.value(QStringLiteral("release_date")));
    }
    v.duration = parse::str(info.value(QStringLiteral("duration")));
    v.coverBig = parse::str(info.value(QStringLiteral("cover_big")));
    if (v.coverBig.isEmpty()) {
        v.coverBig = parse::str(info.value(QStringLiteral("movie_image")));
    }
    v.backdropPath = firstBackdrop(info.value(QStringLiteral("backdrop_path")));
    v.youtubeTrailer = parse::str(info.value(QStringLiteral("youtube_trailer")));
    v.rating = parse::number(info.value(QStringLiteral("rating")));
    return v;
}

} // namespace kiptv
