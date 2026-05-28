#include "models/category.h"

#include "core/parse.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace kiptv {

Category Category::fromJson(const QJsonObject &o)
{
    Category c;
    c.id = parse::str(o.value(QStringLiteral("category_id")));
    c.name = parse::str(o.value(QStringLiteral("category_name")));
    c.parentId = parse::str(o.value(QStringLiteral("parent_id")));
    return c;
}

QVector<Category> categoriesFromJson(const QByteArray &json)
{
    QVector<Category> out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) {
        return out; // some servers return {} on empty; treat as no categories
    }
    const QJsonArray arr = doc.array();
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        if (v.isObject()) {
            out.push_back(Category::fromJson(v.toObject()));
        }
    }
    return out;
}

} // namespace kiptv
