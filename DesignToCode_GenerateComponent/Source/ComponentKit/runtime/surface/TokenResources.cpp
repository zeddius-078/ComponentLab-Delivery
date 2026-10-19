#include "TokenResources.h"
#include <QDir>
#include <QFile>
#include <stdexcept>

namespace token_resources {
namespace {
QString generatedRoot;
}
void setGeneratedRoot(const QString& directory) {
    if (!directory.isEmpty() && !QDir(directory).exists())
        throw std::invalid_argument("Generated Token directory does not exist");
    generatedRoot = directory.isEmpty() ? QString() : QDir(directory).absolutePath();
}
QString generatedDirectory() {
    return generatedRoot;
}
QByteArray read(const QString& resource) {
    QString path = resource;
    if (!generatedRoot.isEmpty()) {
        if (resource.startsWith(":/checkbox/"))
            path = generatedRoot + '/' + resource.mid(2);
        else if (resource == ":/tokens/tokens_qt.json")
            path = QDir(generatedRoot).absoluteFilePath(
                "../DesignSystem/generated/runtime/tokens_qt.json");
        else if (resource == ":/composites/qt.json")
            path = QDir(generatedRoot).absoluteFilePath(
                "../DesignSystem/generated/composites/qt.json");
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(("Missing Token resource: " + path).toStdString());
    return f.readAll();
}
} // namespace token_resources
