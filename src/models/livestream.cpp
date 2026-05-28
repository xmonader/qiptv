#include "models/livestream.h"

#include "core/parse.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace kiptv {

LiveStream LiveStream::fromJson(const QJsonObject &o)
{
    LiveStream s;
    s.streamId = parse::integer(o.value(QStringLiteral("stream_id")));
    s.num = parse::integer(o.value(QStringLiteral("num")));
    s.name = parse::str(o.value(QStringLiteral("name")));
    s.streamIcon = parse::str(o.value(QStringLiteral("stream_icon")));
    s.epgChannelId = parse::str(o.value(QStringLiteral("epg_channel_id")));
    s.categoryId = parse::str(o.value(QStringLiteral("category_id")));
    s.tvArchive = parse::boolean(o.value(QStringLiteral("tv_archive")));
    s.added = parse::integer64(o.value(QStringLiteral("added")));
    return s;
}

QVector<LiveStream> liveStreamsFromJson(const QByteArray &json)
{
    QVector<LiveStream> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) {
        return out;
    }
    const QJsonArray arr = doc.array();
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (v.isObject()) {
            out.push_back(LiveStream::fromJson(v.toObject()));
        }
    }
    return out;
}

} // namespace kiptv
