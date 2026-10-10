#pragma once
#include <QByteArray>
#include <QString>

namespace token_resources {
// Set once before constructing widgets. An override is a complete generated directory.
void setGeneratedRoot(const QString& directory);
QString generatedDirectory();
QByteArray read(const QString& resource);
} // namespace token_resources
