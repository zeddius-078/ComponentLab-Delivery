#pragma once
#include <QJsonObject>
#include <QMap>
#include <QWidget>

namespace design_mapping {
QByteArray read(const QString& path);
QJsonObject parse(const QByteArray& json);
QJsonObject componentMap();
QJsonObject example();
QStringList validate(const QJsonObject& contract, const QString& profile);
QMap<QString, QByteArray> generate(const QJsonObject& contract, const QString& profile);
QString exportFiles(const QJsonObject& contract, const QString& profile,
                    const QString& parentDirectory);
QWidget* preview(const QJsonObject& contract, const QString& profile, QWidget* parent = nullptr);
void mount(QWidget* root, const QJsonObject& contract, const QString& profile);
QString pageStyle();
} // namespace design_mapping
