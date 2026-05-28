#pragma once

#include <QJsonValue>
#include <QString>

/**
 * Defensive coercion helpers for Xtream Codes JSON.
 *
 * Xtream servers are wildly inconsistent: the same field may be an int on
 * one server and a quoted string on another ("stream_id": 5 vs "5"),
 * booleans appear as 0/1/"0"/"1"/true, and missing values show up as null,
 * "", absent keys, or even empty arrays. These helpers never throw and
 * always return the supplied default on anything unexpected.
 */
namespace kiptv::parse {

/// Coerce to string. Numbers are stringified; null/array/object -> def.
QString str(const QJsonValue &v, const QString &def = QString());

/// Coerce to int. Strings are parsed; doubles truncated; junk -> def.
int integer(const QJsonValue &v, int def = 0);

/// Coerce to 64-bit int (for unix timestamps / large ids).
qint64 integer64(const QJsonValue &v, qint64 def = 0);

/// Coerce to double. Strings parsed; junk -> def.
double number(const QJsonValue &v, double def = 0.0);

/// Coerce to bool. Accepts true/false, 1/0, "1"/"0", "true"/"false".
bool boolean(const QJsonValue &v, bool def = false);

} // namespace kiptv::parse
