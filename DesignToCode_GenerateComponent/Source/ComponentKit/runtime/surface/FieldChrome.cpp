#include "FieldChrome.h"
#include "FocusIndicator.h"
#include "Surface.h"
#include "TokenResources.h"
#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QPainter>
#include <QTimer>
#include <QtMath>
#include <stdexcept>

namespace field {
namespace {
const QStringList kCategories{"color", "layout", "number", "font", "boolean", "string", "asset"};

QString canonicalSize(const QString& value) {
    if (value == "sm" || value == "small") return "small";
    if (value == "md" || value == "medium") return "medium";
    if (value == "lg" || value == "large") return "large";
    throw std::invalid_argument("Unknown field size");
}

QString familyId(Family family) {
    return family == Family::Select ? QStringLiteral("select") : QStringLiteral("input");
}
} // namespace

QPixmap iconPixmap(const QIcon& icon, int logicalSize, qreal dpr, const QColor& color) {
    const int physical = qCeil(logicalSize * dpr);
    auto source = icon.pixmap(physical, physical);
    source.setDevicePixelRatio(1);
    QPixmap target(physical, physical);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(target.rect(), source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(target.rect(), color);
    painter.end();
    target.setDevicePixelRatio(dpr);
    return target;
}

Tokens::Tokens(const QString& profile) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(token_resources::read(":/tokens/tokens_qt.json"), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error("Invalid V2 Qt Token document");
    const auto root = document.object();
    values = root["profiles"].toObject()[profile].toObject();
    metadata = root["$meta"].toObject();
    profileName = profile;
    if (values.isEmpty())
        throw std::invalid_argument("Unknown V2 field profile");
}

QJsonValue Tokens::token(const QString& path) const {
    for (const auto& category : kCategories) {
        const auto group = values[category].toObject();
        if (group.contains(path)) return group[path];
    }
    throw std::runtime_error(("Missing generated V2 field Token: " + path).toStdString());
}

bool Tokens::hasToken(const QString& path) const {
    for (const auto& category : kCategories)
        if (values[category].toObject().contains(path)) return true;
    return false;
}

qreal Tokens::scalar(const QString& key) const {
    static const QMap<QString, QString> paths{
        {"gap", "component.input.base.shared.slot.content.gap"},
        {"labelGap", "semantic.space.vertical.standard"},
        {"hintGap", "semantic.space.vertical.micro"},
        {"borderWidth", "component.input.base.appearance.outlined.slot.border.border-width"},
        {"radius", "component.input.base.shared.slot.container.border-radius"},
        {"focusWidth", "component.input.base.state.focused.slot.focus-ring.border-width"},
        {"actionSize", "component.input.base.shared.slot.browse-action.size"},
        {"actionInset", "semantic.layout.padding.block.micro"},
        {"iconSize", "component.input.base.shared.slot.search-icon.size"},
        {"hintFontSize", "semantic.typography.label.default-auxiliary.font-size"},
        {"labelFontSize", "semantic.typography.label.default.font-size"},
        {"optionHeight", "semantic.interaction-size.default"},
        {"popupPadding", "semantic.layout.padding.inline.micro"},
        {"popupGap", "semantic.space.vertical.micro"},
        {"popupRadius", "component.select.base.shared.slot.popup.border-radius"},
        {"tagGap", "semantic.space.horizontal.micro"},
        {"tagRadius", "component.select.base.shared.slot.tag.border-radius"},
        {"tagHeight", "semantic.interaction-size.default"},
        {"tagPadding", "semantic.layout.padding.inline.compact"},
    };
    static const QMap<QString, qreal> runtime{
        {"popupMaxHeight", 256}, {"popupMinWidth", 160},
        {"fieldPreferredWidth", 320}, {"fieldMinimumWidth", 120},
        {"textAreaMinRows", 2}, {"textAreaMaxRows", 5},
    };
    if (runtime.contains(key)) return runtime[key];
    if (!paths.contains(key))
        throw std::invalid_argument(("Unknown V2 field metric: " + key).toStdString());
    const auto value = token(paths[key]);
    if (!value.isDouble())
        throw std::runtime_error(("Non-numeric V2 field Token: " + paths[key]).toStdString());
    return value.toDouble();
}

int Tokens::metric(const QString& key) const { return qRound(scalar(key)); }

QJsonObject Tokens::size(const QString& name, Family component) const {
    const auto sizeName = canonicalSize(name);
    const auto root = "component." + familyId(component) + ".base.size." + sizeName + ".slot.container.";
    const auto heightProperty = component == Family::Select ? "min-height" : "visual-height";
    const auto fontRoot = component == Family::Input
        ? QStringLiteral("component.input.base.typography.slot.content.")
        : QStringLiteral("component.select.base.shared.slot.content.");
    return {
        {"height", token(root + heightProperty)},
        {"paddingInline", token(root + "padding-inline")},
        {"paddingBlock", token("semantic.layout.padding.block.micro")},
        {"fontSize", token(fontRoot + "font-size")},
        {"lineHeight", token(fontRoot + "line-height")},
    };
}

QString Tokens::colorText(const QString& key) const {
    static const QMap<QString, QString> paths{
        {"background", "semantic.color.background.control-default"},
        {"text", "semantic.color.text.primary"},
        {"placeholder", "semantic.color.text.tertiary"},
        {"border", "semantic.color.border.default"},
        {"hoverBorder", "semantic.color.border.hover"},
        {"focusBorder", "semantic.color.interactive.brand.default"},
        {"focusOutline", "semantic.color.interactive.brand.focus-ring"},
        {"disabledBackground", "semantic.color.background.disabled"},
        {"disabledText", "semantic.color.text.disabled"},
        {"disabledBorder", "semantic.color.border.disabled"},
        {"errorBorder", "semantic.color.status.danger.default"},
        {"errorBackground", "semantic.color.status.danger.subtle"},
        {"muted", "semantic.color.text.muted"},
        {"optionHover", "semantic.color.background.hover"},
        {"optionSelected", "semantic.color.interactive.brand.subtle"},
        {"accent", "semantic.color.interactive.brand.default"},
        {"onAccent", "semantic.color.text.invert"},
        {"tagBackground", "semantic.color.background.surface-tertiary"},
    };
    if (!paths.contains(key))
        throw std::invalid_argument(("Unknown V2 field color: " + key).toStdString());
    return token(paths[key]).toString();
}

QColor Tokens::color(const QString& key) const { return surface::color(colorText(key)); }

QString Tokens::family() const {
    return token("semantic.typography.font-family.harmony-os-sans").toString();
}

QString Tokens::iconSource(const QString& key) const {
    static const QMap<QString, QString> sources{
        {"clear", ":/icons/x.svg"},
        {"showPassword", ":/icons/eye.svg"},
        {"hidePassword", ":/icons/eye-off.svg"},
        {"search", ":/icons/search.svg"},
        {"arrow", ":/icons/chevron-down.svg"},
    };
    if (!sources.contains(key))
        throw std::invalid_argument(("Unknown field icon source: " + key).toStdString());
    return sources[key];
}

QString Tokens::sourceHash() const {
    return metadata["commonSourceRevision"].toString() + ":" +
           metadata["platformSourceRevision"].toString();
}

Recipe Tokens::recipe(Family component, const QString& appearance, const QString& state,
                      bool open) const {
    if (appearance != "outlined")
        throw std::invalid_argument("Only the released outlined field appearance is available");
    if (!QStringList{"default", "hover", "focused", "error", "disabled", "loading"}.contains(state))
        throw std::invalid_argument("Unknown V2 field state");
    if (component == Family::Select && open && (state == "disabled" || state == "loading"))
        throw std::invalid_argument("Select cannot be open while disabled or loading");

    const auto family = familyId(component);
    const auto combination = component == Family::Select
        ? "component.select.appearance.outlined.state." + state + ".open." +
              (open ? "true" : "false") + ".slot."
        : "component.input.appearance.outlined.state." + state + ".slot.";
    const auto stateBase = "component." + family + ".base.state." + state + ".slot.";
    const auto defaultBase = "component." + family + ".base.state.default.slot.";
    const auto appearanceBase = "component." + family + ".base.appearance.outlined.slot.";
    const auto sharedBase = "component." + family + ".base.shared.slot.";
    const auto resolved = [this, &combination, &stateBase, &defaultBase,
                           &appearanceBase, &sharedBase](const QString& suffix) {
        for (const auto& prefix : {combination, stateBase, defaultBase, appearanceBase, sharedBase}) {
            const auto path = prefix + suffix;
            if (hasToken(path)) return token(path);
        }
        throw std::runtime_error(("Incomplete V2 field recipe: " + suffix).toStdString());
    };
    return {
        surface::color(resolved("container.background-color").toString()),
        surface::color(resolved("border.color").toString()),
        surface::color(resolved("content.color").toString()),
        surface::color(resolved(component == Family::Select
                                    ? "dropdown-indicator.color" : "search-icon.color").toString()),
        resolved("container.border-radius").toDouble(),
        resolved("border.border-width").toDouble(),
    };
}

ActionRecipe Tokens::actionRecipe(const QString& action, const QString& state) const {
    if (!QStringList{"clear", "password-toggle"}.contains(action))
        throw std::invalid_argument("Unknown Input action");
    if (!QStringList{"default", "hover", "pressed", "disabled"}.contains(state))
        throw std::invalid_argument("Unknown Input action state");
    const auto slot = action;
    const auto overlay = "component.input.overlay.action." + action + ".action-state." +
                         state + ".slot." + slot + ".";
    const auto shared = "component.input.base.shared.slot." + slot + ".";
    return {
        surface::color(token(overlay + "background-color").toString()),
        surface::color(token(overlay + "color").toString()),
        token(shared + "border-radius").toDouble(),
        token(shared + "size").toInt(),
        token(shared + "hit-target").toInt(),
    };
}

QString Tokens::qss(const Recipe& recipe, const QString& name, Family component) const {
    const auto metrics = size(name, component);
    return QStringLiteral(
        "QLineEdit[fieldPart=\"EDITOR\"],QPlainTextEdit[fieldPart=\"EDITOR\"]{"
        "background:transparent;border:0;padding:0;min-height:0;color:%1;"
        "font-family:'%2','HarmonyOS Sans SC';font-size:%3px;"
        "selection-background-color:%4;selection-color:%5;}"
        "QLabel{background:transparent;border:0;color:%1;font-family:'%2','HarmonyOS Sans SC';font-size:%3px;}"
        "QLabel#HINT,QLabel#COUNT{color:%6;}"
        "QLabel#COUNT[countExceeded=\"true\"]{color:%7;}"
        "QFrame#POPUP{background:%8;border:1px solid %9;border-radius:%10px;}"
        "QFrame#TAG{background:%11;border:1px solid %4;border-radius:%12px;}"
        "QLabel#EMPTY{color:%6;}")
        .arg(recipe.foreground.name(QColor::HexArgb), family())
        .arg(metrics["fontSize"].toInt())
        .arg(color("accent").name(QColor::HexArgb), color("onAccent").name(QColor::HexArgb),
             color("muted").name(QColor::HexArgb), colorText("errorBorder"),
             token("semantic.color.background.surface-raised").toString(),
             token("semantic.color.border.default").toString())
        .arg(metric("popupRadius"))
        .arg(color("tagBackground").name(QColor::HexArgb))
        .arg(metric("tagRadius"));
}

Chrome::Chrome(std::shared_ptr<const Tokens> tokens, QWidget* parent, Family family)
    : QFrame(parent), catalog(std::move(tokens)), familyName(family) {
    setObjectName("CONTROL");
    setProperty("fieldPart", "CONTROL");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setControlSize(sizeName);
    auto& focusIndicator = commonWidget::FocusIndicator::instance();
    focusIndicator.watch(this);
    connect(&focusIndicator, &commonWidget::FocusIndicator::changed,
            this, &Chrome::refresh);
}

Chrome::~Chrome() {
    disconnect(&commonWidget::FocusIndicator::instance(), nullptr, this, nullptr);
}

int Chrome::controlHeight() const {
    const auto metrics = catalog->size(sizeName, familyName);
    if (parentWidget() && parentWidget()->property("figmaDesignBounds").isValid()) {
        const auto authored = parentWidget()->property("figmaDesignBounds").toRect();
        if (authored.height() > 0)
            return authored.height();
    }
    int actionHeight = 0;
    for (auto* action : findChildren<Action*>()) {
        // Hidden actions are structural slots, not part of the current
        // control geometry.  Including their native QToolButton sizeHint
        // inflated plain inputs from the authored 46px height to 54px.
        // Use the widget's explicit visibility rather than isVisible(), which
        // also depends on whether the preview page has been shown yet.  The
        // field synchronizer hides structural actions before measuring the
        // authored control height; isHidden() preserves that intent in both
        // shown and offscreen composition tests.
        if (!action->isHidden())
            actionHeight = qMax(actionHeight,
                                qMax(action->sizeHint().height(), action->minimumHeight()));
    }
    if (actionHeight > 0) actionHeight += 2 * catalog->metric("actionInset");
    return qMax(qMax(metrics["height"].toInt(), actionHeight),
                metrics["lineHeight"].toInt() +
                    2 * (metrics["paddingBlock"].toInt() + catalog->metric("borderWidth")));
}

void Chrome::setControlHeight(int value) {
    if (parentWidget() && parentWidget()->property("figmaDesignBounds").isValid()) {
        const auto authored = parentWidget()->property("figmaDesignBounds").toRect();
        if (authored.height() > 0) {
            setFixedHeight(authored.height());
            const int authoredHeight = authored.height();
            QTimer::singleShot(0, this, [this, authoredHeight] {
                setMinimumHeight(authoredHeight);
                setMaximumHeight(authoredHeight);
                setFixedHeight(authoredHeight);
            });
            return;
        }
    }
    setFixedHeight(qMax(controlHeight(), value));
}

void Chrome::setControlSize(const QString& value) {
    catalog->size(value, familyName);
    sizeName = value;
    setControlHeight(controlHeight());
    refresh();
}

void Chrome::setStatus(const QString& value) {
    if (!value.isEmpty() && value != "error")
        throw std::invalid_argument("Unknown validation status");
    status = value;
    refresh();
}

void Chrome::setLoading(bool value) {
    if (loading == value) return;
    loading = value;
    if (loading && familyName == Family::Select) open = false;
    refresh();
}

void Chrome::setAppearance(const QString& value) {
    catalog->recipe(familyName, value, "default", false);
    appearance = value;
    refresh();
}

void Chrome::setOpenCondition(bool value) {
    if (familyName != Family::Select && value)
        throw std::logic_error("Open condition is Select-only");
    if (open == value) return;
    catalog->recipe(familyName, appearance, visualState(), value);
    open = value;
    refresh();
}

void Chrome::setSnapshotState(const QString& value) {
    if (!QStringList{"", "Default", "Hover", "Focus", "Focused", "Error", "Disabled"}.contains(value))
        throw std::invalid_argument("Unknown field preview state");
    snapshotState = value == "Focus" ? QStringLiteral("focused") : value.toLower();
    refresh();
}

QString Chrome::visualState() const {
    if (!isEnabled()) return "disabled";
    if (!snapshotState.isEmpty()) return snapshotState;
    if (!status.isEmpty()) return status;
    if (loading) return "loading";
    // Focused is a component interaction state, independent from the optional
    // focus-ring display policy.  A mouse-focused editor must still use the
    // focused field recipe even when the shared FocusIndicator is in Keyboard
    // mode (or explicitly Off); only the ring itself is policy-controlled.
    const auto* target = QApplication::focusWidget();
    if (target && (target == this || isAncestorOf(target)))
        return "focused";
    return hovered ? QStringLiteral("hover") : QStringLiteral("default");
}

void Chrome::refresh() {
    const auto state = visualState();
    catalog->recipe(familyName, appearance, state, open);
    setProperty("fieldState", state);
    setProperty("fieldAppearance", appearance);
    setProperty("fieldSize", canonicalSize(sizeName));
    setProperty("fieldFamily", familyId(familyName));
    setProperty("fieldOpen", open);
    update();
    emit visualChanged();
}

bool Chrome::event(QEvent* event) {
    if (event->type() == QEvent::DynamicPropertyChange &&
        static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName() == "focusIndicatorVisible")
        refresh();
    if (event->type() == QEvent::Enter) { hovered = true; refresh(); }
    if (event->type() == QEvent::Leave) { hovered = false; refresh(); }
    if (event->type() == QEvent::EnabledChange) refresh();
    return QFrame::event(event);
}

void Chrome::paintEvent(QPaintEvent*) {
    const auto value = catalog->recipe(familyName, appearance, visualState(), open);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(value.background);
    painter.setPen(QPen(value.border, value.borderWidth));
    const qreal inset = value.borderWidth / 2.0;
    painter.drawRoundedRect(QRectF(rect()).adjusted(inset, inset, -inset, -inset),
                            value.radius, value.radius);
}

Action::Action(std::shared_ptr<const Tokens> tokens, QWidget* parent)
    : commonWidget::Z_PushButton(parent), catalog(std::move(tokens)) {
    setProperty("fieldPart", "ACTION");
    // Actions are field chrome slots, not standalone Button specimens. Keep
    // their authored hit-target geometry when the shared Button synchronizer
    // applies the icon-only preset (which otherwise forces the 24px button
    // size onto this 32px field action).
    setProperty("figmaSnapshotGeometry", true);
    setIntent(commonWidget::ButtonIntent::Secondary);
    setAppearance(commonWidget::ButtonAppearance::Text);
    setSize(commonWidget::ButtonSize::Small);
    setLeadingIconStyle(commonWidget::IconContainerStyle::Bare);
    setAccessibleName(tr("Field action"));
    setLeadingIconSource(catalog->iconSource("clear"));
    setIconOnly(true);
    // Field actions are 32px hit targets in the V2 contract.  The underlying
    // Button small size is a visual preset and must not shrink the action slot.
    setMinimumHeight(32);
    setFixedHeight(32);
    connect(this, &commonWidget::Z_PushButton::visualStateChanged,
            this, [this] { publishInteractionState(); });
    publishInteractionState();
}

void Action::setBusy(bool on) { setLoading(on); publishInteractionState(); }
void Action::setIconSource(const QString& source) {
    setLeadingIconSource(source);
    publishInteractionState();
}

void Action::setEnabled(bool value) {
    commonWidget::Z_PushButton::setEnabled(value);
    publishInteractionState();
}

void Action::setRole(Role value) {
    if (actionRole == value) return;
    actionRole = value;
    const auto name = value == Role::Clear ? QStringLiteral("clear")
                    : value == Role::PasswordToggle ? QStringLiteral("password-toggle")
                                                    : QStringLiteral("generic");
    setProperty("fieldActionRole", name);
    setAppearance(commonWidget::ButtonAppearance::Text);
    publishInteractionState();
}

Action::InteractionState Action::interactionState() const {
    if (!isEnabled()) return InteractionState::Disabled;
    if (isDown()) return InteractionState::Pressed;
    if (underMouse()) return InteractionState::Hover;
    return InteractionState::Default;
}

QString Action::stateStyleOverride(const QString&) const {
    if (actionRole == Role::Generic) return {};
    const auto roleName = actionRole == Role::Clear ? QStringLiteral("clear")
                                                    : QStringLiteral("password-toggle");
    const auto state = interactionState();
    const auto stateName = state == InteractionState::Hover ? QStringLiteral("hover")
                         : state == InteractionState::Pressed ? QStringLiteral("pressed")
                         : state == InteractionState::Disabled ? QStringLiteral("disabled")
                                                               : QStringLiteral("default");
    const auto value = catalog->actionRecipe(roleName, stateName);
    return QStringLiteral(
        "QPushButton[fieldActionRole=\"%1\"]{background:%2;border:0;border-radius:%3px;padding:0;}"
        "QPushButton[fieldActionRole=\"%1\"] QLabel[buttonPart=\"GLYPH\"]{color:%4;}")
        .arg(roleName, value.background.name(QColor::HexArgb))
        .arg(value.radius)
        .arg(value.glyph.name(QColor::HexArgb));
}

bool Action::event(QEvent* value) {
    const auto result = commonWidget::Z_PushButton::event(value);
    if (value->type() == QEvent::Enter || value->type() == QEvent::Leave ||
        value->type() == QEvent::MouseButtonPress || value->type() == QEvent::MouseButtonRelease ||
        value->type() == QEvent::KeyPress || value->type() == QEvent::KeyRelease ||
        value->type() == QEvent::EnabledChange) {
        publishInteractionState();
        if (!stateRefreshQueued) {
            stateRefreshQueued = true;
            QTimer::singleShot(0, this, [this] {
                stateRefreshQueued = false;
                refreshState();
                publishInteractionState();
            });
        }
    }
    return result;
}

void Action::publishInteractionState() {
    const auto current = interactionState();
    setProperty("fieldActionState",
                current == InteractionState::Hover ? QStringLiteral("hover")
              : current == InteractionState::Pressed ? QStringLiteral("pressed")
              : current == InteractionState::Disabled ? QStringLiteral("disabled")
                                                      : QStringLiteral("default"));
    if (actionRole != Role::Generic) {
        const auto roleName = actionRole == Role::Clear ? QStringLiteral("clear")
                                                        : QStringLiteral("password-toggle");
        const auto value = catalog->actionRecipe(roleName, property("fieldActionState").toString());
        setFixedSize(value.hitTarget, value.hitTarget);
        setProperty("fieldActionIconSize", value.iconSize);
    }
    if (lastInteractionState != current) {
        lastInteractionState = current;
        emit interactionStateChanged(current);
    }
}

} // namespace field
