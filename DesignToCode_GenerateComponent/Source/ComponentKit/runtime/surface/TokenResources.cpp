#include "TokenResources.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
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
        if (resource == ":/button/manifest.json")
            path = generatedRoot + "/button/manifest.json";
        else if (resource.startsWith(":/checkbox/") || resource.startsWith(":/forms/"))
            path = generatedRoot + '/' + resource.mid(2);
        else {
            auto m =
                QRegularExpression("^:/button/(light|dark)-(comfortable|compact)(-demo)?\\.qss$")
                    .match(resource);
            if (m.hasMatch())
                path = generatedRoot + "/button/" + m.captured(1) + '-' + m.captured(2) +
                       (m.captured(3).isEmpty() ? "/Z_PushButton.qss" : "/demo.qss");
        }
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(("Missing Token resource: " + path).toStdString());
    return f.readAll();
}
} // namespace token_resources
