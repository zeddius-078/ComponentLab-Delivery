#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QCheckBox>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    commonWidget::initializeNativeComponentResources();
    const QString qss = commonWidget::nativeComponentStyleSheet();
    if (qss.isEmpty() || !qss.contains(QStringLiteral("HarmonyOS Sans")) ||
        !qss.contains(QStringLiteral("indicator:indeterminate")))
        return 1;

    QWidget owner;
#define VERIFY_NATIVE_WRAPPER(QT_CLASS, SDK_CLASS) \
    { \
        commonWidget::SDK_CLASS instance(&owner); \
        if (!instance.property("designSystemNative").toBool() || \
            instance.property("nativeQtClass").toString() != QStringLiteral(#QT_CLASS) || \
            instance.property("designSystemClass").toString() != QStringLiteral("commonWidget::" #SDK_CLASS)) \
            return 2; \
    }
    Z_NATIVE_COMPONENT_TYPES(VERIFY_NATIVE_WRAPPER)
#undef VERIFY_NATIVE_WRAPPER

    commonWidget::Z_CheckBox checkbox(&owner);
    checkbox.setTristate(true);
    checkbox.setCheckState(Qt::PartiallyChecked);
    if (checkbox.checkState() != Qt::PartiallyChecked)
        return 3;

    // Application ownership is explicit: the package supplies a QSS string,
    // while a consuming product decides where and when to apply it.
    application.setStyleSheet(qss);
    return application.styleSheet().isEmpty() ? 4 : 0;
}
