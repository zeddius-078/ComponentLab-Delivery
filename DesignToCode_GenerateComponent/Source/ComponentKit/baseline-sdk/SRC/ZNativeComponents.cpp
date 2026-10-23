#include <MyDesignSystem/ZNativeComponents.h>

#include <QFile>
#include <QFontDatabase>
#include <QResource>

// Keep Q_INIT_RESOURCE at global namespace: rcc generates this initializer at
// global linkage, while the public component classes live in commonWidget.
void initializeNativeComponentQrc() {
    Q_INIT_RESOURCE(z_native_components);
}

namespace commonWidget {
void initializeNativeComponentResources() {
    static const bool initialized = [] {
        initializeNativeComponentQrc();
        QFontDatabase::addApplicationFont(QStringLiteral(":/z-native/fonts/HarmonyOS_Sans.ttf"));
        QFontDatabase::addApplicationFont(QStringLiteral(":/z-native/fonts/HarmonyOS_Sans_SC.ttf"));
        return true;
    }();
    Q_UNUSED(initialized);
}

QString nativeComponentStyleSheet() {
    initializeNativeComponentResources();
    QFile qss(QStringLiteral(":/z-native/components.qss"));
    if (!qss.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(qss.readAll());
}

}  // namespace commonWidget
