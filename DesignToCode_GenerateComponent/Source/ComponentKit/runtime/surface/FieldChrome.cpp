#include "FieldChrome.h"
#include "TokenResources.h"
#include "IconSource.h"
#include "FocusIndicator.h"
#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QPainter>
#include <QResizeEvent>
#include <QSvgRenderer>
#include <QtMath>
#include <stdexcept>

namespace field {
const QJsonObject& inputVisuals() {
    static const auto data = QJsonDocument::fromJson(
        token_resources::read(":/figma/input-visuals.json")).object();
    if (data["metrics"].toObject().isEmpty())
        throw std::runtime_error("Captured Input design evidence missing");
    return data;
}
QPixmap iconPixmap(const QIcon& icon, int logicalSize, qreal dpr, const QColor& color) {
    const int physical = qCeil(logicalSize * dpr);
    auto source = icon.pixmap(physical, physical);
    source.setDevicePixelRatio(1);
    QPixmap target(physical, physical);
    target.fill(Qt::transparent);
    QPainter p(&target);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawPixmap(target.rect(), source);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(target.rect(), color);
    p.end();
    target.setDevicePixelRatio(dpr);
    return target;
}
Tokens::Tokens(const QString& profile) {
    values = QJsonDocument::fromJson(token_resources::read(":/forms/" + profile + "/catalog.json"))
                 .object();
    styleTemplate = QString::fromUtf8(token_resources::read(":/forms/" + profile + "/Z_Field.qss"));
    if (values["recipes"].toObject().isEmpty())
        throw std::runtime_error("Empty field Token catalog");
}
qreal Tokens::scalar(const QString& key) const {
    auto m = values["metrics"].toObject();
    if (!m[key].isDouble())
        throw std::invalid_argument(("Missing field metric " + key).toStdString());
    return m[key].toDouble();
}
int Tokens::metric(const QString& key) const {
    return qRound(scalar(key));
}
QJsonObject Tokens::size(const QString& name) const {
    auto s = values["sizes"].toObject()[name].toObject();
    if (s.isEmpty())
        throw std::invalid_argument("Unknown field size");
    return s;
}
QString Tokens::colorText(const QString& key) const {
    auto p = values["palette"].toObject();
    if (!p.contains(key))
        throw std::invalid_argument(("Missing field color " + key).toStdString());
    return p[key].toString();
}
QColor Tokens::color(const QString& key) const {
    return surface::color(colorText(key));
}
QString Tokens::family() const {
    return values["fontFamily"].toString();
}
QString Tokens::iconSource(const QString& key) const {
    auto value = values["icons"].toObject()[key].toString();
    if (value.isEmpty())
        throw std::invalid_argument("Missing field icon source");
    return value;
}
QString Tokens::sourceHash() const {
    return values["sourceHash"].toString();
}
surface::Recipe Tokens::recipe(const QString& p, const QString& v, const QString& s) const {
    auto r = values["recipes"].toObject()[p + '/' + v + '/' + s].toObject();
    if (r.isEmpty())
        throw std::invalid_argument("Missing field material/variant/state recipe");
    return {r["qss"].toString(),
            surface::color(r["foreground"].toString()),
            surface::color(r["glyph"].toString()),
            r["radius"].toDouble(),
            r["borderWidth"].toDouble(),
            r["shadows"].toArray(),
            surface::color(r["glyph"].toString())};
}
QString Tokens::qss(const surface::Recipe& r, const QString& name) const {
    QString result = styleTemplate;
    const QMap<QString, QString> bindings{
        {"FACE", r.qss},
        {"TEXT", r.foreground.name(QColor::HexArgb)},
        {"FAMILY", family() + "\", \"" + values["fallbackFontFamily"].toString()},
        {"FONT", QString::number(size(name)["fontSize"].toInt())},
        {"LABEL_FONT", QString::number(metric("labelFontSize"))},
        {"HINT_FONT", QString::number(metric("hintFontSize"))},
        {"MUTED", colorText("muted")},
        {"ERROR", colorText("errorBorder")},
        {"BACKGROUND", colorText("background")},
        {"BORDER", colorText("border")},
        {"POPUP_RADIUS", QString::number(metric("popupRadius"))},
        {"OPTION_SELECTED", colorText("optionSelected")},
        {"DISABLED_TEXT", colorText("disabledText")},
        {"ACTION_SIZE", QString::number(metric("actionSize"))},
        {"TAG_BACKGROUND", colorText("tagBackground")},
        {"ACCENT", colorText("accent")},
        {"ON_ACCENT", colorText("onAccent")},
        {"ACTION_RADIUS", QString::number(metric("tagRadius"))},
        {"BORDER_WIDTH", QString::number(metric("borderWidth"))},
        {"OPTION_HOVER", colorText("optionHover")},
        {"FOCUS_BORDER", colorText("focusBorder")}};
    for (auto i = bindings.begin(); i != bindings.end(); ++i)
        result.replace("{{" + i.key() + "}}", i.value());
    if (result.contains("{{"))
        throw std::runtime_error("Unresolved field QSS binding");
    return result;
}
Chrome::Chrome(std::shared_ptr<const Tokens> tokens, QWidget* parent)
    : QFrame(parent), catalog(std::move(tokens)), inset(new surface::InsetLayer(this)),
      shadow(new surface::OuterShadowEffect(this)) {
    setObjectName("CONTROL");
    setProperty("fieldPart", "CONTROL");
    inset->setObjectName("INSET_LAYER");
    setGraphicsEffect(shadow);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setControlSize(sizeName);
    auto& focus = commonWidget::FocusIndicator::instance();
    focus.watch(this);
}
int Chrome::controlHeight() const {
    if (isFigmaInputAppearance())
        return inputVisuals()["metrics"].toObject()["sizes"].toObject()[sizeName]
            .toObject()["height"].toInt();
    const auto m = catalog->size(sizeName);
    int actionHeight = 0;
    for (auto* action : findChildren<Action*>())
        actionHeight = qMax(actionHeight, action->sizeHint().height());
    if (actionHeight > 0)
        actionHeight += 2 * catalog->metric("actionInset");
    return qMax(qMax(m["height"].toInt(), actionHeight),
                m["lineHeight"].toInt() +
                    2 * (m["paddingBlock"].toInt() + catalog->metric("borderWidth")));
}
Chrome::~Chrome() {
    // Stop focus callbacks before QWidget destroys focused child editors.
    disconnect(&commonWidget::FocusIndicator::instance(), nullptr, this, nullptr);
}
void Chrome::setControlHeight(int value) {
    setFixedHeight(qMax(controlHeight(), value));
}
void Chrome::setControlSize(const QString& value) {
    catalog->size(value);
    sizeName = value;
    setControlHeight(controlHeight());
    refresh();
}
void Chrome::setVariant(const QString& value) {
    if (!QStringList{"outlined", "filled", "borderless", "underlined"}.contains(value))
        throw std::invalid_argument("Unknown field variant");
    variant = value;
    refresh();
}
void Chrome::setStatus(const QString& value) {
    if (!value.isEmpty() && value != "error" && value != "warning")
        throw std::invalid_argument("Unknown validation status");
    status = value;
    refresh();
}
void Chrome::setAppearance(const QString& value) {
    catalog->recipe(value, variant, visualState());
    appearance = value;
    refresh();
}
void Chrome::setFigmaInputAppearance(bool on) {
    figmaInput = on;
    setControlHeight(controlHeight());
    refresh();
}
bool Chrome::isFigmaInputAppearance() const {
    return figmaInput && variant == "outlined" && status != "warning";
}
void Chrome::setSnapshotState(const QString& value) {
    if (!QStringList{"", "Default", "Hover", "Focus", "Filled", "Error", "Disabled"}.contains(value))
        throw std::invalid_argument("Unknown Input snapshot state");
    snapshotState = value.toLower();
    refresh();
}
QString Chrome::visualState() const {
    if (isFigmaInputAppearance() && isEnabled() && !snapshotState.isEmpty())
        return snapshotState == "filled" ? "default" : snapshotState;
    return !isEnabled()        ? "disabled"
           : !status.isEmpty() ? status
           : commonWidget::FocusIndicator::instance().visibleFor(this) ? "focus"
           : hovered           ? "hover"
                               : "default";
}
void Chrome::refresh() {
    const bool captured = isFigmaInputAppearance();
    setProperty("figmaInputAppearance", captured);
    setProperty("figmaInputCompatibilityFallback", figmaInput && !captured);
    if (captured) {
        setProperty("fieldState", visualState());
        setProperty("fieldVariant", variant);
        setProperty("fieldSize", sizeName);
        setProperty("surfacePreset", "figma-light");
        setStyleSheet("QFrame[fieldPart=\"CONTROL\"] {border:0;background:transparent;padding:0;}");
        inset->hide();
        surface::Recipe r;
        r.radius = inputVisuals()["metrics"].toObject()["radius"].toDouble();
        if (visualState() == "focus")
            r.shadows = QJsonArray{QJsonObject{{"kind","outer"},{"x",0},{"y",0},{"blur",0},{"spread",2},
                {"color",inputVisuals()["colors"].toObject()["focusOutline"]}}};
        shadow->setRecipe(r);
        update();
        emit visualChanged();
        return;
    }
    inset->show();
    // The optional focus stroke is drawn once in paintEvent; keep the base border underneath.
    const auto state = visualState();
    auto r = catalog->recipe(appearance, variant, state == "focus" ? "default" : state);
    setProperty("fieldState", visualState());
    setProperty("fieldVariant", variant);
    setProperty("fieldSize", sizeName);
    setProperty("surfacePreset", appearance);
    setStyleSheet(catalog->qss(r, sizeName));
    inset->setRecipe(r);
    shadow->setRecipe(r);
    inset->setGeometry(rect());
    inset->lower();
    update();
    emit visualChanged();
}
bool Chrome::event(QEvent* e) {
    if (e->type() == QEvent::DynamicPropertyChange &&
        static_cast<QDynamicPropertyChangeEvent*>(e)->propertyName() == "focusIndicatorVisible")
        refresh();
    if (e->type() == QEvent::Enter) {
        hovered = true;
        refresh();
    }
    if (e->type() == QEvent::Leave) {
        hovered = false;
        refresh();
    }
    if (e->type() == QEvent::EnabledChange)
        refresh();
    return QFrame::event(e);
}
void Chrome::resizeEvent(QResizeEvent* e) {
    QFrame::resizeEvent(e);
    inset->setGeometry(rect());
}
void Chrome::paintEvent(QPaintEvent* e) {
    if (isFigmaInputAppearance()) {
        const auto colors = inputVisuals()["colors"].toObject();
        const auto state = visualState();
        const QString background = state == "disabled" ? "disabledBackground" :
            state == "error" ? "errorBackground" : "background";
        const QString border = state == "disabled" ? "disabledBorder" : state == "error" ? "errorBorder" :
            state == "focus" ? "focusBorder" : state == "hover" ? "hoverBorder" : "border";
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(colors[background].toString()));
        painter.setPen(QPen(QColor(colors[border].toString()), 1));
        painter.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5), 3.5, 3.5);
        return;
    }
    QFrame::paintEvent(e);
    if (commonWidget::FocusIndicator::instance().visibleFor(this)) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const auto w = catalog->scalar("focusWidth");
        p.setPen(QPen(commonWidget::FocusIndicator::instance().color(
                         catalog->color(status.isEmpty() ? "focusBorder" : status + "Border")), w));
        const auto r = catalog->recipe(appearance, variant, visualState()).radius;
        const auto rect = QRectF(this->rect()).adjusted(w / 2, w / 2, -w / 2, -w / 2);
        if (variant == "underlined")
            p.drawLine(rect.bottomLeft(), rect.bottomRight());
        else
            p.drawRoundedRect(rect, qMax(0.0, r - w / 2), qMax(0.0, r - w / 2));
    }
}
Action::Action(std::shared_ptr<const Tokens> tokens, QWidget* parent)
    : commonWidget::Z_PushButton(parent) {
    setProperty("fieldPart", "ACTION");
    setType(commonWidget::ButtonType::Text);
    setSize(commonWidget::ButtonSize::XS);
    setLeadingIconStyle(commonWidget::IconContainerStyle::Bare);
    setAccessibleName(tr("Field action"));
    setLeadingIconSource(tokens->iconSource("clear"));
    setIconOnly(true);
}
void Action::setBusy(bool on) {
    setLoading(on);
}
void Action::setIconSource(const QString& source) {
    setLeadingIconSource(source);
}
void Action::setFigmaGlyph(const QString& glyph) {
    if (!glyph.isEmpty() && !QSvgRenderer(":/figma/input/" + glyph + ".svg").isValid())
        throw std::invalid_argument("Unknown captured Input action glyph");
    if (figmaGlyph == glyph)
        return;
    figmaGlyph = glyph;
    setProperty("figmaInputGlyph", glyph);
    if (!glyph.isEmpty()) {
        setFixedSize(32, 32);
        setProperty("figmaAssetBounds", QRect(9,9,16,16));
    } else {
        setMinimumSize(0,0);
        setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    }
    updateGeometry();
    update();
}
void Action::paintEvent(QPaintEvent* e) {
    if (figmaGlyph.isEmpty()) {
        commonWidget::Z_PushButton::paintEvent(e);
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QSvgRenderer svg(":/figma/input/" + figmaGlyph + ".svg");
    svg.render(&painter, QRectF(9,9,16,16));
}
} // namespace field
