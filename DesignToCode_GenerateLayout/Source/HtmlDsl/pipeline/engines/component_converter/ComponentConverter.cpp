// 把组件身份、变体属性、文字槽和资源槽转换为受控的 Qt API 调用。
#include "ComponentConverter.h"
#include "ComponentFactory.h"
#include "IconAssetCatalog.h"
#include "NavigationAppearance.h"
#include "DockWidgetAppearance.h"
#include "LabelAppearance.h"
#include "MdiAppearance.h"
#include "PageFlowAppearance.h"
#include "ProgressAppearance.h"
#include "TabAppearance.h"
#include "ToolBoxAppearance.h"
#include "Z_DesignComponents.h"
#include "Z_PushButton.h"
#include "CommandLinkAppearance.h"
#include "DialogButtonBoxAppearance.h"
#include "ToolButtonAppearance.h"
#include "LineEditAppearance.h"
#include "SliderAppearance.h"
#include "ScrollBarAppearance.h"
#include "DateTimeAppearance.h"
#include <QJsonDocument>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QDockWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStackedWidget>
#include <QWizard>
#include <QWizardPage>
#include <QToolBar>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QCommandLinkButton>
#include <QLineEdit>
#include <QSlider>
#include <QScrollBar>
#include <QTimeEdit>
#include <QTabWidget>
#include <QToolBox>
#include <QSet>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QGroupBox>
#include <QScrollArea>
#include <QSplitter>
#include <QCalendarWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QListView>
#include <QLCDNumber>
#include <QGraphicsView>
#include <QFrame>
#include <QDial>
#include <QStatusBar>
#include <stdexcept>
#include <functional>
#include "ComponentLibrary.h"
#include <QRadioButton>
#include <QColumnView>
#include <QUndoView>
#include <QTreeView>
#include <QTableView>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QFontComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QKeySequenceEdit>
#include <QOpenGLWidget>

