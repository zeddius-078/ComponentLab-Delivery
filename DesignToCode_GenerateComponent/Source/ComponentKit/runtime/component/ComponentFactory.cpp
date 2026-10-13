#include "ComponentFactory.h"
#include "ButtonAppearanceAdapter.h"
#include "FocusIndicator.h"
#include "native/NativeComponents.h"
#include "native/NativeComponentRegistry.h"
#include "Z_CheckBox.h"
#include "Z_DesignComponents.h"
#include "Z_InputField.h"
#include "Z_OtpInput.h"
#include "Z_PushButton.h"
#include "Z_Select.h"
#include <QJsonObject>
#include <QMetaEnum>
#include <QMetaProperty>
#include <QMap>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {

QList<NativeComponentDescriptor> nativeDescriptors() {
    auto descriptors = nativeComponentDescriptors();
    descriptors.append({
        {QStringLiteral("semantic"), QStringLiteral("commonWidget::Z_InputField"), QStringLiteral("ComponentLab/Input"), QStringLiteral("semantic-component")},
        {QStringLiteral("semantic"), QStringLiteral("commonWidget::Z_Select"), QStringLiteral("ComponentLab/Select"), QStringLiteral("semantic-component")},
        {QStringLiteral("semantic"), QStringLiteral("commonWidget::Z_OtpInput"), QStringLiteral("ComponentLab/OTP"), QStringLiteral("semantic-component")},
        {QStringLiteral("QWidget composition"), QStringLiteral("commonWidget::Z_Navbar"), QStringLiteral("ComponentLab/Navbar"), QStringLiteral("composite")},
        {QStringLiteral("QWidget composition"), QStringLiteral("commonWidget::Z_Sidebar"), QStringLiteral("ComponentLab/Sidebar"), QStringLiteral("composite")},
        {QStringLiteral("QWidget composition"), QStringLiteral("commonWidget::Z_DesktopBottomNavigation"), QStringLiteral("ComponentLab/DesktopBottomNavigation"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_MetricCard"), QStringLiteral("ComponentLab/MetricCard"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Chart"), QStringLiteral("ComponentLab/Chart"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_ImagePanel"), QStringLiteral("ComponentLab/ImagePanel"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Empty"), QStringLiteral("ComponentLab/Empty"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Statistic"), QStringLiteral("ComponentLab/Statistic"), QStringLiteral("composite")},
        {QStringLiteral("QGridLayout composition"), QStringLiteral("commonWidget::Z_Descriptions"), QStringLiteral("ComponentLab/Descriptions"), QStringLiteral("composite")},
        {QStringLiteral("QFormLayout composition"), QStringLiteral("commonWidget::Z_Form"), QStringLiteral("ComponentLab/Form"), QStringLiteral("composite")},
        {QStringLiteral("QAbstractButton"), QStringLiteral("commonWidget::Z_Switch"), QStringLiteral("ComponentLab/Switch"), QStringLiteral("design-system")},
        {QStringLiteral("QAbstractButton"), QStringLiteral("commonWidget::Z_Chip"), QStringLiteral("ComponentLab/Chip"), QStringLiteral("design-system")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Card"), QStringLiteral("ComponentLab/Card"), QStringLiteral("design-system")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_IntegrationCard"), QStringLiteral("ComponentLab/IntegrationCard"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Divider"), QStringLiteral("ComponentLab/Divider"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Collapse"), QStringLiteral("ComponentLab/Collapse"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_OverlayDrawer"), QStringLiteral("ComponentLab/OverlayDrawer"), QStringLiteral("composite")},
        {QStringLiteral("QTableWidget"), QStringLiteral("commonWidget::Z_DataTable"), QStringLiteral("ComponentLab/DataTable"), QStringLiteral("design-system")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Alert"), QStringLiteral("ComponentLab/Alert"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Message"), QStringLiteral("ComponentLab/Message"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Notification"), QStringLiteral("ComponentLab/Notification"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Spin"), QStringLiteral("ComponentLab/Spin"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Result"), QStringLiteral("ComponentLab/Result"), QStringLiteral("composite")},
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Skeleton"), QStringLiteral("ComponentLab/Skeleton"), QStringLiteral("composite")},
    });
    return descriptors;
}

QString propertyFromSetter(const QString& api) {
    if (!api.startsWith("set") || api.size() <= 3)
        return {};
    QString property = api.mid(3);
    property[0] = property[0].toLower();
    return property;
}

bool setMetaProperty(QWidget* widget, const QString& propertyName, const QJsonValue& value,
                     QString* error) {
    const QByteArray name = propertyName.toLatin1();
    const int index = widget->metaObject()->indexOfProperty(name.constData());
    if (index < 0) {
        if (error)
            *error = QStringLiteral("Unknown Qt property: ") + propertyName;
        return false;
    }
    const QMetaProperty property = widget->metaObject()->property(index);
    if (!property.isWritable()) {
        if (error)
            *error = QStringLiteral("Read-only Qt property: ") + propertyName;
        return false;
    }
    QVariant converted = value.toVariant();
    if (property.isEnumType() && converted.type() == QVariant::String) {
        const QMetaEnum values = property.enumerator();
        const int enumValue = values.keyToValue(converted.toString().toLatin1().constData());
        if (enumValue < 0) {
            if (error)
                *error = QStringLiteral("Unknown enum value: ") + converted.toString();
            return false;
        }
        converted = enumValue;
    } else if (converted.userType() != property.userType() && !converted.convert(property.userType())) {
        if (error)
            *error = QStringLiteral("Cannot convert value for: ") + propertyName;
        return false;
    }
    if (!property.write(widget, converted)) {
        if (error)
            *error = QStringLiteral("Qt rejected property: ") + propertyName;
        return false;
    }
    return true;
}

} // namespace

ComponentFactory::ComponentFactory(const QString& profile)
    : catalog(std::make_shared<surface::Catalog>(profile)),
      fields(std::make_shared<field::Tokens>(profile)) {}

QWidget* ComponentFactory::create(const QString& name, QWidget* parent) const {
    if (name == "commonWidget::Z_PushButton") return new Z_PushButton(parent);
    if (name == "commonWidget::Z_CheckBox") return new Z_CheckBox(catalog, {}, parent);
    if (name == "commonWidget::Z_InputField") return new Z_InputField(fields, parent);
    if (name == "commonWidget::Z_Select") return new Z_Select(fields, parent);
    if (name == "commonWidget::Z_OtpInput") return new Z_OtpInput(fields, 6, parent);
    if (name == "commonWidget::Z_Navbar") return new Z_Navbar(parent);
    if (name == "commonWidget::Z_Sidebar") return new Z_Sidebar(parent);
    if (name == "commonWidget::Z_DesktopBottomNavigation") return new Z_DesktopBottomNavigation(parent);
    if (name == "commonWidget::Z_MetricCard") return new Z_MetricCard(parent);
    if (name == "commonWidget::Z_Chart") return new Z_Chart(parent);
    if (name == "commonWidget::Z_ImagePanel") return new Z_ImagePanel(parent);
    if (name == "commonWidget::Z_Empty") return new Z_Empty(parent);
    if (name == "commonWidget::Z_Statistic") return new Z_Statistic(parent);
    if (name == "commonWidget::Z_Descriptions") return new Z_Descriptions(parent);
    if (name == "commonWidget::Z_Form") return new Z_Form(parent);
    if (name == "commonWidget::Z_Switch") return new Z_Switch(parent);
    if (name == "commonWidget::Z_Chip") return new Z_Chip(parent);
    if (name == "commonWidget::Z_Card") return new Z_Card(parent);
    if (name == "commonWidget::Z_IntegrationCard") return new Z_IntegrationCard(parent);
    if (name == "commonWidget::Z_Divider") return new Z_Divider(parent);
    if (name == "commonWidget::Z_Collapse") return new Z_Collapse(parent);
    if (name == "commonWidget::Z_OverlayDrawer") return new Z_OverlayDrawer(parent);
    if (name == "commonWidget::Z_DataTable") return new Z_DataTable(parent);
    if (name == "commonWidget::Z_Alert") return new Z_Alert(parent);
    if (name == "commonWidget::Z_Message") return new Z_Message(parent);
    if (name == "commonWidget::Z_Notification") return new Z_Notification(parent);
    if (name == "commonWidget::Z_Spin") return new Z_Spin(parent);
    if (name == "commonWidget::Z_Result") return new Z_Result(parent);
    if (name == "commonWidget::Z_Skeleton") return new Z_Skeleton(parent);
    if (auto* component = createGeneratedNativeComponent(name, parent)) return component;
    throw std::runtime_error(("UNREGISTERED_COMPONENT_CLASS: " + name).toStdString());
}

bool ComponentFactory::apply(QWidget* widget, const QString& api, const QJsonValue& value,
                             QString* error) const {
    if (!widget) {
        if (error) *error = QStringLiteral("Null component");
        return false;
    }
    const QString text = value.toString();
    auto* button = qobject_cast<Z_PushButton*>(widget);
    auto* check = qobject_cast<Z_CheckBox*>(widget);
    auto* input = qobject_cast<Z_InputField*>(widget);
    if (api == "setFocusIndicatorEnabled" || api == "setFocusRingEnabled") {
        FocusIndicator::instance().setEnabledFor(widget, value.toBool());
        return true;
    }
    if (api == "setType" && button) {
        const QMap<QString, ButtonType> values{{"Primary", ButtonType::Primary}, {"Default", ButtonType::Default}, {"Dashed", ButtonType::Dashed}, {"Text", ButtonType::Text}, {"Link", ButtonType::Link}};
        if (values.contains(text)) { button->setType(values[text]); return true; }
    } else if (api == "setSize" && button) {
        const QMap<QString, ButtonSize> values{{"XS", ButtonSize::XS}, {"S", ButtonSize::S}, {"M", ButtonSize::M}, {"L", ButtonSize::L}};
        if (values.contains(text)) { button->setSize(values[text]); return true; }
    } else if (api == "setDanger" && button) { button->setDanger(value.toBool()); return true;
    } else if (api == "setGhost" && button) { button->setGhost(value.toBool()); return true;
    } else if (api == "setLoading" && button) { button->setLoading(value.toBool()); return true;
    } else if (api == "setLeadingIconSource" && button) { button->setLeadingIconSource(text); return true;
    } else if (api == "setTrailingIconSource" && button) { button->setTrailingIconSource(text); return true;
    } else if (api == "setIconStyles" && button) {
        const QMap<QString, IconContainerStyle> values{{"Bare", IconContainerStyle::Bare}, {"Filled", IconContainerStyle::Filled}, {"Outlined", IconContainerStyle::Outlined}};
        if (values.contains(text)) { button->setLeadingIconStyle(values[text]); button->setTrailingIconStyle(values[text]); return true; }
    } else if (api == "setContentMode" && button) {
        button->setIconOnly(false);
        button->setLeadingIconVisible(text == "Leading" || text == "Dual" || text == "IconOnly");
        button->setTrailingIconVisible(text == "Trailing" || text == "Dual");
        button->setIconOnly(text == "IconOnly");
        return true;
    } else if (api == "ButtonAppearanceAdapter.setAppearance" && button) {
        auto* adapter = new ButtonAppearanceAdapter(button, catalog);
        adapter->setAppearance(text);
        return true;
    } else if (api == "setControlSize" && check) { check->setControlSize(text); return true;
    } else if (api == "setAppearance" && check) { check->setAppearance(text); return true;
    } else if (api == "setCheckValue" && check) { check->setChecked(text == "checked"); check->setIndeterminate(text == "mixed"); return true;
    } else if (api == "setKind" && input) {
        const QMap<QString, Z_InputField::Kind> values{{"Text", Z_InputField::Kind::Text}, {"Password", Z_InputField::Kind::Password}, {"Search", Z_InputField::Kind::Search}, {"TextArea", Z_InputField::Kind::TextArea}};
        if (values.contains(text)) { input->setKind(values[text]); return true; }
    } else if (api == "setControlSize" && input) { input->setControlSize(text); return true;
    } else if (api == "setVariant" && input) { input->setVariant(text); return true;
    } else if (api == "setAppearance" && input) { input->setAppearance(text); return true;
    } else if (api == "setStatus" && input) { input->setStatus(text); return true;
    } else if (api == "setLabel" && input) { input->setLabel(text); return true;
    } else if (api == "setHint" && input) { input->setHint(text); return true;
    } else if (api == "setPlaceholder" && input) { input->setPlaceholder(text); return true;
    } else if (api == "setValue" && input) { input->setValue(text); return true;
    } else if (api == "setAllowClear" && input) { input->setAllowClear(value.toBool()); return true;
    } else if (api == "setReadOnly" && input) { input->setReadOnly(value.toBool()); return true;
    } else if (api == "setLoading" && input) { input->setLoading(value.toBool()); return true;
    } else if ((api == "editorTextMode" && input && (text == "setPlaceholder" || text == "setValue")) ||
               (api == "prefixPolicy" && input && text == "none")) {
        return true;
    }
    const QString propertyName = propertyFromSetter(api);
    if (!propertyName.isEmpty() && setMetaProperty(widget, propertyName, value, error))
        return true;
    if (error && error->isEmpty())
        *error = QStringLiteral("Unregistered API for component: ") + api;
    return false;
}

void ComponentFactory::applyCalls(QWidget* widget, const QJsonArray& calls) const {
    for (const auto& entry : calls) {
        const QJsonObject call = entry.toObject();
        QString error;
        if (!apply(widget, call["api"].toString(), call["value"], &error))
            throw std::runtime_error(error.toStdString());
    }
}

QList<NativeComponentDescriptor> ComponentFactory::descriptors() { return nativeDescriptors(); }

QStringList ComponentFactory::registeredClasses() {
    QStringList result;
    for (const auto& descriptor : nativeDescriptors())
        result.append(descriptor.sdkClass);
    return result;
}

bool ComponentFactory::isRegistered(const QString& sdkClass) {
    return registeredClasses().contains(sdkClass);
}

} // namespace commonWidget
