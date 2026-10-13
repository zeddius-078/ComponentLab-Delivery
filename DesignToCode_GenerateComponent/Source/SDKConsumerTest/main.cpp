#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QWidget>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    component_theme::install(application, QStringLiteral("light-comfortable"));

    QWidget owner;
    commonWidget::ComponentFactory factory(QStringLiteral("light-comfortable"));
    const auto descriptors = commonWidget::ComponentFactory::descriptors();
    QSet<QString> classes;
    for (const auto& descriptor : descriptors) {
        if (descriptor.sdkClass.isEmpty() || descriptor.designComponent.isEmpty() ||
            classes.contains(descriptor.sdkClass))
            return 2;
        classes.insert(descriptor.sdkClass);
        auto* component = factory.create(descriptor.sdkClass, &owner);
        if (!component)
            return 3;
        if (descriptor.mappingKind == QStringLiteral("visual-component")) {
            if (!descriptor.sdkClass.startsWith(QStringLiteral("commonWidget::Z_")) ||
                !component->property("designSystemNative").toBool() ||
                component->property("designSystemClass").toString() != descriptor.sdkClass ||
                component->property("nativeQtClass").toString() != descriptor.qtClass)
                return 6;
            const QString metaClass = QString::fromLatin1(component->metaObject()->className());
            if (!metaClass.endsWith(descriptor.sdkClass.mid(QStringLiteral("commonWidget::").size())))
                return 7;
        }
    }
    if (descriptors.size() != 86 || classes.size() != descriptors.size())
        return 4;

    auto* lineEdit = factory.create(QStringLiteral("commonWidget::Z_LineEdit"), &owner);
    QJsonArray calls;
    calls.append(QJsonObject{{QStringLiteral("api"), QStringLiteral("setPlaceholderText")},
                             {QStringLiteral("value"), QStringLiteral("SDK consumer")}});
    factory.applyCalls(lineEdit, calls);
    if (lineEdit->property("placeholderText").toString() != QStringLiteral("SDK consumer"))
        return 5;

    auto* chart = qobject_cast<commonWidget::Z_Chart*>(
        factory.create(QStringLiteral("commonWidget::Z_Chart"), &owner));
    if (!chart)
        return 8;
    chart->setVariantTuple(QStringLiteral("Kind=Line, State=Data"));
    if (chart->variantValue(QStringLiteral("Kind")) != QStringLiteral("Line") ||
        chart->property("figmaVariant").toString() != QStringLiteral("Kind=Line, State=Data"))
        return 9;

    auto* designSwitch = qobject_cast<commonWidget::Z_Switch*>(
        factory.create(QStringLiteral("commonWidget::Z_Switch"), &owner));
    if (!designSwitch)
        return 10;
    designSwitch->setVariantTuple(
        QStringLiteral("Size=Small, Value=On, State=Focus-visible"));
    if (!designSwitch->isChecked() ||
        designSwitch->variantProperties().value(QStringLiteral("Size")).toString() !=
            QStringLiteral("Small"))
        return 11;

    auto* bottomNavigation = qobject_cast<commonWidget::Z_DesktopBottomNavigation*>(
        factory.create(QStringLiteral("commonWidget::Z_DesktopBottomNavigation"), &owner));
    if (!bottomNavigation || bottomNavigation->primaryItems().size() != 3 ||
        bottomNavigation->secondaryItems().size() != 3)
        return 12;
    bottomNavigation->setVariantTuple(QStringLiteral("IconPlacement=Start"));
    if (bottomNavigation->sizeHint() != QSize(840, 122))
        return 13;
    QString focusError;
    if (!factory.apply(bottomNavigation, QStringLiteral("setFocusRingEnabled"), false,
                       &focusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(bottomNavigation))
        return 14;
    return 0;
}