namespace d2c {
namespace {
// 可切换组件库配置：把设计契约类名按 profile 的批量改名与逐组件覆盖解析为最终类名。
QString resolveClass(const QJsonObject& rule, const QJsonObject& profile) {
    QString cls = rule["qtClass"].toString();
    if (profile.isEmpty())
        return cls;
    const auto overrides = profile["componentOverrides"].toObject();
    const auto override = overrides[rule["designComponentName"].toString()].toObject();
    if (override.contains("qtClass"))
        cls = override["qtClass"].toString();
    for (const auto& entry : profile["classRenames"].toArray()) {
        const auto rename = entry.toObject();
        cls = cls.replace(QRegularExpression(rename["pattern"].toString()),
                          rename["replacement"].toString());
    }
    return cls;
}

// qt-native 配置下的公开 Qt 类创建表：只接受白名单类名，其余失败关闭。
bool isQtBuiltinClass(const QString& cls) {
    static const QSet<QString> names{
        "QRadioButton", "QColumnView", "QUndoView", "QTreeView", "QTableView",
        "QListWidget", "QTreeWidget", "QTableWidget", "QFontComboBox", "QDateEdit",
        "QDateTimeEdit", "QKeySequenceEdit", "QOpenGLWidget", "QDockWidget", "QMdiArea",
        "QPushButton", "QCheckBox", "QLineEdit", "QLabel", "QSlider", "QScrollBar",
        "QTimeEdit", "QToolButton", "QDialogButtonBox", "QCommandLinkButton", "QSplitter",
        "QScrollArea", "QCalendarWidget", "QTextBrowser", "QTextEdit", "QPlainTextEdit",
        "QListView", "QLCDNumber", "QGraphicsView", "QFrame", "QDial", "QStatusBar",
        "QGroupBox", "QTabWidget", "QStackedWidget", "QToolBox", "QProgressBar",
        "QSpinBox", "QDoubleSpinBox", "QComboBox", "QWidget"};
    return names.contains(cls);
}

QWidget* createQtBuiltin(const QString& cls, QWidget* parent) {
    if (!isQtBuiltinClass(cls))
        throw std::runtime_error(("UNSUPPORTED_QT_NATIVE_CLASS: " + cls).toStdString());
    if (cls == "QRadioButton") return new QRadioButton(parent);
    if (cls == "QColumnView") return new QColumnView(parent);
    if (cls == "QUndoView") return new QUndoView(parent);
    if (cls == "QTreeView") return new QTreeView(parent);
    if (cls == "QTableView") return new QTableView(parent);
    if (cls == "QListWidget") return new QListWidget(parent);
    if (cls == "QTreeWidget") return new QTreeWidget(parent);
    if (cls == "QTableWidget") return new QTableWidget(parent);
    if (cls == "QFontComboBox") return new QFontComboBox(parent);
    if (cls == "QDateEdit") return new QDateEdit(parent);
    if (cls == "QDateTimeEdit") return new QDateTimeEdit(parent);
    if (cls == "QKeySequenceEdit") return new QKeySequenceEdit(parent);
    if (cls == "QOpenGLWidget") return new QOpenGLWidget(parent);
    if (cls == "QDockWidget") return new QDockWidget(parent);
    if (cls == "QMdiArea") return new QMdiArea(parent);
    if (cls == "QPushButton") return new QPushButton(parent);
    if (cls == "QCheckBox") return new QCheckBox(parent);
    if (cls == "QLineEdit") return new QLineEdit(parent);
    if (cls == "QLabel") return new QLabel(parent);
    if (cls == "QSlider") return new QSlider(parent);
    if (cls == "QScrollBar") return new QScrollBar(parent);
    if (cls == "QTimeEdit") return new QTimeEdit(parent);
    if (cls == "QToolButton") return new QToolButton(parent);
    if (cls == "QDialogButtonBox") return new QDialogButtonBox(parent);
    if (cls == "QCommandLinkButton") return new QCommandLinkButton(parent);
    if (cls == "QSplitter") return new QSplitter(parent);
    if (cls == "QScrollArea") return new QScrollArea(parent);
    if (cls == "QCalendarWidget") return new QCalendarWidget(parent);
    if (cls == "QTextBrowser") return new QTextBrowser(parent);
    if (cls == "QTextEdit") return new QTextEdit(parent);
    if (cls == "QPlainTextEdit") return new QPlainTextEdit(parent);
    if (cls == "QListView") return new QListView(parent);
    if (cls == "QLCDNumber") return new QLCDNumber(parent);
    if (cls == "QGraphicsView") return new QGraphicsView(parent);
    if (cls == "QFrame") return new QFrame(parent);
    if (cls == "QDial") return new QDial(parent);
    if (cls == "QStatusBar") return new QStatusBar(parent);
    if (cls == "QGroupBox") return new QGroupBox(parent);
    if (cls == "QTabWidget") return new QTabWidget(parent);
    if (cls == "QStackedWidget") return new QStackedWidget(parent);
    if (cls == "QToolBox") return new QToolBox(parent);
    if (cls == "QProgressBar") return new QProgressBar(parent);
    if (cls == "QSpinBox") return new QSpinBox(parent);
    if (cls == "QDoubleSpinBox") return new QDoubleSpinBox(parent);
    if (cls == "QComboBox") return new QComboBox(parent);
    if (cls == "QWidget") return new QWidget(parent);
    throw std::runtime_error(("UNSUPPORTED_QT_NATIVE_CLASS: " + cls).toStdString());
}

// public-qt 策略下的公开 Qt API 直接应用；返回 false 表示该 API 不适用于当前控件类型。
bool applyQtApi(QWidget* w, const QString& api, const QJsonValue& value) {
    if (api == "setText") {
        if (auto* label = qobject_cast<QLabel*>(w)) label->setText(value.toString());
        else if (auto* button = qobject_cast<QAbstractButton*>(w)) button->setText(value.toString());
        else if (auto* edit = qobject_cast<QTextEdit*>(w)) edit->setPlainText(value.toString());
        else if (auto* browser = qobject_cast<QTextBrowser*>(w)) browser->setPlainText(value.toString());
        else return false;
        return true;
    }
    if (api == "setEnabled") { w->setEnabled(value.toBool()); return true; }
    if (api == "setReadOnly") {
        if (auto* edit = qobject_cast<QLineEdit*>(w)) edit->setReadOnly(value.toBool());
        else if (auto* text = qobject_cast<QTextEdit*>(w)) text->setReadOnly(value.toBool());
        else return false;
        return true;
    }
    if (api == "setChecked") {
        if (auto* button = qobject_cast<QAbstractButton*>(w)) button->setChecked(value.toBool());
        else return false;
        return true;
    }
    if (api == "setPlaceholderText" || api == "setPlaceholder") {
        if (auto* edit = qobject_cast<QLineEdit*>(w)) edit->setPlaceholderText(value.toString());
        else return false;
        return true;
    }
    if (api == "setCurrentIndex") {
        if (auto* tabs = qobject_cast<QTabWidget*>(w)) tabs->setCurrentIndex(value.toInt());
        else if (auto* stack = qobject_cast<QStackedWidget*>(w)) stack->setCurrentIndex(value.toInt());
        else if (auto* combo = qobject_cast<QComboBox*>(w)) combo->setCurrentIndex(value.toInt());
        else return false;
        return true;
    }
    if (api == "setValue") {
        if (auto* slider = qobject_cast<QAbstractSlider*>(w)) slider->setValue(value.toInt());
        else if (auto* spin = qobject_cast<QSpinBox*>(w)) spin->setValue(value.toInt());
        else if (auto* progress = qobject_cast<QProgressBar*>(w)) progress->setValue(value.toInt());
        else if (auto* edit = qobject_cast<QLineEdit*>(w)) edit->setText(value.toString());
        else return false;
        return true;
    }
    if (api == "setRange") {
        if (auto* progress = qobject_cast<QProgressBar*>(w)) progress->setRange(0, value.toInt());
        else return false;
        return true;
    }
    if (api == "setFormat") {
        if (auto* progress = qobject_cast<QProgressBar*>(w)) progress->setFormat(value.toString());
        else return false;
        return true;
    }
    if (api == "setTitle") {
        if (auto* group = qobject_cast<QGroupBox*>(w)) group->setTitle(value.toString());
        else return false;
        return true;
    }
    if (api == "setGrid") {
        if (auto* calendar = qobject_cast<QCalendarWidget*>(w)) calendar->setGridVisible(value.toBool());
        else return false;
        return true;
    }
    return false;
}

// 异常保留设计节点与可用属性，便于定位规则缺项；失败时不猜测替代控件。
[[noreturn]] void fail(const QString& code, const QJsonObject& node, const QString& component,
                       const QString& detail) {
    const QJsonObject error{{"code", code},
                            {"nodeId", node["guid"]},
                            {"nodeName", node["name"]},
                            {"componentName", component},
                            {"componentSetRef", node["mainStateGroup"]},
                            {"mainComponentRef", node["mainComponent"]},
                            {"detail", detail},
                            {"availableProperties", node.contains("propRefMap")
                                ? node["propRefMap"] : node["propAssignMap"]}};
    throw std::runtime_error(QJsonDocument(error).toJson(QJsonDocument::Compact).toStdString());
}

// 在指定层级按 GUID 精确查找；递归层级由调用方选择。
QJsonObject find(const QJsonArray& list, const QString& guid) {
    // 只检查调用方传入的这一层，按 GUID 返回精确引用，避免跨组件集误匹配。
    for (const auto& value : list) {
        const auto n = value.toObject();
        // 按 GUID 返回唯一目标定义，保证组件集与母版查找使用稳定身份。
        if (n["guid"].toString() == guid)
            return n;
    }
    return {};
}

// 母版文字层名称作为槽位键；重名会让覆盖来源不明确，因此直接报错。
void collectTextSlots(const QJsonObject& node, QMap<QString, QJsonObject>& out) {
    const auto name = node["name"].toString();
    // 只有文字节点登记槽名；同名槽无法唯一确定实例覆盖目标，必须拒绝。
    if (node.contains("nodeText")) {
        // 同名文字槽无法确定覆盖目标，立即报告歧义，避免后值无声覆盖前值。
        if (out.contains(name))
            throw std::runtime_error("AMBIGUOUS_TEXT_SLOT");
        out[name] = node;
    }
    // 递归收集母版内部文字槽，使嵌套标签也能接收实例覆盖。
    for (const auto& v : node["childNode"].toArray())
        collectTextSlots(v.toObject(), out);
}

// 命中图标前缀即停止下钻，避免把图标内部路径当成额外图标；顺序按 DSL 逆序读取。
void collectGlyphs(const QJsonObject& node, const QString& prefix, QJsonArray& glyphs) {
    // 按映射约定的前缀收集图标节点，使图标资源与前后插槽的职责对应。
    if (node["name"].toString().startsWith(prefix)) {
        glyphs.append(node);
        return;
    }
    const auto children = node["childNode"].toArray();
    // 按 DSL 约定逆序收集图标，保持 leading/trailing 与视觉顺序对应。
    for (int i = children.size() - 1; i >= 0; --i)
        collectGlyphs(children[i].toObject(), prefix, glyphs);
}
}

ComponentConverter::ComponentConverter(QJsonObject input, QJsonObject rules,
                                       QJsonObject libraryProfile, bool layoutOnlyMode)
    : raw(std::move(input)), mapping(std::move(rules)),
      profile(std::move(libraryProfile)),
      layoutOnly(layoutOnlyMode),
      componentFactory(std::make_unique<commonWidget::ComponentFactory>(
          mapping["profile"].toString())) {}

ComponentConverter::~ComponentConverter() = default;

QJsonObject ComponentConverter::componentFamily(const QJsonObject& instance) const {
    const auto definitions = raw["pixComponentTreeDslNodes"].toArray();
    const auto stateGroup = instance["mainStateGroup"].toString();
    const auto mainComponent = instance["mainComponent"].toString();
    if (stateGroup.isEmpty() && mainComponent.isEmpty())
        return {};
    if (!stateGroup.isEmpty()) {
        const auto family = find(definitions, stateGroup);
        if (!family.isEmpty())
            return family;
    }
    if (!mainComponent.isEmpty())
        return find(definitions, mainComponent);
    return {};
}

QJsonObject ComponentConverter::componentMaster(const QJsonObject& instance) const {
    const auto family = componentFamily(instance);
    if (family.isEmpty())
        return {};
    const auto mainComponent = instance["mainComponent"].toString();
    if (mainComponent.isEmpty())
        return family;
    // 独立公开组件的实例直接引用顶层定义；组件集实例则引用 family.childNode
    // 中的母版。两种形态都来自普通 DSL，不能按实例名称猜测。
    if (family["guid"].toString() == mainComponent)
        return family;
    return find(family["childNode"].toArray(), mainComponent);
}

QWidget* ComponentConverter::structureFallback(const QJsonObject& instance, QWidget* parent,
                                                const QString& detail) {
    const auto family = componentFamily(instance);
    auto* widget = new QWidget(parent);
    widget->setProperty("designNodeId", instance["guid"].toString());
    widget->setProperty("designComponent", family["name"].toString());
    records.append(QJsonObject{{"nodeId", instance["guid"]},
                               {"componentName", family["name"]},
                               {"qtClass", "QWidget"},
                               {"calls", QJsonArray{}},
                               {"appearance", "DEFAULT_COMPONENT_APPEARANCE"},
                               {"mappingDepth", "IDENTITY_ONLY_FALLBACK"},
                               {"warning", "UNMAPPED_COMPONENT"},
                               {"detail", detail}});
    return widget;
}

void ComponentConverter::discardTraceForNode(const QString& nodeId) {
    QJsonArray kept;
    for (const auto& entry : records)
        if (entry.toObject()["nodeId"].toString() != nodeId)
            kept.append(entry);
    records = kept;
}

QWidget* ComponentConverter::convert(const QJsonObject& n, QWidget* parent) {
    // 用公共设计身份（组件集或独立组件节点）和名称双重匹配规则，再确认实例指向的母版 key。
    const auto family = componentFamily(n);
    const auto fail = [&](const QString& code, const QJsonObject& node, const QString& detail) {
        d2c::fail(code, node, family["name"].toString(), detail);
    };
    // 组件映射依赖实例引用；普通 FRAME 或文本必须交给其他转换路径。
    if (n["type"] != "INSTANCE")
        fail("INVALID_NODE_TYPE", n, n["type"].toString());
    if (family.isEmpty() && n["mainStateGroup"].toString().isEmpty() &&
        n["mainComponent"].toString().isEmpty())
        fail("DETACHED_DSL_REQUIRES_BASE_IDENTITY", n,
             "mainStateGroup/mainComponent are empty; node name and componentName are not family identities");
    if (family.isEmpty())
        fail("UNMAPPED_COMPONENT", n,
             "component family reference is not present in pixComponentTreeDslNodes");
    QJsonObject rule;
    // 遍历声明的映射规则，以名称和 key 双重确认身份；多次命中会造成歧义。
    for (const auto& r : mapping["rules"].toArray()) {
        const auto candidate = r.toObject();
        // 公共独立组件使用 designIdentityId；组件集和旧 key 路径继续兼容。
        // 结构模式允许设计库换代（key 变化）：只按组件名确认身份，类仍须可注册。
        const bool identityMatches = layoutOnly ? true : candidate.contains("designIdentityId")
            ? candidate["designIdentityId"] == family["guid"]
            : candidate.contains("designComponentSetId")
            ? candidate["designComponentSetId"] == family["guid"]
            : candidate["designComponentKey"] == family["componentKey"];
        const auto declaredName = candidate["designComponentName"].toString();
        const auto familyName = family["name"].toString();
        const auto namePolicy = mapping["componentNameMatchPolicy"].toString();
        // 组件登记名只覆盖 ComponentLab/<组件集>/<组件身份>；身份之后的路径由属性/变体解析，
        // 不再参与组件族匹配。没有显式策略时保持历史精确匹配，避免无意放宽旧规则。
        const bool nameMatches = familyName == declaredName ||
            (namePolicy == QStringLiteral("family-identity-prefix") &&
             familyName.startsWith(declaredName + QStringLiteral("/")));
        if (identityMatches && nameMatches) {
            // 已有命中规则时再次匹配表示配置有歧义，拒绝按遍历顺序任意选取。
            if (!rule.isEmpty())
                fail("AMBIGUOUS_COMPONENT_MAPPING", n, family["name"].toString());
            rule = candidate;
        }
    }
    // 没有契约时立即报告未映射，避免使用默认按钮掩盖设计侧缺项。
    if (rule.isEmpty())
        fail("UNMAPPED_COMPONENT", n, family["name"].toString());
    const auto master = componentMaster(n);
    // 核对母版引用及 key，防止实例属性被解释为另一母版的接口参数。
    if (!layoutOnly && (master.isEmpty() || master["componentKey"] != n["componentKey"]))
        fail("INVALID_COMPONENT_REFERENCE", n, family["name"].toString());
    // 先校验属性全集、Profile 和值域，再创建控件，减少半成品对象。
    QMap<QString, QString> props;
    // 将变体列表建立为属性字典；重复名称必须报错，避免后值静默覆盖前值。
    for (const auto& p : n["propRefMap"].toObject()["variant"].toArray()) {
        const auto o = p.toObject();
        const auto name = o["name"].toString();
        if (!o["name"].isString() || !o["value"].isString())
            fail("INVALID_PROPERTY_VALUE_TYPE", n, name);
        // 重复变体属性没有明确优先级，停止转换，避免不同解析顺序得到不同状态。
        if (props.contains(name))
            fail("DUPLICATE_PROPERTY", n, name);
        props[name] = o["value"].toString();
    }
    // 限定设计 Profile 与当前运行时 profile 一致，避免混用主题 Token。
    // 设计侧别名必须在 profileAliases 中声明，并且目标必须等于当前映射使用的 SDK 上下文。
    bool profileAccepted = false;
    if (!props.contains("Profile"))
        profileAccepted = true;
    else if (props.value("Profile") == mapping["profile"].toString())
        profileAccepted = true;
    else {
        const auto designProfile = props.value("Profile");
        const auto alias = mapping["profileAliases"].toObject()[designProfile].toObject();
        // 只有别名明确指向当前 SDK 上下文时才允许转换，禁止单纯靠字符串白名单放行。
        profileAccepted = !alias.isEmpty() &&
            alias["runtimeProfile"].toString() == mapping["profile"].toString();
    }
    if (!profileAccepted)
        fail("PROFILE_MISMATCH", n, props.value("Profile"));
    const QString stateProperty = rule["stateProperty"].toString(
        QStringLiteral("state"));
    const QString compositionApi = rule["compositionApi"].toString();
    // public-qt 策略不使用 SDK 组合 API；组合属性和调用在下方按公开 API 白名单过滤。
    // Resolve the effective class before the property loop. Hybrid profiles may
    // keep the SDK class for components without a public Qt equivalent while
    // routing only whitelisted classes through the built-in constructor path.
    const auto sdkCls = rule["qtClass"].toString();
    const auto contract = library::resolve(rule, profile);
    const auto cls = contract.isEmpty() ? resolveClass(rule, profile) : contract["className"].toString();
    const auto measurementClass = contract.isEmpty() ? cls : contract["qtBase"].toString();
    const bool publicQt = profile["apiPolicy"].toString() == QStringLiteral("public-qt") &&
                          isQtBuiltinClass(measurementClass);
    if (profile["apiPolicy"].toString() == QStringLiteral("public-qt") &&
        !publicQt && cls != sdkCls)
        fail("UNSUPPORTED_QT_NATIVE_CLASS", n, cls);
    QSet<QString> allowedQtApis;
    for (const auto& entry : profile["publicQtApis"].toArray())
        allowedQtApis.insert(entry.toString());
    QJsonArray skippedCalls;
    const auto propertyRules = rule["propertyMappings"].toObject();
    // 逐项核对契约必填属性，缺少数据时在创建 QWidget 前终止。
    for (const auto& required : propertyRules.keys())
        // 每个必填映射属性都要有设计值，缺失时报告错误，防止默认值掩盖不完整设计。
        if (!props.contains(required))
            fail("MISSING_PROPERTY", n, required);
    QJsonObject values;
    QMap<QString, QString> compositionProperties;
    // 映射可显式声明无 SDK 对应 setter 的设计轴（ignoredProperties），
    // 只跳过声明过的轴并记录原因；未声明的轴仍按 UNMAPPED_PROPERTY 拒绝。
    const auto ignoredPropertyReason = [&rule](const QString& name, const QString& value) -> QString {
        for (const auto& entry : rule["ignoredProperties"].toArray()) {
            if (entry.isString() && entry.toString() == name) return QStringLiteral("declared-ignored");
            const auto object = entry.toObject();
            if (object["name"].toString() != name) continue;
            // values 为空表示忽略整个轴；否则只忽略列出的取值，其余取值仍然拒绝。
            const auto values = object["values"].toArray();
            if (values.isEmpty() || values.contains(QJsonValue(value)))
                return object["reason"].toString(QStringLiteral("declared-ignored"));
        }
        return {};
    };
    // 将一般变体翻译成 API 参数；Profile 和 state 分别由主题检查及状态规则处理。
    for (auto it = props.cbegin(); it != props.cend(); ++it) {
        // 跳过已分配给专门流程的字段，避免将状态或主题误当通用 setter。
        if (it.key() == "Profile" ||
            (it.key() == stateProperty && !propertyRules.contains(it.key())))
            continue;
        const auto spec = propertyRules[it.key()].toObject();
        const auto domain = spec["values"].toObject();
        if (!domain.contains(it.value())) {
            const auto reason = ignoredPropertyReason(it.key(), it.value());
            if (!reason.isEmpty()) {
                skippedCalls.append(QJsonObject{{"api", "ignoredProperty"},
                    {"property", it.key()}, {"value", it.value()}, {"reason", reason}});
                continue;
            }
            fail("UNMAPPED_PROPERTY", n, it.key() + "=" + it.value());
        }
        const auto api = spec["api"].toString();
        const auto mapped = domain[it.value()];
        if (!publicQt && !compositionApi.isEmpty() && api == compositionApi) {
            if (!mapped.isString())
                fail("INVALID_API_VALUE_TYPE", n, api + ":" + it.key());
            compositionProperties[it.key()] = mapped.toString();
        } else {
            values[api] = mapped;
        }
    }
    const auto states = rule["stateMappings"].toObject();
    // 只有明确映射的初始状态可以生成，未知状态不降级为 default。
    if (!states.isEmpty() && !states.contains(props.value(stateProperty)))
        fail("UNMAPPED_STATE", n, props.value(stateProperty));
    // sdkCls 是构建期实测控件使用的 SDK 类；cls 是按 profile 解析后写入交付的最终类名。
    // 转换期间由 unique_ptr 托管，后续任何 setter 失败都会释放本次控件。
    std::unique_ptr<QWidget> owned;
    using namespace commonWidget;
    Z_PushButton* button = nullptr;
    // public-qt 直接用公开 Qt 白名单表创建并测量；sdk-composition（含自用库）仍以 SDK 控件实测，
    // 交付类名由 renames/overrides 决定，调用契约要求目标库提供同名的 apply 适配。
    if (publicQt) {
        owned.reset(createQtBuiltin(measurementClass, parent));
    } else {
        if (!commonWidget::ComponentFactory::isRegistered(sdkCls))
            fail("UNREGISTERED_QT_CLASS", n, sdkCls);
        owned.reset(componentFactory->create(sdkCls, parent));
        button = qobject_cast<Z_PushButton*>(owned.get());
    }
    auto* w = owned.get();
    w->setObjectName("node_" + n["guid"].toString().replace(':', '_'));
    w->setProperty("designNodeId", n["guid"].toString());
    w->setProperty("designComponent", family["name"].toString());
    QJsonArray calls;
    if (layoutOnly) {
        // 结构模式只确认身份、类注册和布局承载；不读取或执行任何实例属性、状态、槽位、
        // 组合 API 或外观/Token setter，保留 Qt/组件构造函数提供的默认外观。
        records.append(QJsonObject{{"nodeId", n["guid"]},
                                    {"componentName", family["name"]},
                                    {"qtClass", cls},
                                    {"calls", calls},
                                    {"appearance", "DEFAULT_COMPONENT_APPEARANCE"},
                                    {"mappingDepth", "IDENTITY_AND_CLASS_ONLY"}});
        return owned.release();
    }
    if (!publicQt && !compositionApi.isEmpty()) {
        if (compositionApi == QStringLiteral("navigation.populateMenu")) {
            auto* menu = qobject_cast<QMenu*>(w);
            if (!menu)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::navigation::populateMenu(menu);
        } else if (compositionApi == QStringLiteral("navigation.populateMenuBar")) {
            auto* menuBar = qobject_cast<QMenuBar*>(w);
            if (!menuBar)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::navigation::populateMenuBar(menuBar);
        } else if (compositionApi == QStringLiteral("navigation.configureToolBar")) {
            auto* toolBar = qobject_cast<QToolBar*>(w);
            if (!toolBar)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::navigation::configureToolBar(toolBar, compositionProperties);
        } else if (compositionApi == QStringLiteral("tabs.populateTabWidget")) {
            auto* tabs = qobject_cast<QTabWidget*>(w);
            if (!tabs)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::populateFigmaTabWidget(tabs);
        } else if (compositionApi == QStringLiteral("toolbox.populateToolBox")) {
            auto* toolBox = qobject_cast<QToolBox*>(w);
            if (!toolBox)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::populateFigmaToolBox(toolBox);
        } else if (compositionApi == QStringLiteral("dock.configureDockWidget")) {
            auto* dock = qobject_cast<QDockWidget*>(w);
            if (!dock)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaDockWidget(dock, compositionProperties);
        } else if (compositionApi == QStringLiteral("mdi.configureSubWindow")) {
            auto* sub = qobject_cast<QMdiSubWindow*>(w);
            if (!sub)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaMdiSubWindow(sub, compositionProperties);
        } else if (compositionApi == QStringLiteral("mdi.configureArea")) {
            auto* area = qobject_cast<QMdiArea*>(w);
            if (!area)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaMdiArea(area, compositionProperties);
        } else if (compositionApi == QStringLiteral("pageflow.configureStack")) {
            auto* stack = qobject_cast<QStackedWidget*>(w);
            if (!stack)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaStackedWidget(stack, compositionProperties);
        } else if (compositionApi == QStringLiteral("pageflow.configureWizardPage")) {
            auto* page = qobject_cast<QWizardPage*>(w);
            if (!page)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaWizardPage(page, compositionProperties);
        } else if (compositionApi == QStringLiteral("pageflow.configureWizard")) {
            auto* wizard = qobject_cast<QWizard*>(w);
            if (!wizard)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaWizard(wizard, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureLabel")) {
            auto* label = qobject_cast<QLabel*>(w);
            if (!label)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaLabel(label, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureProgressBar")) {
            auto* progress = qobject_cast<QProgressBar*>(w);
            if (!progress)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaProgressBar(progress, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureMetricCard")) {
            auto* card = qobject_cast<commonWidget::Z_MetricCard*>(w);
            if (!card)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaMetricCard(card, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureStatistic")) {
            auto* statistic = qobject_cast<commonWidget::Z_Statistic*>(w);
            if (!statistic)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaStatistic(statistic, compositionProperties);
        } else if (compositionApi == QStringLiteral("actions.configureToolButton")) {
            auto* tool = qobject_cast<QToolButton*>(w);
            if (!tool)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaToolButton(tool, compositionProperties);
        } else if (compositionApi == QStringLiteral("actions.configureDialogButtonGroup")) {
            auto* box = qobject_cast<QDialogButtonBox*>(w);
            if (!box)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaDialogButtonGroup(box, compositionProperties);
        } else if (compositionApi == QStringLiteral("actions.configureCommandLinkButton")) {
            auto* command = qobject_cast<QCommandLinkButton*>(w);
            if (!command)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaCommandLinkButton(command, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureLineEdit")) {
            auto* edit = qobject_cast<QLineEdit*>(w);
            if (!edit)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaLineEdit(edit, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureSlider")) {
            auto* slider = qobject_cast<QSlider*>(w);
            if (!slider)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaSlider(slider, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureScrollBar")) {
            auto* bar = qobject_cast<QScrollBar*>(w);
            if (!bar)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaScrollBar(bar, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureTimePicker")) {
            auto* edit = qobject_cast<QTimeEdit*>(w);
            if (!edit)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaTimePicker(edit, compositionProperties);
        } else {
            fail("UNREGISTERED_COMPOSITION_API", n, compositionApi);
        }
        QJsonObject compositionCall{{"api", compositionApi}};
        if (!compositionProperties.isEmpty()) {
            QJsonObject propertyTrace;
            for (auto it = compositionProperties.cbegin(); it != compositionProperties.cend(); ++it)
                propertyTrace[it.key()] = it.value();
            compositionCall["properties"] = propertyTrace;
        }
        calls.append(compositionCall);
    }
    // 图标槽和图标资源只适用于按钮；其他控件沿用各自的参数路径。
    if (button) {
        // 图标来自母版 GLYPH 槽及实例名称覆盖，再通过资源表找到 QRC/本地路径。
        const auto resources = rule["resourceMappings"].toObject();
        const auto contentSlots = resources["contentSlots"].toObject();
        // 内容模式决定图标槽数量与位置，缺少规则时不能猜测 leading/trailing。
        // 轴名随设计库版本变化（content / Content），以映射解析后的 API 值为准。
        const auto contentMode = values.contains(QStringLiteral("setContentMode"))
            ? values.value(QStringLiteral("setContentMode")).toString()
            : props.value(QStringLiteral("content"));
        if (!contentSlots.contains(contentMode))
            fail("UNMAPPED_CONTENT_MODE", n, contentMode);
        const auto targets = contentSlots[contentMode].toArray();
        QJsonArray glyphs;
        collectGlyphs(master, resources["glyphPrefix"].toString(), glyphs);
        // 要求设计图标数量与内容模式的目标槽一致，避免图标错位或被丢弃。
        if (glyphs.size() != targets.size())
            fail("RESOURCE_SLOT_COUNT_MISMATCH", n, family["name"].toString());
        // 逐槽解析设计图标，再写入对应按钮接口和调用轨迹。
        for (int i = 0; i < glyphs.size(); ++i) {
            auto glyph = glyphs[i].toObject();
            // 只应用指向当前图标 GUID 的名称覆盖，保留其他图标及文字覆盖。
            for (const auto& p : n["props"].toArray()) {
                const auto o = p.toObject();
                // 只应用当前图标节点的名称覆盖，保证实例换图标时能查到正确资源。
                if (o["componentId"] == glyph["guid"] && o.contains("name"))
                    glyph["name"] = o["name"];
            }
            const auto resource =
                resources["entries"].toObject()[glyph["name"].toString()].toObject();
            // 未登记的图标没有可信 QRC 路径，停止转换并报告具体槽名。
            if (resource.isEmpty())
                fail("UNMAPPED_RESOURCE", n, glyph["name"].toString());
            QString source;
            if (resource.contains("assetId")) {
                source = resource["assetId"].toString();
                try {
                    // Validate against the GenerateComponent SDK catalog. The stable ID is
                    // passed to the component; GenerateLayout never owns the concrete path.
                    commonWidget::IconAssetCatalog::resolve(source);
                } catch (const std::exception&) {
                    fail("UNMAPPED_RESOURCE", n, source);
                }
            } else {
                // Page-owned assets continue to use their generated QRC/local source.
                source = resource["source"].toString();
            }
            if (source.isEmpty())
                fail("UNMAPPED_RESOURCE", n, glyph["name"].toString());
            const auto target = targets[i].toString();
            // 依据契约选择前或后图标接口；未知槽直接拒绝，防止落到错误位置。
            if (target == "leading")
                button->setLeadingIconSource(source);
            else if (target == "trailing")
                button->setTrailingIconSource(source);
            else
                fail("UNMAPPED_RESOURCE_SLOT", n, target);
            // 独立图标模式缺少可见文本，使用资源契约的名称补足辅助阅读语义。
            if (props.value("content") == "IconOnly")
                button->setAccessibleName(resource["accessibleName"].toString());
            calls.append(QJsonObject{
                {"api", target == "leading" ? "setLeadingIconSource" : "setTrailingIconSource"},
                {"value", source},
                {"designGlyph", glyph["name"]},
                {"designGlyphId", glyph["guid"]}});
        }
    }
    // API 调用同样由 SDK Factory 校验和分派；映射层只负责设计值域到公开 API 的翻译。
    const auto invoke = [&](const QString& api, const QJsonValue& value) {
        static const QSet<QString> booleanApis{"setEnabled", "setDanger", "setGhost",
                                               "setLoading", "setAllowClear", "setReadOnly",
                                               "setActionsVisible", "setDismissible",
                                               "setCollapsed", "setGrid", "setShowGrip",
                                               "navigation.setMenuBarCornerVisible"};
        static const QSet<QString> numericApis{"setSegmentCount", "setCurrentIndex",
                                               "setMinimum", "setMaximum",
                                               "setSingleStep"};
        const bool flexibleValueApi = api == QStringLiteral("setValue");
        if ((flexibleValueApi && !value.isString() && !value.isDouble()) ||
            (booleanApis.contains(api) && !value.isBool()) ||
            (numericApis.contains(api) && !value.isDouble()) ||
            (!flexibleValueApi && !booleanApis.contains(api) &&
             !numericApis.contains(api) && !value.isString()))
            fail("INVALID_API_VALUE_TYPE", n, api);
        // public-qt：不在白名单内的 SDK 专属 API 被跳过并记录，保持渐进式适配透明。
        if (publicQt) {
            if (!allowedQtApis.contains(api)) {
                skippedCalls.append(QJsonObject{{"api", api}, {"value", value}});
                return;
            }
            if (!applyQtApi(w, api, value))
                fail("INVALID_QT_API_TARGET", n, api);
            calls.append(QJsonObject{{"api", api}, {"value", value}});
            return;
        }
        QString error;
        if (api == QStringLiteral("navigation.setMenuBarCornerVisible")) {
            auto* menuBar = qobject_cast<QMenuBar*>(w);
            if (!menuBar)
                fail("INVALID_COMPOSITION_TARGET", n, api);
            commonWidget::navigation::setMenuBarCornerVisible(menuBar, value.toBool());
        } else if (!componentFactory->apply(w, api, value, &error)) {
            fail("UNREGISTERED_API", n, api + ": " + error);
        }
        calls.append(QJsonObject{{"api", api}, {"value", value}});
    };
    // 先应用普通属性；外观最后应用，避免兼容 setter 覆盖 V2 appearance 轴。
    for (auto it = values.begin(); it != values.end(); ++it)
        if (it.key() != QStringLiteral("setAppearance"))
            invoke(it.key(), it.value());
    // 文字按“母版默认值 → 实例 props 覆盖”求值；when 控制当前内容模式使用哪些槽。
    QMap<QString, QJsonObject> textSlots;
    collectTextSlots(master, textSlots);
    // 逐条求值文字槽规则，只为当前内容模式启用的槽调用 setter。
    for (const auto& s : rule["slotMappings"].toArray()) {
        const auto slot = s.toObject();
        bool active = true;
        const auto when = slot["when"].toObject();
        // when 的条件须全部满足；任一不满足即关闭本槽，避免给 IconOnly 写入文本。
        for (auto it = when.begin(); it != when.end(); ++it)
            // 条件值不在允许范围时使该槽规则失配，避免给当前内容模式装配不适用的槽。
            if (!it.value().toArray().contains(props.value(it.key())))
                active = false;
        // 不适用的槽直接跳过，其母版默认文字也不应进入当前实例。
        if (!active)
            continue;
        const auto definition = textSlots.value(slot["name"].toString());
        // 缺失命名槽无法定位文本来源；仅显式声明 optional 的槽可以跳过并记录。
        if (definition.isEmpty()) {
            if (slot["optional"].toBool(false)) {
                skippedCalls.append(QJsonObject{{"api", "optionalSlot"},
                    {"property", slot["name"].toString()},
                    {"reason", QStringLiteral("design provides no content for this slot")}});
                continue;
            }
            fail("MISSING_SLOT", n, slot["name"].toString());
        }
        auto value = definition[slot["field"].toString()];
        // 按文字槽 GUID 查找实例覆盖；未命中时保留母版文字。
        for (const auto& p : n["props"].toArray()) {
            const auto o = p.toObject();
            // 仅使用目标定义上对应字段的实例覆盖，未提供覆盖时保留母版默认值。
            if (o["componentId"] == definition["guid"] && o.contains(slot["field"].toString()))
                value = o[slot["field"].toString()];
        }
        auto api = slot["api"].toString();
        // 输入框文字需要按内容模式选择 value 或 placeholder 接口，不能统一写成标签。
        if (api == "editorText")
            api = values["editorTextMode"].toString();
        invoke(api, value);
    }
    // 普通属性和文字就绪后应用状态。
    // 按规则顺序应用初始状态，使禁用和 loading 覆盖普通属性初始化。
    for (const auto& s : states[props.value(stateProperty)].toArray())
        invoke(s.toObject()["api"].toString(), s.toObject()["value"]);
    // 直接调用 SDK 的 V2 appearance 轴；不再创建私有材质适配器或读取旧 Catalog。
    if (values.contains(QStringLiteral("setAppearance")))
        invoke(QStringLiteral("setAppearance"), values[QStringLiteral("setAppearance")]);
    QJsonObject record{{"nodeId", n["guid"]},
                        {"componentName", family["name"]},
                        {"qtClass", cls},
                        {"calls", calls}};
    if (!skippedCalls.isEmpty())
        record["skippedCalls"] = skippedCalls;
    if (!contract.isEmpty()) {
        record["libraryComponentId"] = contract["id"];
        record["measurementQtClass"] = measurementClass;
        record["implementationStatus"] = "planned";
    }
    records.append(record);
    return owned.release();
}
}
