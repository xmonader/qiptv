#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace qiptv {

/// A content category (live / VOD / series share this shape).
struct Category {
    QString id;
    QString name;
    QString parentId;

    static Category fromJson(const QJsonObject &o);
};

/// Parse a get_*_categories response (a JSON array of category objects).
QVector<Category> categoriesFromJson(const QByteArray &json);

} // namespace qiptv
