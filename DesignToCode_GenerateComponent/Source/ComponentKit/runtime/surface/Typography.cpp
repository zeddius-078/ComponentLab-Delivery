#include "Typography.h"

#include <QFontDatabase>
#include <stdexcept>

namespace typography {
namespace {
void registerFont(const QString& resource, const QString& expectedFamily) {
    const auto id = QFontDatabase::addApplicationFont(resource);
    if (id < 0 || !QFontDatabase::applicationFontFamilies(id).contains(expectedFamily))
        throw std::runtime_error(
            QString("Cannot register required font family %1 from %2")
                .arg(expectedFamily, resource)
                .toStdString());
}
} // namespace

const QString& globalFamily() {
    static const QString value = QStringLiteral("HarmonyOS Sans");
    return value;
}

const QString& simplifiedChineseFamily() {
    static const QString value = QStringLiteral("HarmonyOS Sans SC");
    return value;
}

QStringList applicationFamilies() {
    // Global UI text uses HarmonyOS Sans. Simplified-Chinese glyphs fall back
    // to the SC family; component code must not reverse this order.
    return {globalFamily(), simplifiedChineseFamily()};
}

void installApplicationFonts() {
    static const bool installed = [] {
        registerFont(QStringLiteral(":/fonts/HarmonyOS_Sans_Regular.ttf"), globalFamily());
        registerFont(QStringLiteral(":/fonts/HarmonyOS_Sans_SC_Regular.ttf"),
                     simplifiedChineseFamily());
        return true;
    }();
    Q_UNUSED(installed)
}

} // namespace typography
