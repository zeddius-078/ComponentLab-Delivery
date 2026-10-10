#pragma once
#include "token_inputs.h"
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <cmath>

namespace contract_values {
inline void require(bool ok, const QString& message) {
    if (!ok)
        throw std::runtime_error(message.toStdString());
}
inline QByteArray read(const QString& file) {
    QFile f(file);
    require(f.open(QIODevice::ReadOnly), "Cannot read " + file);
    return f.readAll();
}
inline QJsonObject json(const QString& file) {
    QJsonParseError e;
    auto d = QJsonDocument::fromJson(read(file), &e);
    require(e.error == QJsonParseError::NoError && d.isObject(), "Invalid JSON " + file);
    return d.object();
}
inline QJsonValue bind(const QJsonValue& v, const token_input::Values& tokens) {
    if (v.isString()) {
        auto m = QRegularExpression("^\\{([^{}]+)\\}$").match(v.toString());
        if (m.hasMatch()) {
            require(tokens.contains(m.captured(1)), "Missing canonical Token: " + m.captured(1));
            return tokens[m.captured(1)];
        }
    }
    if (v.isObject()) {
        auto o = v.toObject();
        for (auto i = o.begin(); i != o.end(); ++i)
            i.value() = bind(i.value(), tokens);
        return o;
    }
    if (v.isArray()) {
        QJsonArray a;
        for (auto x : v.toArray())
            a.append(bind(x, tokens));
        return a;
    }
    return v;
}
inline double number(const QJsonValue& v, const QString& name, double low, double high) {
    require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= low &&
                v.toDouble() <= high,
            "Invalid numeric contract field " + name);
    return v.toDouble();
}
inline QString n(double v) {
    return QString::number(v, 'g', 12);
}
inline QString color(const QJsonValue& v) {
    auto s = v.toString();
    QColor c(s);
    auto m =
        QRegularExpression(
            "^rgba\\(\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*\\)$")
            .match(s);
    if (m.hasMatch()) {
        c = QColor(int(number(m.captured(1).toDouble(), "red", 0, 255)),
                   int(number(m.captured(2).toDouble(), "green", 0, 255)),
                   int(number(m.captured(3).toDouble(), "blue", 0, 255)));
        c.setAlphaF(number(m.captured(4).toDouble(), "alpha", 0, 1));
    }
    require(c.isValid(), "Invalid color " + s);
    return c.alpha() == 255 ? c.name()
                            : QString("rgba(%1, %2, %3, %4)")
                                  .arg(c.red())
                                  .arg(c.green())
                                  .arg(c.blue())
                                  .arg(c.alpha());
}
inline void output(const QString& file, const QByteArray& bytes, bool check) {
    if (QFile::exists(file) && read(file) == bytes)
        return;
    require(!check, "Generated drift: " + file);
    QDir().mkpath(QFileInfo(file).absolutePath());
    QSaveFile f(file);
    require(f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size() && f.commit(),
            "Cannot write " + file);
}
} // namespace contract_values
