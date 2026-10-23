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

    // A Z_* wrapper must preserve the useful Qt constructor surface rather
    // than forcing every consumer through the QWidget* overload.
    commonWidget::Z_PushButton labelled(QStringLiteral("Confirm"), &owner);
    commonWidget::Z_Menu titledMenu(QStringLiteral("Actions"), &owner);
    commonWidget::Z_TableWidget grid(2, 3, &owner);
    if (labelled.text().isEmpty() || titledMenu.title().isEmpty() ||
        grid.rowCount() != 2 || grid.columnCount() != 3)
        return 5;
    if (labelled.property("designSystemClass").toString() !=
        QStringLiteral("commonWidget::Z_PushButton"))
        return 4;

    commonWidget::Z_CheckBox checkbox(&owner);
    checkbox.setTristate(true);
    checkbox.setCheckState(Qt::PartiallyChecked);
    if (checkbox.checkState() != Qt::PartiallyChecked)
        return 3;

    // Application ownership is explicit: the package supplies a QSS string,
    // while a consuming product decides where and when to apply it.
    application.setStyleSheet(qss);
    return application.styleSheet().isEmpty() ? 5 : 0;
}
