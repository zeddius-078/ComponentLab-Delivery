#include "IconAssetCatalog.h"

#include <QFile>
#include <stdexcept>

namespace commonWidget {
namespace {

const QList<IconAssetDescriptor>& catalog() {
    static const QList<IconAssetDescriptor> value{
        {QStringLiteral("icon.action.previous"), QStringLiteral(":/icons/arrow-left.svg"),
         QString::fromUtf8("返回"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
        {QStringLiteral("icon.action.next"), QStringLiteral(":/icons/arrow-right.svg"),
         QString::fromUtf8("前进"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
        {QStringLiteral("icon.action.close"), QStringLiteral(":/icons/x.svg"),
         QString::fromUtf8("关闭"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
        {QStringLiteral("icon.action.search"), QStringLiteral(":/icons/search.svg"),
         QString::fromUtf8("搜索"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
        {QStringLiteral("icon.visibility.show"), QStringLiteral(":/icons/eye.svg"),
         QString::fromUtf8("显示内容"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
        {QStringLiteral("icon.visibility.hide"), QStringLiteral(":/icons/eye-off.svg"),
         QString::fromUtf8("隐藏内容"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
        {QStringLiteral("icon.navigation.expand"), QStringLiteral(":/icons/chevron-down.svg"),
         QString::fromUtf8("展开"), QStringLiteral("SVG"), QStringLiteral("Lucide"),
         QStringLiteral("ISC / licenses/Lucide-LICENSE.txt"), {16, 20, 24},
         {QStringLiteral("Primary"), QStringLiteral("Secondary")}, true},
    };
    return value;
}

[[noreturn]] void unmapped(const QString& id) {
    throw std::invalid_argument(
        QStringLiteral("UNMAPPED_RESOURCE: %1").arg(id).toStdString());
}

}

QList<IconAssetDescriptor> IconAssetCatalog::descriptors() { return catalog(); }

bool IconAssetCatalog::isSemanticId(const QString& value) {
    return value.trimmed().startsWith(QStringLiteral("icon."));
}

bool IconAssetCatalog::contains(const QString& id) {
    const QString key = id.trimmed();
    for (const auto& item : catalog())
        if (item.id == key)
            return true;
    return false;
}

const IconAssetDescriptor& IconAssetCatalog::descriptor(const QString& id) {
    const QString key = id.trimmed();
    for (const auto& item : catalog())
        if (item.id == key)
            return item;
    unmapped(key);
}

QString IconAssetCatalog::resolve(const QString& id) {
    const auto& item = descriptor(id);
    if (!QFile::exists(item.qrcPath))
        unmapped(id);
    return item.qrcPath;
}

}
