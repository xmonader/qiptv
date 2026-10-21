#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace qiptv {

/// A live TV channel from get_live_streams.
struct LiveStream {
    int streamId = 0;
    int num = 0;
    QString name;
    QString streamIcon;     ///< logo URL
    QString epgChannelId;
    QString categoryId;
    bool tvArchive = false; ///< catch-up available
    qint64 added = 0;       ///< unix timestamp

    static LiveStream fromJson(const QJsonObject &o);
};

/// Parse a get_live_streams response (JSON array).
QVector<LiveStream> liveStreamsFromJson(const QByteArray &json);

} // namespace qiptv
