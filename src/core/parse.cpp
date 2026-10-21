#include "core/parse.h"

namespace qiptv::parse {

QString str(const QJsonValue &v, const QString &def)
{
    switch (v.type()) {
    case QJsonValue::String:
        return v.toString();
    case QJsonValue::Double: {
        const double d = v.toDouble();
        // Render integral doubles without a trailing ".0".
        if (d == static_cast<double>(static_cast<qint64>(d))) {
            return QString::number(static_cast<qint64>(d));
        }
        return QString::number(d);
    }
    case QJsonValue::Bool:
        return v.toBool() ? QStringLiteral("1") : QStringLiteral("0");
    default:
        return def;
    }
}

int integer(const QJsonValue &v, int def)
{
    return static_cast<int>(integer64(v, def));
}

qint64 integer64(const QJsonValue &v, qint64 def)
{
    switch (v.type()) {
    case QJsonValue::Double:
        return static_cast<qint64>(v.toDouble());
    case QJsonValue::String: {
        const QString s = v.toString().trimmed();
        bool ok = false;
        // Parse leading integer; tolerate values like "5.0" by trying double.
        const qint64 i = s.toLongLong(&ok);
        if (ok) {
            return i;
        }
        const double d = s.toDouble(&ok);
        return ok ? static_cast<qint64>(d) : def;
    }
    case QJsonValue::Bool:
        return v.toBool() ? 1 : 0;
    default:
        return def;
    }
}

double number(const QJsonValue &v, double def)
{
    switch (v.type()) {
    case QJsonValue::Double:
        return v.toDouble();
    case QJsonValue::String: {
        bool ok = false;
        const double d = v.toString().trimmed().toDouble(&ok);
        return ok ? d : def;
    }
    case QJsonValue::Bool:
        return v.toBool() ? 1.0 : 0.0;
    default:
        return def;
    }
}

bool boolean(const QJsonValue &v, bool def)
{
    switch (v.type()) {
    case QJsonValue::Bool:
        return v.toBool();
    case QJsonValue::Double:
        return v.toDouble() != 0.0;
    case QJsonValue::String: {
        const QString s = v.toString().trimmed().toLower();
        if (s == QStringLiteral("1") || s == QStringLiteral("true")) {
            return true;
        }
        if (s == QStringLiteral("0") || s == QStringLiteral("false") || s.isEmpty()) {
            return false;
        }
        return def;
    }
    default:
        return def;
    }
}

} // namespace qiptv::parse
