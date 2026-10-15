#include "Z_CheckBox.h"
#include "IconSource.h"
#include "FocusIndicator.h"
#include <QAccessibleWidget>
#include <QEvent>
#include <QDynamicPropertyChangeEvent>
#include <QFile>
#include <QFontDatabase>
#include <QFrame>
#include <QJsonDocument>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {
const QJsonObject& figmaValues() {
    static const QJsonObject values = [] {
        QFile file(":/figma/checkbox-gallery.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Figma checkbox visual contract missing");
        for (const auto* weight : {"Regular", "Medium", "SemiBold"})
            if (QFontDatabase::addApplicationFont(QString(":/figma/fonts/Inter-%1.otf").arg(weight)) < 0)
                throw std::runtime_error("Figma checkbox Inter font missing");
        return QJsonDocument::fromJson(file.readAll()).object()["variables"].toObject();
    }();
    return values;
}
QString figmaValue(const QString& key) {
    const auto value = figmaValues().value(key).toString();
    if (value.isEmpty()) throw std::runtime_error("Figma checkbox token missing");
    return value;
}
int figmaExtent(const QString& size) {
    return figmaValue("checkbox/size/" + size + "/indicator/size").toInt();
}
class CheckBoxAccessible : public QAccessibleWidget {
  public:
    explicit CheckBoxAccessible(Z_CheckBox* w) : QAccessibleWidget(w, QAccessible::CheckBox) {}
    QAccessible::State state() const override {
        auto s = QAccessibleWidget::state();
        auto* w = static_cast<Z_CheckBox*>(widget());
        s.checkable = true;
        s.checked = w->isChecked();
        s.checkStateMixed = w->isIndeterminate();
        return s;
    }
    QString text(QAccessible::Text t) const override {
        if (t == QAccessible::Name && widget()->accessibleName().isEmpty())
            return static_cast<Z_CheckBox*>(widget())->text();
        return QAccessibleWidget::text(t);
    }
    int childCount() const override {
        return 0;
    }
    QAccessibleInterface* child(int) const override {
        return nullptr;
    }
    int indexOfChild(const QAccessibleInterface*) const override {
        return -1;
    }
    QAccessibleInterface* focusChild() const override {
        return nullptr;
    }
    QStringList actionNames() const override {
        return {QAccessibleActionInterface::toggleAction(),
                QAccessibleActionInterface::setFocusAction()};
    }
    void doAction(const QString& name) override {
        auto* w = static_cast<Z_CheckBox*>(widget());
        if (!w->isEnabled())
            return;
        if (name == QAccessibleActionInterface::toggleAction())
            w->click();
        else if (name == QAccessibleActionInterface::setFocusAction())
            w->setFocus(Qt::OtherFocusReason);
    }
    QStringList keyBindingsForAction(const QString& action) const override {
        return action == QAccessibleActionInterface::toggleAction() ? QStringList{"Space"}
                                                                    : QStringList{};
    }
};
QAccessibleInterface* accessibleFactory(const QString&, QObject* object) {
    auto* w = qobject_cast<Z_CheckBox*>(object);
    return w ? new CheckBoxAccessible(w) : nullptr;
}
QString cssColor(const QColor& c) {
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}
} // namespace
Z_CheckBox::Z_CheckBox(std::shared_ptr<const surface::Catalog> input, const QString& text,
                       QWidget* parent)
    : QCheckBox(parent), catalog(std::move(input)) {
    static const bool installed = [] {
        QAccessible::installFactory(accessibleFactory);
        return true;
    }();
    Q_UNUSED(installed);
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_CheckBox");
    setProperty("nativeQtClass", "QCheckBox");
    setObjectName("CHECKBOX_ROOT");
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);
    setTristate(false);
    face = new QFrame(this);
    face->setObjectName("INDICATOR_FACE");
    face->setProperty("checkboxPart", "FACE");
    face->setAttribute(Qt::WA_TransparentForMouseEvents);
    inset = new surface::InsetLayer(face);
    glyph = new QLabel(face);
    glyph->setObjectName("GLYPH");
    glyph->setAlignment(Qt::AlignCenter);
    glyph->setAttribute(Qt::WA_TransparentForMouseEvents);
    label = new QLabel(this);
    label->setObjectName("LABEL");
    label->setProperty("checkboxPart", "LABEL");
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setTextFormat(Qt::PlainText);
    outer = new surface::OuterShadowEffect(this);
    setGraphicsEffect(outer);
    checkIcon = IconSource::load(catalog->glyphSource(false));
    mixedIcon = IconSource::load(catalog->glyphSource(true));
    QCheckBox::setText(text);
    label->setText(text);
    ready = true;
    connect(this, &QAbstractButton::toggled, this, [this] { refresh(); });
    connect(this, &QAbstractButton::pressed, this, [this] { refresh(); });
    connect(this, &QAbstractButton::released, this, [this] { refresh(); });
    refresh();
    FocusIndicator::instance().watch(this);
}
void Z_CheckBox::setText(const QString& value) {
    QCheckBox::setText(value);
    if (ready) {
        label->setText(value);
        refresh();
    }
}
void Z_CheckBox::setIndeterminate(bool value) {
    if (mixed == value)
        return;
    mixed = value;
    refresh();
    emit indeterminateChanged(value);
    QAccessible::State change;
    change.checkStateMixed = true;
    QAccessibleStateChangeEvent e(this, change);
    QAccessible::updateAccessibility(&e);
}
void Z_CheckBox::setAppearance(const QString& id) {
    if (id != "figma-light") catalog->label(id);
    if (preset == id)
        return;
    preset = id;
    refresh();
    emit appearanceChanged(id);
}
void Z_CheckBox::setControlSize(const QString& size) {
    catalog->checkboxMetrics(size);
    if (sizeName == size)
        return;
    sizeName = size;
    refresh();
}
QString Z_CheckBox::interactionState() const {
    if (preset == "figma-light" && isEnabled()) {
        const auto forced = property("figmaPreviewState").toString().toLower();
        if (forced == "default" || forced == "hover" || forced == "pressed" || forced == "focus-visible")
            return forced;
    }
    return !isEnabled() ? "disabled" : isDown() ? "pressed" : hovering ? "hover" : "default";
}
void Z_CheckBox::nextCheckState() {
    const bool requested = !isChecked();
    if (!controlled)
        setChecked(requested);
    emit checkedChangeRequested(requested);
}
QSize Z_CheckBox::sizeHint() const {
    if (preset == "figma-light") {
        const int slot = sizeName == "lg" ? 24 : 20;
        const int gap = figmaValue("checkbox/base/default/content/gap").toInt();
        return {qMax(32, 8 + slot + (text().isEmpty() ? 0 : gap + label->fontMetrics().horizontalAdvance(text()))), 32};
    }
    auto m = catalog->checkboxMetrics(sizeName);
    const int edge = qCeil(catalog->focusWidth() + catalog->focusGap());
    const int indicator = m["indicator"].toInt();
    const int textWidth = label ? label->fontMetrics().horizontalAdvance(text()) : 0;
    return {qMax(m["hitTarget"].toInt(),
                 indicator + 2 * edge + (text().isEmpty() ? 0 : m["spacing"].toInt() + textWidth)),
            qMax(m["hitTarget"].toInt(), label ? label->fontMetrics().height() : 0)};
}
QSize Z_CheckBox::minimumSizeHint() const {
    return sizeHint();
}
QRect Z_CheckBox::indicatorRect() const {
    return face->geometry();
}
QRect Z_CheckBox::designContentRect() const {
    if (preset != "figma-light") return rect();
    const int slot = sizeName == "lg" ? 24 : 20;
    return QRect(4, (height() - slot) / 2, width() - 8, slot);
}
void Z_CheckBox::arrange() {
    if (!ready)
        return;
    if (preset == "figma-light") {
        const auto content = designContentRect();
        const int extent = figmaExtent(sizeName), slot = content.height();
        QRect indicator(content.x() + (slot - extent) / 2, (height() - extent) / 2, extent, extent);
        const int start = content.x() + slot + figmaValue("checkbox/base/default/content/gap").toInt();
        QRect labelRect(start, 0, qMax(0, width() - start - 4), height());
        if (layoutDirection() == Qt::RightToLeft) {
            indicator.moveLeft(width() - indicator.right() - 1);
            labelRect.moveLeft(4);
        }
        face->setGeometry(indicator);glyph->setGeometry(face->rect());
        label->setGeometry(labelRect);
        label->setAlignment(Qt::AlignVCenter | (layoutDirection() == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft));
        return;
    }
    auto m = catalog->checkboxMetrics(sizeName);
    int extent = m["indicator"].toInt(), edge = qCeil(catalog->focusWidth() + catalog->focusGap());
    QRect faceRect(edge, (height() - extent) / 2, extent, extent);
    if (layoutDirection() == Qt::RightToLeft)
        faceRect.moveRight(width() - edge - 1);
    face->setGeometry(faceRect);
    inset->setGeometry(face->rect());
    glyph->setGeometry(face->rect());
    const int start = extent + 2 * edge + m["spacing"].toInt();
    label->setGeometry(layoutDirection() == Qt::RightToLeft
                           ? QRect(0, 0, qMax(0, width() - start), height())
                           : QRect(start, 0, qMax(0, width() - start), height()));
    label->setAlignment(Qt::AlignVCenter |
                        (layoutDirection() == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft));
    outer->setSurfaceRect(faceRect);
}
void Z_CheckBox::refresh() {
    if (!ready)
        return;
    if (preset == "figma-light") {refreshFigma();return;}
    inset->show();outer->setEnabled(true);
    auto r = catalog->checkbox(preset, isChecked() || mixed, interactionState());
    auto m = catalog->checkboxMetrics(sizeName);
    face->setProperty("surfacePreset", preset);
    face->setProperty("selectionState", isChecked() || mixed ? "selected" : "unchecked");
    face->setProperty("interactionState", interactionState());
    face->setStyleSheet("QFrame[checkboxPart=\"FACE\"] {" + r.qss +
                        QString("border-width:%1px;").arg(r.borderWidth) + "}");
    label->setStyleSheet(QString("QLabel { color:%1; font-family:'%2'; font-size:%3px; "
                                 "background:transparent; border:none; }")
                             .arg(cssColor(r.foreground), catalog->fontFamily())
                             .arg(m["fontSize"].toInt()));
    inset->setRecipe(r);
    outer->setRecipe(r);
    const int extent = m["indicator"].toInt();
    const qreal dpr = devicePixelRatioF();
    const int physical = qCeil(extent * dpr);
    QPixmap pixmap(physical, physical);
    pixmap.fill(Qt::transparent);
    if (isChecked() || mixed) {
        QPainter p(&pixmap);
        auto source = (mixed ? mixedIcon : checkIcon).pixmap(physical, physical);
        source.setDevicePixelRatio(1);
        p.drawPixmap(pixmap.rect(), source);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pixmap.rect(), r.glyph);
    }
    pixmap.setDevicePixelRatio(dpr);
    glyph->setPixmap(pixmap);
    setMinimumSize(sizeHint());
    arrange();
    updateGeometry();
    update();
}
void Z_CheckBox::refreshFigma() {
    const auto state = interactionState();
    const bool disabled = !isEnabled();
    // Product decision: an enabled mixed state must remain identifiable as a
    // horizontal dash. The captured white-on-white mark is not an acceptable
    // runtime rendering, so the selected surface is retained for contrast.
    const bool selected = isChecked() || (mixed && !disabled);
    const QString base = "checkbox/base/";
    QString fill = figmaValue(base + (disabled ? "disabled/indicator/background" :
                                   selected ? "selected/indicator/color" : "default/indicator/background"));
    QString border = figmaValue(base + (disabled ? (isChecked() ? "disabled/indicator/selected-border-color" : "disabled/indicator/border-color") :
                                     selected ? "selected/indicator/color" : "default/indicator/border-color"));
    if (!disabled && (state == "hover" || state == "pressed")) {
        border = figmaValue(base + (selected ? state + "/indicator/color" : "selected/indicator/color"));
        if (selected) fill = border;
    }
    face->setProperty("surfacePreset", preset);
    face->setProperty("selectionState", selected ? "selected" : "unchecked");
    face->setProperty("interactionState", state);
    face->setStyleSheet(QString("QFrame[checkboxPart=\"FACE\"] {background:%1;border:%2px solid %3;border-radius:%4px;}")
        .arg(fill,figmaValue(base+"default/indicator/border-width"),border,figmaValue(base+"default/indicator/radius")));
    label->setStyleSheet(QString("QLabel {color:%1;font-family:'Inter';font-size:%2px;font-weight:500;background:transparent;border:0;}")
        .arg(figmaValue(base+(disabled?"disabled/label/color":"default/label/color")),figmaValue("typography/font-size/data/numeric")));
    inset->hide();outer->setEnabled(false);
    const int extent=figmaExtent(sizeName);const qreal dpr=devicePixelRatioF();
    QPixmap pixmap(qCeil(extent*dpr),qCeil(extent*dpr));pixmap.fill(Qt::transparent);pixmap.setDevicePixelRatio(dpr);
    if (isChecked() || mixed) {
        QPainter painter(&pixmap);painter.setRenderHint(QPainter::Antialiasing);
        const QColor color(figmaValue(disabled?"color/icon/disabled":"color/icon/invert"));
        if (mixed) {
            painter.setPen(Qt::NoPen);painter.setBrush(color);
            if (disabled) painter.drawRoundedRect(QRectF((extent-8)/2.,(extent-8)/2.,8,8),1,1);
            else painter.drawRoundedRect(QRectF((extent-10)/2.,(extent-2)/2.,10,2),1,1);
        } else {
            const int iconSize=16;
            const QIcon icon(":/figma/checkbox/check.svg");
            icon.paint(&painter,QRect((extent-iconSize)/2,(extent-iconSize)/2,iconSize,iconSize));
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);painter.fillRect(pixmap.rect(),color);
        }
    }
    glyph->setPixmap(pixmap);
    // A fixed specimen width is authoritative. Font rasterizers may round an
    // advance one pixel differently from Figma; a state refresh must not resize it.
    setMinimumSize(sizeHint().boundedTo(maximumSize()));arrange();updateGeometry();update();
}
bool Z_CheckBox::event(QEvent* e) {
    if (e->type() == QEvent::Enter)
        hovering = true;
    if (e->type() == QEvent::Leave)
        hovering = false;
    bool result = QCheckBox::event(e);
    if (ready && e->type() == QEvent::DynamicPropertyChange &&
        static_cast<QDynamicPropertyChangeEvent*>(e)->propertyName() == "figmaPreviewState")
        refresh();
    if (ready && (e->type() == QEvent::Enter || e->type() == QEvent::Leave ||
                  e->type() == QEvent::EnabledChange || e->type() == QEvent::FocusIn ||
                  e->type() == QEvent::FocusOut || e->type() == QEvent::LayoutDirectionChange ||
                  e->type() == QEvent::ScreenChangeInternal))
        refresh();
    return result;
}
void Z_CheckBox::paintEvent(QPaintEvent*) {
    if (preset == "figma-light") {
        if (isEnabled() && (interactionState() == "focus-visible" || FocusIndicator::instance().visibleFor(this))) {
            QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.setBrush(Qt::NoBrush);
            const int ringWidth=figmaValue("checkbox/base/focused/focus-ring/width").toInt();
            painter.setPen(QPen(QColor(figmaValue("checkbox/base/focused/focus-ring/color")),ringWidth));
            painter.drawRoundedRect(QRectF(face->geometry()).adjusted(-3,-3,3,3),7,7);
        }
        return;
    }
    if (FocusIndicator::instance().visibleFor(this)) {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(FocusIndicator::instance().color(catalog->focusColor()), catalog->focusWidth()));
        const qreal gap = catalog->focusGap() + catalog->focusWidth() / 2;
        auto rect = QRectF(face->geometry()).adjusted(-gap, -gap, gap, gap);
        auto r = catalog->checkbox(preset, isChecked() || mixed, interactionState());
        p.drawRoundedRect(rect, r.radius + gap, r.radius + gap);
    }
}
void Z_CheckBox::resizeEvent(QResizeEvent* e) {
    QCheckBox::resizeEvent(e);
    arrange();
}
} // namespace commonWidget
