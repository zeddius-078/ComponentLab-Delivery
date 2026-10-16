#include "Z_FoundationComponents.h"

#include "FocusIndicator.h"
#include "Surface.h"
#include "TokenResources.h"
#include "native/Z_DoubleSpinBox.h"
#include "native/Z_SpinBox.h"
#include <QAbstractItemModel>
#include <QAbstractSpinBox>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {

struct Tokens {
    QJsonObject color;
    QJsonObject layout;
    explicit Tokens(const QString& profile) {
        const auto document = QJsonDocument::fromJson(token_resources::read(":/tokens/tokens_qt.json"));
        const auto profiles = document.object()["profiles"].toObject();
        const auto selected = profiles[profile].toObject();
        if (selected.isEmpty())
            throw std::invalid_argument("Unknown foundation component profile");
        color = selected["color"].toObject();
        layout = selected["layout"].toObject();
    }
    QColor c(const char* key) const {
        const auto value = color[QString::fromLatin1(key)].toString();
        if (value.isEmpty())
            throw std::runtime_error("Missing foundation component color token");
        const auto rgba = QRegularExpression(
                              QStringLiteral("^rgba\\((\\d+),\\s*(\\d+),\\s*(\\d+),\\s*([0-9.]+)\\)$"))
                              .match(value);
        if (rgba.hasMatch()) {
            QColor result(rgba.captured(1).toInt(), rgba.captured(2).toInt(),
                          rgba.captured(3).toInt());
            result.setAlphaF(rgba.captured(4).toDouble());
            return result;
        }
        return surface::color(value);
    }
    int n(const char* key) const {
        const auto value = layout[QString::fromLatin1(key)];
        if (!value.isDouble())
            throw std::runtime_error("Missing foundation component layout token");
        return value.toInt();
    }
};

void identify(QWidget* widget, const char* component, const char* sdkClass) {
    widget->setProperty("designSystemClass", QString::fromLatin1(sdkClass));
    widget->setProperty("designComponent", QString::fromLatin1(component));
    widget->setProperty("runtimeImplementation", true);
    widget->setProperty("figmaVariantContract", true);
}

void requireOneOf(const QString& value, const QStringList& domain, const char* property) {
    if (!domain.contains(value))
        throw std::invalid_argument((QString::fromLatin1(property) + " has unsupported value: " + value).toStdString());
}

class TransferItemDelegate final : public QStyledItemDelegate {
public:
    explicit TransferItemDelegate(const QString& profile, QObject* parent)
        : QStyledItemDelegate(parent) {
        const Tokens tokens(profile);
        const bool dark = profile.startsWith(QStringLiteral("dark-"));
        text = tokens.c(dark ? "semantic.color.text.secondary"
                             : "base.stoken.color.text.secondary");
        disabled = tokens.c("base.stoken.color.text.disabled");
        border = tokens.c(dark ? "semantic.color.border.default"
                               : "base.stoken.color.border.secondary");
        brand = tokens.c("base.stoken.color.interactive.brand.default");
        inverse = tokens.c("base.stoken.color.text.invert");
        selected = tokens.c("base.stoken.color.interactive.brand.subtle");
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (option.state.testFlag(QStyle::State_Selected))
            painter->fillRect(option.rect, selected);

        const bool enabled = option.state.testFlag(QStyle::State_Enabled) &&
                             index.flags().testFlag(Qt::ItemIsEnabled);
        const QRect indicator(option.rect.left() + 2,
                              option.rect.center().y() - 6, 12, 12);
        const bool checkable = index.flags().testFlag(Qt::ItemIsUserCheckable);
        if (checkable) {
            const bool checked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
            painter->setPen(QPen(enabled ? (checked ? brand : border) : disabled, 1));
            if (checked)
                painter->setBrush(enabled ? brand : disabled);
            else
                painter->setBrush(QBrush(Qt::NoBrush));
            painter->drawRoundedRect(QRectF(indicator).adjusted(.5, .5, -.5, -.5), 2, 2);
            if (checked) {
                QPen checkPen(inverse, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter->setPen(checkPen);
                QPainterPath check;
                check.moveTo(indicator.left() + 3, indicator.center().y());
                check.lineTo(indicator.left() + 5, indicator.bottom() - 3);
                check.lineTo(indicator.right() - 2, indicator.top() + 3);
                painter->drawPath(check);
            }
        }

        painter->setPen(enabled ? text : disabled);
        painter->setFont(option.font);
        const int textLeft = checkable ? indicator.right() + 6 : option.rect.left() + 2;
        painter->drawText(QRect(textLeft, option.rect.top(),
                                qMax(0, option.rect.right() - textLeft), option.rect.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          index.data(Qt::DisplayRole).toString());
        painter->restore();
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override {
        if (!model || !index.flags().testFlag(Qt::ItemIsEnabled) ||
            !index.flags().testFlag(Qt::ItemIsUserCheckable))
            return false;
        const QRect indicator(option.rect.left() + 2,
                              option.rect.center().y() - 6, 12, 12);
        bool activate = false;
        if (event->type() == QEvent::MouseButtonRelease) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            activate = mouse->button() == Qt::LeftButton && indicator.contains(mouse->pos());
        } else if (event->type() == QEvent::KeyPress) {
            const auto* key = static_cast<QKeyEvent*>(event);
            activate = key->key() == Qt::Key_Space || key->key() == Qt::Key_Select;
        }
        if (!activate)
            return false;
        const Qt::CheckState next = index.data(Qt::CheckStateRole).toInt() == Qt::Checked
            ? Qt::Unchecked : Qt::Checked;
        return model->setData(index, next, Qt::CheckStateRole);
    }

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override {
        const QSize base = QStyledItemDelegate::sizeHint(option, index);
        return {base.width(), qMax(20, base.height())};
    }

private:
    QColor text;
    QColor disabled;
    QColor border;
    QColor brand;
    QColor inverse;
    QColor selected;
};

QColor toneColor(const Tokens& tokens, const QString& tone, bool neutralBadge = false) {
    if (tone == "Neutral")
        return tokens.c(neutralBadge ? "base.stoken.badge.base.default.container.background"
                                     : "base.stoken.tag.base.default.container.background");
    if (tone == "Info" || tone == "Process")
        return tokens.c("base.stoken.color.interactive.brand.default");
    if (tone == "Success")
        return tokens.c("base.stoken.color.status.success.default");
    if (tone == "Warning")
        return tokens.c("base.stoken.color.status.warning.default");
    if (tone == "Danger")
        return tokens.c("base.stoken.color.status.danger.default");
    throw std::invalid_argument("Unsupported tone");
}

QColor toneText(const Tokens& tokens, const QString& tone) {
    return tone == "Info" || tone == "Process" || tone == "Danger"
               ? tokens.c("base.stoken.color.text.invert")
               : tokens.c("base.stoken.color.text.primary");
}

QFont componentFont(int pixelSize) {
    QFont font(QStringLiteral("Noto Sans SC"));
    font.setPixelSize(pixelSize);
    font.setWeight(QFont::Medium);
    return font;
}

} // namespace

Z_Badge::Z_Badge(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    identify(this, "ComponentLab/Badge", "commonWidget::Z_Badge");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(labelValue);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void Z_Badge::setLabel(const QString& value) {
    if (labelValue == value) return;
    labelValue = value;
    setAccessibleName(value);
    updateGeometry(); update(); emit labelChanged(value);
}
void Z_Badge::setKind(const QString& value) {
    requireOneOf(value, {"Text", "Dot"}, "Badge.kind");
    if (kindValue == value) return;
    kindValue = value; updateGeometry(); update(); emit kindChanged(value);
}
void Z_Badge::setTone(const QString& value) {
    requireOneOf(value, {"Neutral", "Info", "Success", "Warning", "Danger"}, "Badge.tone");
    if (toneValue == value) return;
    toneValue = value; update(); emit toneChanged(value);
}
QSize Z_Badge::sizeHint() const {
    if (kindValue == "Dot") return {8, 8};
    const QFontMetrics metrics(componentFont(12));
    return {qMax(18, metrics.horizontalAdvance(labelValue) + 8), 18};
}
void Z_Badge::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(toneColor(tokens, toneValue, true));
    painter.drawRoundedRect(rect(), height() / 2.0, height() / 2.0);
    if (kindValue == "Text") {
        painter.setFont(componentFont(12));
        painter.setPen(toneValue == "Neutral" ? tokens.c("base.stoken.badge.base.default.label.color")
                                              : toneText(tokens, toneValue));
        painter.drawText(rect(), Qt::AlignCenter, labelValue);
    }
}

Z_StatusIndicator::Z_StatusIndicator(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    identify(this, "ComponentLab/StatusIndicator", "commonWidget::Z_StatusIndicator");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(labelValue);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}
void Z_StatusIndicator::setLabel(const QString& value) {
    if (labelValue == value) return;
    labelValue = value; setAccessibleName(value); updateGeometry(); update(); emit labelChanged(value);
}
void Z_StatusIndicator::setShowLabel(bool value) {
    if (showLabelValue == value) return;
    showLabelValue = value; updateGeometry(); update(); emit showLabelChanged(value);
}
void Z_StatusIndicator::setSize(const QString& value) {
    requireOneOf(value, {"Small", "Medium"}, "StatusIndicator.size");
    if (sizeValue == value) return;
    sizeValue = value; updateGeometry(); update(); emit sizeChanged(value);
}
void Z_StatusIndicator::setTone(const QString& value) {
    requireOneOf(value, {"Neutral", "Process", "Success", "Warning", "Danger"}, "StatusIndicator.tone");
    if (toneValue == value) return;
    toneValue = value; update(); emit toneChanged(value);
}
QSize Z_StatusIndicator::sizeHint() const {
    const int dot = sizeValue == "Small" ? 8 : 10;
    const int fontSize = sizeValue == "Small" ? 12 : 14;
    const int height = sizeValue == "Small" ? 18 : 21;
    const int width = showLabelValue ? dot + (sizeValue == "Small" ? 6 : 8) + QFontMetrics(componentFont(fontSize)).horizontalAdvance(labelValue) : dot;
    return {width, height};
}
void Z_StatusIndicator::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    const int dot = sizeValue == "Small" ? 8 : 10;
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(toneValue == "Neutral" ? tokens.c("base.stoken.color.text.secondary") : toneColor(tokens, toneValue));
    painter.drawEllipse(QRectF(0, (height() - dot) / 2.0, dot, dot));
    if (showLabelValue) {
        painter.setFont(componentFont(sizeValue == "Small" ? 12 : 14));
        painter.setPen(tokens.c("base.stoken.color.text.primary"));
        const int x = dot + (sizeValue == "Small" ? 6 : 8);
        painter.drawText(QRect(x, 0, width() - x, height()), Qt::AlignVCenter | Qt::AlignLeft, labelValue);
    }
}

Z_Link::Z_Link(const QString& profile, QWidget* parent)
    : QAbstractButton(parent), profileValue(profile) {
    identify(this, "ComponentLab/Link", "commonWidget::Z_Link");
    setText(QStringLiteral("View details"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    FocusIndicator::instance().watch(this);
    connect(this, &QAbstractButton::clicked, this, [this] { emit activated(targetValue); });
}
void Z_Link::setTarget(const QUrl& value) {
    if (targetValue == value) return;
    targetValue = value; emit targetChanged(value);
}
void Z_Link::setSnapshotState(const QString& value) {
    if (!value.isEmpty())
        requireOneOf(value, {"Default", "Hover", "Pressed", "Focus-visible", "Disabled"},
                     "Link.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != QStringLiteral("Disabled"));
    update();
    emit snapshotStateChanged(value);
}
QSize Z_Link::sizeHint() const {
    const QFontMetrics metrics(componentFont(14));
    return {metrics.horizontalAdvance(text()) + 4, 25};
}
void Z_Link::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QString state = snapshotStateValue;
    // Retain the former dynamic-property evidence path as source-compatible
    // behavior for callers that have not yet moved to the public API.
    if (state.isEmpty()) state = property("figmaSnapshotState").toString();
    if (state.isEmpty()) {
        if (!isEnabled()) state = "Disabled";
        else if (isDown()) state = "Pressed";
        else if (underMouse()) state = "Hover";
        else if (FocusIndicator::instance().visibleFor(this)) state = "Focus-visible";
        else state = "Default";
    }
    QColor color = tokens.c("base.stoken.color.interactive.brand.default");
    if (state == "Hover") color = tokens.c("base.stoken.color.interactive.brand.hover");
    else if (state == "Pressed") color = tokens.c("base.stoken.color.interactive.brand.pressed");
    else if (state == "Disabled") color = tokens.c("base.stoken.color.interactive.brand.disabled");
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    if (state == "Focus-visible" && FocusIndicator::instance().enabledFor(this)) {
        QPen ring(FocusIndicator::instance().color(tokens.c("base.stoken.color.interactive.brand.focus-ring")), 2);
        painter.setPen(ring); painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
    }
    QFont font = componentFont(14);
    font.setUnderline(state == "Hover" || state == "Pressed");
    painter.setFont(font); painter.setPen(color);
    painter.drawText(rect().adjusted(2, 0, -2, 0), Qt::AlignCenter, text());
}
void Z_Link::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        click(); event->accept(); return;
    }
    QAbstractButton::keyPressEvent(event);
}

Z_Tag::Z_Tag(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), dismissButton(new QToolButton(this)) {
    identify(this, "ComponentLab/Tag", "commonWidget::Z_Tag");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(labelValue);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dismissButton->setText(QString::fromUtf8("×"));
    dismissButton->setAccessibleName(QStringLiteral("Dismiss ") + labelValue);
    dismissButton->setAutoRaise(true);
    dismissButton->setCursor(Qt::PointingHandCursor);
    dismissButton->setFocusPolicy(Qt::StrongFocus);
    dismissButton->hide();
    FocusIndicator::instance().watch(dismissButton);
    connect(dismissButton, &QToolButton::clicked, this, &Z_Tag::dismissed);
}
void Z_Tag::setLabel(const QString& value) {
    if (labelValue == value) return;
    labelValue = value; setAccessibleName(value);
    dismissButton->setAccessibleName(QStringLiteral("Dismiss ") + value);
    updateGeometry(); update(); emit labelChanged(value);
}
void Z_Tag::setSize(const QString& value) {
    requireOneOf(value, {"Small", "Medium"}, "Tag.size");
    if (sizeValue == value) return;
    sizeValue = value; updateGeometry(); update(); emit sizeChanged(value);
}
void Z_Tag::setTone(const QString& value) {
    requireOneOf(value, {"Neutral", "Info", "Success", "Warning", "Danger"}, "Tag.tone");
    if (toneValue == value) return;
    toneValue = value; update(); emit toneChanged(value);
}
void Z_Tag::setDismissible(bool value) {
    if (dismissibleValue == value) return;
    dismissibleValue = value; dismissButton->setVisible(value);
    updateGeometry(); update(); emit dismissibleChanged(value);
}
QSize Z_Tag::sizeHint() const {
    const int fontSize = sizeValue == "Small" ? 12 : 14;
    const int height = sizeValue == "Small" ? 22 : 29;
    return {QFontMetrics(componentFont(fontSize)).horizontalAdvance(labelValue) + 8 + (dismissibleValue ? height - 4 : 0), height};
}
void Z_Tag::resizeEvent(QResizeEvent*) {
    const int buttonSize = qMax(16, height() - 6);
    dismissButton->setGeometry(width() - buttonSize - 3, (height() - buttonSize) / 2, buttonSize, buttonSize);
}
void Z_Tag::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    QColor background = toneColor(tokens, toneValue);
    QColor foreground = toneText(tokens, toneValue);
    QPen pen(Qt::NoPen);
    if (toneValue == "Neutral") {
        pen = QPen(tokens.c("base.stoken.tag.base.default.border.color"), 1);
        foreground = tokens.c("base.stoken.tag.base.default.label.color");
    }
    painter.setPen(pen); painter.setBrush(background);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), height() / 2.0, height() / 2.0);
    painter.setFont(componentFont(sizeValue == "Small" ? 12 : 14)); painter.setPen(foreground);
    const int right = dismissibleValue ? dismissButton->width() + 6 : 4;
    painter.drawText(rect().adjusted(4, 0, -right, 0), Qt::AlignCenter, labelValue);
    dismissButton->setStyleSheet(QString("QToolButton { color:%1; border:none; background:transparent; }").arg(foreground.name(QColor::HexArgb)));
}

Z_Tooltip::Z_Tooltip(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    identify(this, "ComponentLab/Tooltip", "commonWidget::Z_Tooltip");
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAccessibleName(contentValue);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}
Z_Tooltip::~Z_Tooltip() { detach(); }
void Z_Tooltip::setContent(const QString& value) {
    if (contentValue == value) return;
    contentValue = value; setAccessibleName(value); updateGeometry(); update(); emit contentChanged(value);
}
void Z_Tooltip::setPlacement(const QString& value) {
    requireOneOf(value, {"TopLeft", "Top", "TopRight", "RightTop", "Right", "RightBottom",
                         "BottomRight", "Bottom", "BottomLeft", "LeftBottom", "Left", "LeftTop"},
                 "Tooltip.placement");
    if (placementValue == value) return;
    placementValue = value; updateGeometry(); update(); emit placementChanged(value);
}
void Z_Tooltip::setShowArrow(bool value) {
    if (showArrowValue == value) return;
    showArrowValue = value; updateGeometry(); update(); emit showArrowChanged(value);
}
QSize Z_Tooltip::sizeHint() const {
    const int arrow = showArrowValue ? 7 : 0;
    const QFontMetrics metrics(componentFont(12));
    const QRect textBounds = metrics.boundingRect(QRect(0, 0, 280, 4096),
        Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, contentValue);
    const QSize text(qMax(32, textBounds.width() + 16), qMax(26, textBounds.height() + 12));
    const bool horizontalArrow = placementValue.startsWith(QStringLiteral("Left")) ||
                                 placementValue.startsWith(QStringLiteral("Right"));
    return horizontalArrow ? QSize(text.width() + arrow, text.height())
                           : QSize(text.width(), text.height() + arrow);
}
void Z_Tooltip::attachTo(QWidget* target) {
    if (targetValue == target) return;
    detach();
    if (!target) return;
    targetValue = target;
    target->installEventFilter(this);
    setParent(target->window(), Qt::ToolTip);
    hide();
}
void Z_Tooltip::detach() {
    if (targetValue) targetValue->removeEventFilter(this);
    targetValue.clear();
    hide();
}
bool Z_Tooltip::eventFilter(QObject* watched, QEvent* event) {
    if (watched != targetValue) return QWidget::eventFilter(watched, event);
    switch (event->type()) {
    case QEvent::Enter:
    case QEvent::FocusIn: showForTarget(); break;
    case QEvent::Leave:
    case QEvent::FocusOut:
    case QEvent::Hide:
    case QEvent::Destroy: hide(); break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) hide();
        break;
    default: break;
    }
    return QWidget::eventFilter(watched, event);
}
void Z_Tooltip::showForTarget() {
    if (!targetValue || !targetValue->isVisible()) return;
    resize(sizeHint());
    const QRect targetRect(targetValue->mapToGlobal(QPoint(0, 0)), targetValue->size());
    QPoint point;
    const auto horizontalOrigin = [&] {
        if (placementValue.endsWith(QStringLiteral("Left"))) return targetRect.left();
        if (placementValue.endsWith(QStringLiteral("Right"))) return targetRect.right() - width();
        return targetRect.center().x() - width() / 2;
    };
    const auto verticalOrigin = [&] {
        if (placementValue.endsWith(QStringLiteral("Top"))) return targetRect.top();
        if (placementValue.endsWith(QStringLiteral("Bottom"))) return targetRect.bottom() - height();
        return targetRect.center().y() - height() / 2;
    };
    if (placementValue.startsWith(QStringLiteral("Bottom")))
        point = {horizontalOrigin(), targetRect.bottom() + 4};
    else if (placementValue.startsWith(QStringLiteral("Left")))
        point = {targetRect.left() - width() - 4, verticalOrigin()};
    else if (placementValue.startsWith(QStringLiteral("Right")))
        point = {targetRect.right() + 4, verticalOrigin()};
    else
        point = {horizontalOrigin(), targetRect.top() - height() - 4};
    move(point); show(); raise();
}
void Z_Tooltip::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    const int arrow = showArrowValue ? 7 : 0;
    QRectF bubble = rect();
    const bool top = placementValue.startsWith(QStringLiteral("Top"));
    const bool bottom = placementValue.startsWith(QStringLiteral("Bottom"));
    const bool left = placementValue.startsWith(QStringLiteral("Left"));
    if (top) bubble.adjust(0, 0, 0, -arrow);
    else if (bottom) bubble.adjust(0, arrow, 0, 0);
    else if (left) bubble.adjust(0, 0, -arrow, 0);
    else bubble.adjust(arrow, 0, 0, 0);
    const QColor background = tokens.c("base.stoken.tooltip.base.default.container.background");
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen); painter.setBrush(background);
    painter.drawRoundedRect(bubble.adjusted(0, 0, -1, -1), tokens.n("base.stoken.tooltip.base.default.container.radius"), tokens.n("base.stoken.tooltip.base.default.container.radius"));
    if (showArrowValue) {
        QPainterPath path;
        qreal anchorX = width() / 2.0;
        if (placementValue.endsWith(QStringLiteral("Left"))) anchorX = 16;
        else if (placementValue.endsWith(QStringLiteral("Right"))) anchorX = width() - 16;
        qreal anchorY = height() / 2.0;
        if (placementValue.endsWith(QStringLiteral("Top"))) anchorY = 14;
        else if (placementValue.endsWith(QStringLiteral("Bottom"))) anchorY = height() - 14;
        if (top) { path.moveTo(anchorX-5, bubble.bottom()); path.lineTo(anchorX, height()); path.lineTo(anchorX+5, bubble.bottom()); }
        else if (bottom) { path.moveTo(anchorX-5, bubble.top()); path.lineTo(anchorX, 0); path.lineTo(anchorX+5, bubble.top()); }
        else if (left) { path.moveTo(bubble.right(), anchorY-5); path.lineTo(width(), anchorY); path.lineTo(bubble.right(), anchorY+5); }
        else { path.moveTo(bubble.left(), anchorY-5); path.lineTo(0, anchorY); path.lineTo(bubble.left(), anchorY+5); }
        path.closeSubpath(); painter.drawPath(path);
    }
    painter.setFont(componentFont(12)); painter.setPen(tokens.c("base.stoken.tooltip.base.default.content.color"));
    painter.drawText(bubble.adjusted(8, 6, -8, -6),
                     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, contentValue);
}

Z_SearchInput::Z_SearchInput(const QString& profile, QWidget* parent)
    : Z_InputField(std::make_shared<field::Tokens>(profile), parent) {
    identify(this, "ComponentLab/SearchInput", "commonWidget::Z_SearchInput");
    setKind(Kind::Search);
    setControlSize(QStringLiteral("md"));
    setVariant(QStringLiteral("outlined"));
    setPlaceholder(QStringLiteral("Search"));
    setSuffix(shortcutValue);
    setAllowClear(true);
    setFocusPolicy(Qt::NoFocus);
    // QWidget propagates its focus policy to the focus proxy; restore the editor as the
    // sole tab stop so the composite root never creates a duplicate focus stop.
    lineEdit()->setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(QStringLiteral("Search"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}
void Z_SearchInput::setShortcutText(const QString& value) {
    if (shortcutValue == value) return;
    shortcutValue = value;
    setSuffix(value);
    emit shortcutTextChanged(value);
}
void Z_SearchInput::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Focus", "Filled", "Disabled", "Error"},
                 "SearchInput.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != "Disabled");
    setStatus(value == "Error" ? QStringLiteral("error") : QString());
    // FieldChrome validates the Figma-facing title-case state and normalizes it internally.
    Z_InputField::setSnapshotState(value);
    emit snapshotStateChanged(value);
}
QSize Z_SearchInput::sizeHint() const { return {360, 40}; }

Z_NumberInput::Z_NumberInput(const QString& profile, QWidget* parent)
    : QWidget(parent), integerEditor(new Z_SpinBox(this)),
      decimalEditor(new Z_DoubleSpinBox(this)), editorLayout(new QHBoxLayout(this)) {
    Q_UNUSED(profile)
    identify(this, "ComponentLab/NumberInput", "commonWidget::Z_NumberInput");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Number input"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    editorLayout->addWidget(integerEditor);
    editorLayout->addWidget(decimalEditor);
    integerEditor->setFocusPolicy(Qt::StrongFocus);
    decimalEditor->setFocusPolicy(Qt::StrongFocus);
    integerEditor->setAlignment(Qt::AlignLeft);
    decimalEditor->setAlignment(Qt::AlignLeft);
    decimalEditor->setDecimals(2);
    decimalEditor->hide();
    connect(integerEditor, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value) {
                if (typeValue == "Integer") emit valueChanged(value);
            });
    connect(decimalEditor, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
                if (typeValue == "Decimal") emit valueChanged(value);
            });
    syncContracts();
    setValue(12.0);
}
double Z_NumberInput::value() const {
    return typeValue == "Decimal" ? decimalEditor->value() : integerEditor->value();
}
QAbstractSpinBox* Z_NumberInput::editor() const {
    return typeValue == "Decimal" ? static_cast<QAbstractSpinBox*>(decimalEditor)
                                   : static_cast<QAbstractSpinBox*>(integerEditor);
}
void Z_NumberInput::setType(const QString& value) {
    requireOneOf(value, {"Integer", "Decimal"}, "NumberInput.type");
    if (typeValue == value) return;
    const double previous = this->value();
    typeValue = value;
    integerEditor->setVisible(value == "Integer");
    decimalEditor->setVisible(value == "Decimal");
    setValue(previous);
    emit typeChanged(value);
}
void Z_NumberInput::setValue(double value) {
    const double bounded = qBound(minimumValue, value, maximumValue);
    const double previous = this->value();
    const QSignalBlocker integerBlocker(integerEditor);
    const QSignalBlocker decimalBlocker(decimalEditor);
    integerEditor->setValue(qRound(bounded));
    decimalEditor->setValue(bounded);
    if (!qFuzzyCompare(previous + 1.0, this->value() + 1.0))
        emit valueChanged(this->value());
}
void Z_NumberInput::setMinimum(double value) { setRange(value, maximumValue); }
void Z_NumberInput::setMaximum(double value) { setRange(minimumValue, value); }
void Z_NumberInput::setRange(double minimum, double maximum) {
    if (minimum > maximum)
        throw std::invalid_argument("NumberInput minimum exceeds maximum");
    minimumValue = minimum;
    maximumValue = maximum;
    syncContracts();
}
void Z_NumberInput::setSingleStep(double value) {
    if (!(value > 0.0))
        throw std::invalid_argument("NumberInput step must be positive");
    stepValue = value;
    syncContracts();
}
void Z_NumberInput::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Focus", "Disabled"}, "NumberInput.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != "Disabled");
    integerEditor->setProperty("figmaSnapshotState", value);
    decimalEditor->setProperty("figmaSnapshotState", value);
    integerEditor->update();
    decimalEditor->update();
    emit snapshotStateChanged(value);
}
QSize Z_NumberInput::sizeHint() const { return {280, 40}; }
void Z_NumberInput::syncContracts() {
    const double current = value();
    const QSignalBlocker integerBlocker(integerEditor);
    const QSignalBlocker decimalBlocker(decimalEditor);
    integerEditor->setRange(qCeil(minimumValue), qFloor(maximumValue));
    integerEditor->setSingleStep(qMax(1, qRound(stepValue)));
    decimalEditor->setRange(minimumValue, maximumValue);
    decimalEditor->setSingleStep(stepValue);
    integerEditor->setValue(qRound(qBound(minimumValue, current, maximumValue)));
    decimalEditor->setValue(qBound(minimumValue, current, maximumValue));
}

Z_ColorPicker::Z_ColorPicker(const QString& profile, QWidget* parent)
    : QAbstractButton(parent), profileValue(profile) {
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/ColorPicker", "commonWidget::Z_ColorPicker");
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("Color picker"));
    setAccessibleDescription(QStringLiteral("Current color #5B5BD6; opens color selection"));
    FocusIndicator::instance().watch(this);
    connect(this, &QAbstractButton::clicked, this, [this] {
        emit pickerRequested(colorValue);
    });
}
QString Z_ColorPicker::colorText() const {
    return colorValue.name(colorFormatValue == QStringLiteral("HexArgb")
                               ? QColor::HexArgb : QColor::HexRgb).toUpper();
}
void Z_ColorPicker::setColor(const QColor& value) {
    if (!value.isValid())
        throw std::invalid_argument("ColorPicker.color must be a valid QColor");
    if (colorValue == value) return;
    colorValue = value;
    setAccessibleDescription(QStringLiteral("Current color %1; opens color selection")
                                 .arg(colorText()));
    update();
    emit colorChanged(value);
}
void Z_ColorPicker::setColorFormat(const QString& value) {
    requireOneOf(value, {"HexRgb", "HexArgb"}, "ColorPicker.colorFormat");
    if (colorFormatValue == value) return;
    colorFormatValue = value;
    setAccessibleDescription(QStringLiteral("Current color %1; opens color selection")
                                 .arg(colorText()));
    update();
    emit colorFormatChanged(value);
}
void Z_ColorPicker::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Focus", "Disabled"}, "ColorPicker.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != QStringLiteral("Disabled"));
    update();
    emit snapshotStateChanged(value);
}
QSize Z_ColorPicker::sizeHint() const { return {300, 40}; }
void Z_ColorPicker::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    const bool disabled = !isEnabled() || snapshotStateValue == QStringLiteral("Disabled");
    const bool focused = snapshotStateValue == QStringLiteral("Focus") ||
                         property("focusIndicatorVisible").toBool();
    const bool darkProfile = profileValue.startsWith(QStringLiteral("dark-"));
    const QColor background = tokens.c(disabled
        ? (darkProfile ? "semantic.color.background.surface-muted"
                       : "base.stoken.color.background.surface-tertiary")
        : (darkProfile ? "semantic.color.background.surface"
                       : "base.stoken.color.background.surface"));
    const QColor border = tokens.c(disabled
        ? "base.stoken.color.border.disabled"
        : focused ? "base.stoken.color.interactive.brand.focus-ring"
                  : (darkProfile ? "semantic.color.border.default"
                                 : "base.stoken.color.border.default"));
    const QColor text = tokens.c(disabled ? "semantic.color.text.disabled"
                                          : (darkProfile ? "semantic.color.text.primary"
                                                         : "base.stoken.color.text.primary"));
    const QColor action = tokens.c(disabled ? "base.stoken.color.text.disabled"
                                            : "base.stoken.color.interactive.brand.default");
    const QColor swatch = disabled ? tokens.c("base.stoken.color.border.disabled") : colorValue;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(focused && !disabled ? FocusIndicator::instance().color(border) : border, 1));
    painter.setBrush(background);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);

    painter.setPen(Qt::NoPen);
    painter.setBrush(swatch);
    painter.drawRoundedRect(QRectF(30, 8, 24, 24), 4, 4);

    painter.setFont(componentFont(13));
    painter.setPen(text);
    painter.drawText(QRect(62, 0, 100, height()), Qt::AlignVCenter | Qt::AlignLeft,
                     colorText());
    QFont actionFont = componentFont(11);
    painter.setFont(actionFont);
    painter.setPen(action);
    painter.drawText(QRect(222, 0, 48, height()), Qt::AlignVCenter | Qt::AlignRight,
                     QStringLiteral("Open"));
}
void Z_ColorPicker::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        click();
        event->accept();
        return;
    }
    QAbstractButton::keyPressEvent(event);
}
void Z_ColorPicker::changeEvent(QEvent* event) {
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) update();
}

Z_Transfer::Z_Transfer(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/Transfer", "commonWidget::Z_Transfer");
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("Transfer selection"));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(14);

    const auto createPanel = [this](const QString& objectName, QLabel** label,
                                    QListView** view) {
        auto* panel = new QFrame(this);
        panel->setObjectName(objectName);
        panel->setFixedSize(205, 180);
        panel->setFocusPolicy(Qt::NoFocus);
        auto* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(12, 12, 12, 12);
        panelLayout->setSpacing(8);
        *label = new QLabel(panel);
        (*label)->setFocusPolicy(Qt::NoFocus);
        (*label)->setFont(componentFont(14));
        panelLayout->addWidget(*label);
        *view = new QListView(panel);
        (*view)->setFrameShape(QFrame::NoFrame);
        (*view)->setEditTriggers(QAbstractItemView::NoEditTriggers);
        (*view)->setSelectionMode(QAbstractItemView::ExtendedSelection);
        (*view)->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        (*view)->setFocusPolicy(Qt::StrongFocus);
        (*view)->setUniformItemSizes(true);
        (*view)->setFont(componentFont(13));
        (*view)->setItemDelegate(new TransferItemDelegate(profileValue, *view));
        panelLayout->addWidget(*view, 1);
        FocusIndicator::instance().watch(*view);
        return panel;
    };

    root->addWidget(createPanel(QStringLiteral("transferAvailablePanel"),
                                &availableLabelValue, &availableViewValue));
    availableViewValue->setAccessibleName(QStringLiteral("Available items"));

    auto* commandColumn = new QWidget(this);
    commandColumn->setObjectName(QStringLiteral("transferCommands"));
    commandColumn->setFocusPolicy(Qt::NoFocus);
    commandColumn->setFixedWidth(44);
    auto* commands = new QVBoxLayout(commandColumn);
    commands->setContentsMargins(0, 0, 0, 0);
    commands->setSpacing(8);
    commands->addStretch(1);
    moveToSelectedButtonValue = new QToolButton(commandColumn);
    moveToSelectedButtonValue->setObjectName(QStringLiteral("transferMoveToSelected"));
    moveToSelectedButtonValue->setText(QStringLiteral("→"));
    moveToSelectedButtonValue->setAccessibleName(QStringLiteral("Move to selected"));
    moveToSelectedButtonValue->setFocusPolicy(Qt::StrongFocus);
    moveToSelectedButtonValue->setFixedSize(44, 36);
    commands->addWidget(moveToSelectedButtonValue);
    moveToAvailableButtonValue = new QToolButton(commandColumn);
    moveToAvailableButtonValue->setObjectName(QStringLiteral("transferMoveToAvailable"));
    moveToAvailableButtonValue->setText(QStringLiteral("←"));
    moveToAvailableButtonValue->setAccessibleName(QStringLiteral("Move to available"));
    moveToAvailableButtonValue->setFocusPolicy(Qt::StrongFocus);
    moveToAvailableButtonValue->setFixedSize(44, 36);
    commands->addWidget(moveToAvailableButtonValue);
    commands->addStretch(1);
    FocusIndicator::instance().watch(moveToSelectedButtonValue);
    FocusIndicator::instance().watch(moveToAvailableButtonValue);
    root->addWidget(commandColumn);

    root->addWidget(createPanel(QStringLiteral("transferSelectedPanel"),
                                &selectedLabelValue, &selectedViewValue));
    selectedViewValue->setAccessibleName(QStringLiteral("Selected items"));

    connect(moveToSelectedButtonValue, &QToolButton::clicked, this, [this] {
        emit moveToSelectedRequested(requestedRows(availableViewValue));
    });
    connect(moveToAvailableButtonValue, &QToolButton::clicked, this, [this] {
        emit moveToAvailableRequested(requestedRows(selectedViewValue));
    });
    bindView(availableViewValue);
    bindView(selectedViewValue);
    refreshContent();
    refreshStyle();
    refreshAccessibility();
}

QAbstractItemModel* Z_Transfer::availableModel() const {
    return availableViewValue->model();
}

QAbstractItemModel* Z_Transfer::selectedModel() const {
    return selectedViewValue->model();
}

void Z_Transfer::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Disabled", "Loading"},
                 "Transfer.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != QStringLiteral("Disabled"));
    refreshContent();
    refreshStyle();
    refreshAccessibility();
    update();
    emit snapshotStateChanged(value);
}

void Z_Transfer::setAvailableTitle(const QString& value) {
    if (availableTitleValue == value) return;
    availableTitleValue = value;
    refreshContent();
    refreshAccessibility();
    emit availableTitleChanged(value);
}

void Z_Transfer::setSelectedTitle(const QString& value) {
    if (selectedTitleValue == value) return;
    selectedTitleValue = value;
    refreshContent();
    refreshAccessibility();
    emit selectedTitleChanged(value);
}

void Z_Transfer::setMoveToSelectedAllowed(bool value) {
    if (moveToSelectedAllowedValue == value) return;
    moveToSelectedAllowedValue = value;
    refreshContent();
    emit moveToSelectedAllowedChanged(value);
}

void Z_Transfer::setMoveToAvailableAllowed(bool value) {
    if (moveToAvailableAllowedValue == value) return;
    moveToAvailableAllowedValue = value;
    refreshContent();
    emit moveToAvailableAllowedChanged(value);
}

void Z_Transfer::setAvailableModel(QAbstractItemModel* model) {
    if (availableViewValue->model() == model) return;
    availableViewValue->setModel(model);
    bindView(availableViewValue);
    if (model) {
        const auto refresh = [this, model] {
            if (availableViewValue->model() == model) {
                refreshContent();
                refreshAccessibility();
            }
        };
        connect(model, &QAbstractItemModel::rowsInserted, this, refresh);
        connect(model, &QAbstractItemModel::rowsRemoved, this, refresh);
        connect(model, &QAbstractItemModel::modelReset, this, refresh);
        connect(model, &QAbstractItemModel::layoutChanged, this, refresh);
    }
    refreshContent();
    refreshAccessibility();
    emit availableModelChanged(model);
}

void Z_Transfer::setSelectedModel(QAbstractItemModel* model) {
    if (selectedViewValue->model() == model) return;
    selectedViewValue->setModel(model);
    bindView(selectedViewValue);
    if (model) {
        const auto refresh = [this, model] {
            if (selectedViewValue->model() == model) {
                refreshContent();
                refreshAccessibility();
            }
        };
        connect(model, &QAbstractItemModel::rowsInserted, this, refresh);
        connect(model, &QAbstractItemModel::rowsRemoved, this, refresh);
        connect(model, &QAbstractItemModel::modelReset, this, refresh);
        connect(model, &QAbstractItemModel::layoutChanged, this, refresh);
    }
    refreshContent();
    refreshAccessibility();
    emit selectedModelChanged(model);
}

void Z_Transfer::bindView(QListView* view) {
    if (!view->selectionModel()) return;
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
                refreshAccessibility();
                emit selectionChanged();
            });
}

QModelIndexList Z_Transfer::requestedRows(QListView* view) const {
    QModelIndexList rows;
    if (!view->model())
        return rows;
    for (int row = 0; row < view->model()->rowCount(); ++row) {
        const QModelIndex index = view->model()->index(row, 0);
        if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
            rows.append(index);
    }
    if (rows.isEmpty() && view->selectionModel())
        rows = view->selectionModel()->selectedRows();
    return rows;
}

void Z_Transfer::refreshContent() {
    const int availableCount = availableModel() ? availableModel()->rowCount() : 0;
    const int selectedCount = selectedModel() ? selectedModel()->rowCount() : 0;
    availableLabelValue->setText(QStringLiteral("%1 · %2")
                                     .arg(availableTitleValue).arg(availableCount));
    selectedLabelValue->setText(snapshotStateValue == QStringLiteral("Loading")
                                    ? QStringLiteral("Loading…")
                                    : QStringLiteral("%1 · %2")
                                          .arg(selectedTitleValue).arg(selectedCount));
    moveToSelectedButtonValue->setEnabled(isEnabled() && moveToSelectedAllowedValue);
    moveToAvailableButtonValue->setEnabled(isEnabled() && moveToAvailableAllowedValue);
}

void Z_Transfer::refreshStyle() {
    const Tokens tokens(profileValue);
    const bool darkProfile = profileValue.startsWith(QStringLiteral("dark-"));
    const QColor panel = tokens.c(darkProfile
        ? "semantic.color.background.surface-muted"
        : "base.stoken.color.background.surface-tertiary");
    const QColor surface = tokens.c(darkProfile
        ? "semantic.color.background.surface"
        : "base.stoken.color.background.surface");
    const QColor panelBorder = tokens.c(darkProfile
        ? "semantic.color.border.subtle"
        : "base.stoken.color.border.secondary");
    const QColor primary = tokens.c(darkProfile
        ? "semantic.color.text.primary"
        : "base.stoken.color.text.primary");
    const QColor secondary = tokens.c(darkProfile
        ? "semantic.color.text.secondary"
        : "base.stoken.color.text.secondary");
    const QColor disabled = tokens.c("base.stoken.color.text.disabled");
    const QColor disabledBorder = tokens.c("base.stoken.color.border.disabled");
    const QColor brand = tokens.c("base.stoken.color.interactive.brand.default");
    const QColor subtle = tokens.c("base.stoken.color.interactive.brand.subtle");
    const QString panelStyle = QStringLiteral(
        "QFrame { background:%1; border:1px solid %2; border-radius:8px; }"
        "QLabel { background:transparent; border:0; color:%3; font-weight:600; }"
        "QListView { background:transparent; border:0; color:%4; outline:0; }"
        "QListView::item { min-height:20px; }"
        "QListView::item:selected { background:%5; color:%3; border-radius:3px; }"
        "QListView[focusIndicatorVisible=\"true\"] { border:2px solid %6; border-radius:4px; }"
        "QListView:disabled { color:%7; }")
        .arg(panel.name(), panelBorder.name(), primary.name(), secondary.name(),
             subtle.name(), brand.name(), disabled.name());
    findChild<QFrame*>(QStringLiteral("transferAvailablePanel"))->setStyleSheet(panelStyle);
    findChild<QFrame*>(QStringLiteral("transferSelectedPanel"))->setStyleSheet(panelStyle);
    setStyleSheet(QStringLiteral(
        "QToolButton { font-family:'Noto Sans SC'; font-size:14px; min-width:0; min-height:0; }"
        "QToolButton#transferMoveToSelected { background:%1; border:1px solid %2; border-radius:8px; color:%2; font-weight:600; }"
        "QToolButton#transferMoveToAvailable { background:%3; border:1px solid %4; border-radius:8px; color:%5; }"
        "QToolButton#transferMoveToSelected:disabled, "
        "QToolButton#transferMoveToAvailable:disabled { background:%3; border-color:%6; color:%7; }"
        "QToolButton[focusIndicatorVisible=\"true\"] { border:2px solid %2; }")
        .arg(subtle.name(), brand.name(), surface.name(), panelBorder.name(),
             primary.name(), disabledBorder.name(), disabled.name()));
}

void Z_Transfer::refreshAccessibility() {
    const int availableCount = availableModel() ? availableModel()->rowCount() : 0;
    const int selectedCount = selectedModel() ? selectedModel()->rowCount() : 0;
    const int availableSelection = availableViewValue->selectionModel()
        ? availableViewValue->selectionModel()->selectedRows().size() : 0;
    const int selectedSelection = selectedViewValue->selectionModel()
        ? selectedViewValue->selectionModel()->selectedRows().size() : 0;
    setAccessibleDescription(QStringLiteral(
        "%1 available, %2 selected; %3 available rows and %4 selected rows selected; state %5")
        .arg(availableCount).arg(selectedCount).arg(availableSelection)
        .arg(selectedSelection).arg(snapshotStateValue));
}

QSize Z_Transfer::sizeHint() const { return {520, 210}; }

void Z_Transfer::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    const bool darkProfile = profileValue.startsWith(QStringLiteral("dark-"));
    const QColor background = tokens.c(darkProfile
        ? "semantic.color.background.surface"
        : "base.stoken.color.background.surface");
    const QColor border = tokens.c(!isEnabled()
        ? "base.stoken.color.border.disabled"
        : (darkProfile ? "semantic.color.border.default"
                       : "base.stoken.color.border.default"));
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(border, 1));
    painter.setBrush(background);
    painter.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), 10, 10);
}

void Z_Transfer::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        refreshContent();
        refreshStyle();
        update();
    }
}

Z_Upload::Z_Upload(const QString& profile, QWidget* parent)
    : QAbstractButton(parent), profileValue(profile) {
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/Upload", "commonWidget::Z_Upload");
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setAcceptDrops(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("File upload"));
    FocusIndicator::instance().watch(this);
    connect(this, &QAbstractButton::clicked, this, [this] {
        if (snapshotStateValue != QStringLiteral("Uploading"))
            emit browseRequested();
    });
    refreshAccessibility();
}

void Z_Upload::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Hover", "Uploading", "Success", "Error"},
                 "Upload.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    dragActive = false;
    refreshAccessibility();
    update();
    emit snapshotStateChanged(value);
}

void Z_Upload::setProgress(int value) {
    if (value < 0 || value > 100)
        throw std::invalid_argument("Upload.progress must be between 0 and 100");
    if (progressValue == value) return;
    progressValue = value;
    refreshAccessibility();
    update();
    emit progressChanged(value);
}

void Z_Upload::setFileName(const QString& value) {
    if (fileNameValue == value) return;
    fileNameValue = value;
    refreshAccessibility();
    update();
    emit fileNameChanged(value);
}

void Z_Upload::setHelperText(const QString& value) {
    if (helperTextValue == value) return;
    helperTextValue = value;
    refreshAccessibility();
    update();
    emit helperTextChanged(value);
}

QSize Z_Upload::sizeHint() const { return {390, 150}; }

QString Z_Upload::primaryText() const {
    if (snapshotStateValue == QStringLiteral("Uploading"))
        return fileNameValue.isEmpty()
            ? QStringLiteral("Uploading…")
            : QStringLiteral("Uploading %1…").arg(fileNameValue);
    if (snapshotStateValue == QStringLiteral("Success"))
        return QStringLiteral("Upload complete");
    if (snapshotStateValue == QStringLiteral("Error"))
        return QStringLiteral("Upload failed");
    return QStringLiteral("Drop file or browse");
}

void Z_Upload::refreshAccessibility() {
    QString description = primaryText();
    if (!helperTextValue.isEmpty())
        description += QStringLiteral(". ") + helperTextValue;
    if (snapshotStateValue == QStringLiteral("Uploading"))
        description += QStringLiteral(". %1 percent").arg(progressValue);
    setAccessibleDescription(description);
}

void Z_Upload::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    const bool darkProfile = profileValue.startsWith(QStringLiteral("dark-"));
    const bool error = snapshotStateValue == QStringLiteral("Error");
    const bool uploading = snapshotStateValue == QStringLiteral("Uploading");
    const bool success = snapshotStateValue == QStringLiteral("Success");
    const bool highlighted = snapshotStateValue == QStringLiteral("Hover") ||
                             dragActive || underMouse() || isDown() ||
                             FocusIndicator::instance().visibleFor(this);
    const QColor background = tokens.c(darkProfile
        ? "semantic.color.background.surface"
        : "base.stoken.color.background.surface");
    const QColor normalBorder = tokens.c(darkProfile
        ? "semantic.color.border.default"
        : "base.stoken.color.border.default");
    const QColor brand = tokens.c("base.stoken.color.interactive.brand.default");
    const QColor danger = tokens.c("base.stoken.color.status.danger.default");
    // Figma currently binds the Success glyph to the near-white success-bg
    // value. Use the semantic success foreground so the state remains legible.
    const QColor successForeground = tokens.c("semantic.color.status.success");
    const QColor primary = tokens.c(darkProfile
        ? "semantic.color.text.primary"
        : "base.stoken.color.text.primary");
    const QColor secondary = tokens.c(darkProfile
        ? "semantic.color.text.secondary"
        : "base.stoken.color.text.secondary");
    const QColor border = error ? danger : highlighted ? brand : normalBorder;
    const qreal borderWidth = error || highlighted ? 2.0 : 1.0;
    const QColor glyph = error ? danger : success ? successForeground : brand;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(border, borderWidth));
    painter.setBrush(background);
    const qreal inset = borderWidth / 2.0;
    painter.drawRoundedRect(QRectF(rect()).adjusted(inset, inset, -inset, -inset), 10, 10);

    QFont glyphFont = componentFont(28);
    glyphFont.setWeight(QFont::DemiBold);
    painter.setFont(glyphFont);
    painter.setPen(glyph);
    const QString glyphText = error ? QStringLiteral("!")
        : success ? QStringLiteral("✓")
        : uploading ? QStringLiteral("%1%").arg(progressValue)
                    : QStringLiteral("↑");
    painter.drawText(QRect(14, 25, width() - 28, 38), Qt::AlignCenter, glyphText);

    QFont primaryFont = componentFont(15);
    primaryFont.setWeight(QFont::DemiBold);
    painter.setFont(primaryFont);
    painter.setPen(primary);
    painter.drawText(QRect(14, 66, width() - 28, 24), Qt::AlignCenter, primaryText());

    QFont helperFont = componentFont(12);
    helperFont.setWeight(QFont::Normal);
    painter.setFont(helperFont);
    painter.setPen(secondary);
    painter.drawText(QRect(14, 94, width() - 28, 20), Qt::AlignCenter, helperTextValue);
}

void Z_Upload::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        click();
        event->accept();
        return;
    }
    QAbstractButton::keyPressEvent(event);
}

void Z_Upload::dragEnterEvent(QDragEnterEvent* event) {
    if (isEnabled() && snapshotStateValue != QStringLiteral("Uploading") &&
        event->mimeData()->hasUrls()) {
        dragActive = true;
        update();
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void Z_Upload::dragLeaveEvent(QDragLeaveEvent* event) {
    dragActive = false;
    update();
    event->accept();
}

void Z_Upload::dropEvent(QDropEvent* event) {
    QStringList localFiles;
    for (const auto& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) localFiles.append(url.toLocalFile());
    }
    dragActive = false;
    update();
    if (localFiles.isEmpty()) {
        event->ignore();
        return;
    }
    emit filesDropped(localFiles);
    event->acceptProposedAction();
}

void Z_Upload::changeEvent(QEvent* event) {
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        setCursor(isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

Z_NumericKeypad::Z_NumericKeypad(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), keyLayoutValue(new QGridLayout(this)) {
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/NumericKeypad", "commonWidget::Z_NumericKeypad");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Numeric keypad"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    keyLayoutValue->setContentsMargins(14, 12, 14, 12);
    keyLayoutValue->setHorizontalSpacing(8);
    keyLayoutValue->setVerticalSpacing(8);
    keyLayoutValue->setAlignment(Qt::AlignCenter);

    for (int index = 0; index < keyValues.size(); ++index) {
        auto* button = new QToolButton(this);
        button->setObjectName(QStringLiteral("numericKeypadKey%1").arg(index));
        button->setFocusPolicy(Qt::StrongFocus);
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("keypadKey", keyValues[index]);
        button->installEventFilter(this);
        FocusIndicator::instance().watch(button);
        keyLayoutValue->addWidget(button, index / 3, index % 3);
        keyButtons.append(button);
        connect(button, &QToolButton::clicked, this, [this, index] {
            const QString semanticKey = keyValues[index];
            emit keyPressed(semanticKey);
            if (semanticKey == QStringLiteral("commit")) emit commitRequested();
        });
    }
    resize(sizeHint());
    refreshStyle();
    refreshGeometry();
    refreshAccessibleDescription();
}
void Z_NumericKeypad::setDensity(const QString& value) {
    requireOneOf(value, {"Comfortable", "Compact"}, "NumericKeypad.density");
    if (densityValue == value) return;
    densityValue = value;
    resize(sizeHint());
    refreshStyle();
    refreshGeometry();
    refreshAccessibleDescription();
    updateGeometry();
    emit densityChanged(value);
}
void Z_NumericKeypad::setKeyLayout(const QStringList& values) {
    if (values.size() != 12)
        throw std::invalid_argument("NumericKeypad requires exactly twelve key slots");
    QStringList uniqueValues;
    for (const auto& value : values) {
        if (value.trimmed().isEmpty())
            throw std::invalid_argument("NumericKeypad semantic key cannot be empty");
        if (uniqueValues.contains(value))
            throw std::invalid_argument("NumericKeypad semantic keys must be unique");
        uniqueValues.append(value);
    }
    for (const auto& value : allowedKeyValues)
        if (!values.contains(value))
            throw std::invalid_argument("NumericKeypad allowed key is absent from the new layout");
    if (keyValues == values) return;
    keyValues = values;
    refreshStyle();
    refreshAccessibleDescription();
    emit keyLayoutChanged(values);
}
void Z_NumericKeypad::setAllowedKeys(const QStringList& values) {
    QStringList uniqueValues;
    for (const auto& value : values) {
        if (!keyValues.contains(value))
            throw std::invalid_argument("NumericKeypad allowed key is absent from the layout");
        if (!uniqueValues.contains(value)) uniqueValues.append(value);
    }
    if (allowedKeyValues == uniqueValues) return;
    allowedKeyValues = uniqueValues;
    refreshStyle();
    refreshAccessibleDescription();
    emit allowedKeysChanged(uniqueValues);
}
void Z_NumericKeypad::setDecimalSeparator(const QString& value) {
    if (value.isEmpty() || value.size() > 2)
        throw std::invalid_argument("NumericKeypad decimal separator must contain one visible separator");
    if (decimalSeparatorValue == value) return;
    decimalSeparatorValue = value;
    refreshStyle();
    emit decimalSeparatorChanged(value);
}
QSize Z_NumericKeypad::sizeHint() const {
    return densityValue == QStringLiteral("Compact") ? QSize(260, 250) : QSize(260, 290);
}
bool Z_NumericKeypad::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(watched, event);
    const int index = keyButtons.indexOf(qobject_cast<QToolButton*>(watched));
    if (index < 0) return QWidget::eventFilter(watched, event);
    const int key = static_cast<QKeyEvent*>(event)->key();
    auto move = [](int current, int qtKey) {
        const int row = current / 3;
        const int column = current % 3;
        if (qtKey == Qt::Key_Left) return row * 3 + (column + 2) % 3;
        if (qtKey == Qt::Key_Right) return row * 3 + (column + 1) % 3;
        if (qtKey == Qt::Key_Up) return ((row + 3) % 4) * 3 + column;
        if (qtKey == Qt::Key_Down) return ((row + 1) % 4) * 3 + column;
        return current;
    };
    int next = index;
    if (key == Qt::Key_Home) next = 0;
    else if (key == Qt::Key_End) next = keyButtons.size() - 1;
    else if (key == Qt::Key_Left || key == Qt::Key_Right ||
             key == Qt::Key_Up || key == Qt::Key_Down)
        next = move(index, key);
    else
        return QWidget::eventFilter(watched, event);

    for (int attempts = 0; attempts < keyButtons.size(); ++attempts) {
        if (keyButtons[next] && keyButtons[next]->isEnabled()) {
            keyButtons[next]->setFocus(Qt::TabFocusReason);
            event->accept();
            return true;
        }
        if (key == Qt::Key_Home || key == Qt::Key_End)
            next += key == Qt::Key_Home ? 1 : -1;
        else
            next = move(next, key);
        if (next < 0 || next >= keyButtons.size()) break;
    }
    return QWidget::eventFilter(watched, event);
}
void Z_NumericKeypad::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c(isEnabled() ? "base.stoken.color.border.default"
                                             : "base.stoken.color.border.disabled"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}
void Z_NumericKeypad::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) refreshStyle();
}
QString Z_NumericKeypad::displayText(const QString& semanticKey) const {
    if (semanticKey == QStringLiteral("decimal")) return decimalSeparatorValue;
    if (semanticKey == QStringLiteral("backspace")) return QString::fromUtf8("⌫");
    if (semanticKey == QStringLiteral("commit")) return QString::fromUtf8("↵");
    return semanticKey;
}
QString Z_NumericKeypad::accessibleNameFor(const QString& semanticKey) const {
    if (semanticKey == QStringLiteral("decimal")) return QStringLiteral("Decimal separator");
    if (semanticKey == QStringLiteral("backspace")) return QStringLiteral("Backspace");
    if (semanticKey == QStringLiteral("commit")) return QStringLiteral("Commit");
    if (semanticKey.size() == 1 && semanticKey.front().isDigit())
        return QStringLiteral("Digit ") + semanticKey;
    return displayText(semanticKey);
}
void Z_NumericKeypad::refreshGeometry() {
    const int height = densityValue == QStringLiteral("Compact") ? 42 : 50;
    for (const auto& button : keyButtons)
        if (button) button->setFixedSize(68, height);
    keyLayoutValue->invalidate();
    keyLayoutValue->activate();
}
void Z_NumericKeypad::refreshStyle() {
    const Tokens tokens(profileValue);
    const QString surface = tokens.c("base.stoken.color.background.surface").name(QColor::HexArgb);
    const QString border = tokens.c("base.stoken.color.border.default").name(QColor::HexArgb);
    const QString primary = tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb);
    const QString disabled = tokens.c("base.stoken.color.text.disabled").name(QColor::HexArgb);
    const QString borderDisabled = tokens.c("base.stoken.color.border.disabled").name(QColor::HexArgb);
    const QString brand = tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb);
    const QString subtle = tokens.c("base.stoken.color.interactive.brand.subtle").name(QColor::HexArgb);
    const QString focus = FocusIndicator::instance().color(
        tokens.c("base.stoken.color.interactive.brand.default")).name(QColor::HexArgb);
    const int keyContentHeight = densityValue == QStringLiteral("Compact") ? 40 : 48;
    for (int index = 0; index < keyButtons.size(); ++index) {
        auto* button = keyButtons[index].data();
        if (!button) continue;
        const QString semanticKey = keyValues[index];
        const bool allowed = allowedKeyValues.isEmpty() || allowedKeyValues.contains(semanticKey);
        button->setEnabled(isEnabled() && allowed);
        button->setProperty("keypadKey", semanticKey);
        button->setText(displayText(semanticKey));
        button->setAccessibleName(accessibleNameFor(semanticKey));
        button->setAccessibleDescription(QStringLiteral("Numeric keypad key: ") + semanticKey);
        const bool emphasized = semanticKey == QStringLiteral("0") && button->isEnabled();
        const QString foreground = button->isEnabled() ? (emphasized ? brand : primary) : disabled;
        const QString edge = button->isEnabled() ? (emphasized ? brand : border) : borderDisabled;
        const QString background = emphasized ? subtle : surface;
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background:%1; color:%2; border:1px solid %3; border-radius:8px; padding:0; min-width:66px; max-width:66px; min-height:%6px; max-height:%6px; }"
            "QToolButton:hover:enabled { border-color:%4; }"
            "QToolButton:pressed:enabled { background:%5; border-color:%4; }"
            "QToolButton[focusIndicatorVisible=\"true\"] { border:2px solid %7; }"
            "QToolButton:focus { outline:none; }")
            .arg(background, foreground, edge, brand, subtle,
                 QString::number(keyContentHeight), focus));
        QFont font(QStringLiteral("Inter"));
        font.setPixelSize(14);
        font.setWeight(emphasized ? QFont::DemiBold : QFont::Normal);
        button->setFont(font);
    }
    update();
}
void Z_NumericKeypad::refreshAccessibleDescription() {
    const int enabledKeys = allowedKeyValues.isEmpty() ? keyValues.size() : allowedKeyValues.size();
    setAccessibleDescription(QStringLiteral("%1 numeric keypad with %2 enabled keys")
        .arg(densityValue).arg(enabledKeys));
}

Z_OverflowMenuButton::Z_OverflowMenuButton(const QString& profile, QWidget* parent)
    : QToolButton(parent), profileValue(profile) {
    identify(this, "ComponentLab/OverflowMenuButton", "commonWidget::Z_OverflowMenuButton");
    setText(QString::fromUtf8("•••"));
    setAccessibleName(QStringLiteral("More actions"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    FocusIndicator::instance().watch(this);
}
void Z_OverflowMenuButton::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Hover", "Pressed", "Focus", "Disabled"},
                 "OverflowMenuButton.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != "Disabled");
    setDown(value == "Pressed");
    update();
    emit snapshotStateChanged(value);
}
QSize Z_OverflowMenuButton::sizeHint() const { return {40, 40}; }
void Z_OverflowMenuButton::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QString state = snapshotStateValue;
    if (state.isEmpty()) {
        if (!isEnabled()) state = "Disabled";
        else if (isDown()) state = "Pressed";
        else if (underMouse()) state = "Hover";
        else if (FocusIndicator::instance().visibleFor(this)) state = "Focus";
        else state = "Default";
    }
    QColor background = tokens.c("base.stoken.color.background.surface");
    if (state == "Hover" || state == "Disabled")
        background = tokens.c("base.stoken.color.background.surface-tertiary");
    else if (state == "Pressed")
        background = tokens.c("base.stoken.color.background.pressed");
    const QColor border = state == "Disabled"
        ? tokens.c("base.stoken.color.border.disabled")
        : state == "Focus"
        ? tokens.c("base.stoken.color.interactive.brand.focus-ring")
        : tokens.c("base.stoken.color.border.default");
    const QColor foreground = state == "Disabled"
        ? tokens.c("base.stoken.color.text.disabled")
        : tokens.c("base.stoken.color.text.primary");
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(border, state == "Focus" ? 2 : 1));
    painter.setBrush(background);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
    painter.setPen(foreground);
    QFont font = componentFont(16);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(rect(), Qt::AlignCenter, text());
}

Z_SegmentedControl::Z_SegmentedControl(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), segmentLayout(new QHBoxLayout(this)) {
    identify(this, "ComponentLab/SegmentedControl", "commonWidget::Z_SegmentedControl");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Segmented control"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    segmentLayout->setContentsMargins(8, 9, 8, 9);
    segmentLayout->setSpacing(0);
    rebuild();
}
QString Z_SegmentedControl::currentText() const {
    return currentIndexValue >= 0 && currentIndexValue < itemValues.size()
        ? itemValues[currentIndexValue] : QString();
}
void Z_SegmentedControl::setItems(const QStringList& values) {
    if (values.size() < 2 || values.size() > 4)
        throw std::invalid_argument("SegmentedControl requires 2 to 4 items");
    for (const auto& value : values)
        if (value.trimmed().isEmpty())
            throw std::invalid_argument("SegmentedControl item text cannot be empty");
    if (itemValues == values) return;
    itemValues = values;
    currentIndexValue = qBound(0, currentIndexValue, itemValues.size() - 1);
    rebuild();
    updateGeometry();
    emit itemsChanged(values);
    emit segmentCountChanged(values.size());
}
void Z_SegmentedControl::setSegmentCount(int value) {
    if (value < 2 || value > 4)
        throw std::invalid_argument("SegmentedControl segmentCount must be 2, 3 or 4");
    static const QStringList defaults{QStringLiteral("Overview"), QStringLiteral("Parameters"),
                                      QStringLiteral("History"), QStringLiteral("Logs")};
    setItems(defaults.mid(0, value));
}
void Z_SegmentedControl::setCurrentIndex(int value) {
    if (value < 0 || value >= itemValues.size())
        throw std::out_of_range("SegmentedControl currentIndex is outside the item range");
    if (currentIndexValue == value) return;
    currentIndexValue = value;
    if (group && group->button(value)) group->button(value)->setChecked(true);
    refreshStyle();
    emit currentIndexChanged(value);
    emit currentTextChanged(currentText());
}
QSize Z_SegmentedControl::sizeHint() const { return {itemValues.size() * 104 + 16, 56}; }
bool Z_SegmentedControl::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        const int index = group ? group->id(qobject_cast<QAbstractButton*>(watched)) : -1;
        if (index >= 0) {
            const int key = static_cast<QKeyEvent*>(event)->key();
            int next = index;
            if (key == Qt::Key_Left || key == Qt::Key_Up) next = (index - 1 + itemValues.size()) % itemValues.size();
            else if (key == Qt::Key_Right || key == Qt::Key_Down) next = (index + 1) % itemValues.size();
            else if (key == Qt::Key_Home) next = 0;
            else if (key == Qt::Key_End) next = itemValues.size() - 1;
            else return QWidget::eventFilter(watched, event);
            setCurrentIndex(next);
            group->button(next)->setFocus(Qt::TabFocusReason);
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
void Z_SegmentedControl::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c(isEnabled() ? "base.stoken.color.border.default"
                                             : "base.stoken.color.border.disabled"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}
void Z_SegmentedControl::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) refreshStyle();
}
void Z_SegmentedControl::rebuild() {
    if (group) delete group;
    group = new QButtonGroup(this);
    group->setExclusive(true);
    while (auto* item = segmentLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (int index = 0; index < itemValues.size(); ++index) {
        auto* button = new QToolButton(this);
        button->setText(itemValues[index]);
        button->setCheckable(true);
        button->setChecked(index == currentIndexValue);
        button->setFixedSize(104, 38);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAccessibleName(itemValues[index]);
        button->installEventFilter(this);
        FocusIndicator::instance().watch(button);
        group->addButton(button, index);
        segmentLayout->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, index] { setCurrentIndex(index); });
    }
    refreshStyle();
}
void Z_SegmentedControl::refreshStyle() {
    if (!group) return;
    const Tokens tokens(profileValue);
    const QString disabled = tokens.c("base.stoken.color.text.disabled").name(QColor::HexArgb);
    const QString borderDisabled = tokens.c("base.stoken.color.border.disabled").name(QColor::HexArgb);
    const QString surface = tokens.c("base.stoken.color.background.surface").name(QColor::HexArgb);
    const QString border = tokens.c("base.stoken.color.border.default").name(QColor::HexArgb);
    const QString primary = tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb);
    const QString brand = tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb);
    const QString subtle = tokens.c("base.stoken.color.interactive.brand.subtle").name(QColor::HexArgb);
    for (auto* abstract : group->buttons()) {
        auto* button = qobject_cast<QToolButton*>(abstract);
        const bool selected = group->id(button) == currentIndexValue;
        const QString foreground = isEnabled() ? (selected ? brand : primary) : disabled;
        const QString edge = isEnabled() ? (selected ? brand : border) : borderDisabled;
        const QString background = selected ? subtle : surface;
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background:%1; color:%2; border:1px solid %3; border-radius:8px; padding:0 14px; }"
            "QToolButton:focus { outline:none; }").arg(background, foreground, edge));
        QFont font = componentFont(14);
        font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
        button->setFont(font);
    }
    update();
}

Z_Stepper::Z_Stepper(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    identify(this, "ComponentLab/Stepper", "commonWidget::Z_Stepper");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Workflow progress"));
    setAccessibleDescription(QStringLiteral("Current step: Parameters"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}
void Z_Stepper::setItems(const QStringList& values) {
    if (values.size() < 2 || values.size() > 6)
        throw std::invalid_argument("Stepper requires 2 to 6 items");
    for (const auto& value : values)
        if (value.trimmed().isEmpty())
            throw std::invalid_argument("Stepper item text cannot be empty");
    if (itemValues == values) return;
    itemValues = values;
    currentIndexValue = qBound(0, currentIndexValue, itemValues.size() - 1);
    setAccessibleDescription(QStringLiteral("Current step: ") + itemValues[currentIndexValue]);
    updateGeometry(); update(); emit itemsChanged(values);
}
void Z_Stepper::setCurrentIndex(int value) {
    if (value < 0 || value >= itemValues.size())
        throw std::out_of_range("Stepper currentIndex is outside the item range");
    if (currentIndexValue == value) return;
    currentIndexValue = value;
    setAccessibleDescription(QStringLiteral("Current step: ") + itemValues[value]);
    update(); emit currentIndexChanged(value);
}
void Z_Stepper::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Active", "Completed", "Error"},
                 "Stepper.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    update(); emit snapshotStateChanged(value);
}
QSize Z_Stepper::sizeHint() const { return {620, 88}; }
void Z_Stepper::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const int connectorWidth = 60;
    const int gap = 8;
    const QFont labelFont = componentFont(12);
    const QFontMetrics labelMetrics(labelFont);
    QList<int> itemWidths;
    int total = 0;
    for (const auto& item : itemValues) {
        const int itemWidth = qMax(28, labelMetrics.horizontalAdvance(item));
        itemWidths.append(itemWidth);
        total += itemWidth;
    }
    total += (itemValues.size() - 1) * (connectorWidth + gap * 2);
    int x = (width() - total) / 2;
    const QColor surface = tokens.c("base.stoken.color.background.surface");
    const QColor border = tokens.c("base.stoken.color.border.default");
    const QColor borderSecondary = tokens.c("base.stoken.color.border.secondary");
    const QColor primary = tokens.c("base.stoken.color.text.primary");
    const QColor brand = tokens.c("base.stoken.color.interactive.brand.default");
    const QColor subtle = tokens.c("base.stoken.color.interactive.brand.subtle");
    const QColor danger = tokens.c("base.stoken.color.status.danger.default");
    const QColor success = tokens.c("base.stoken.color.status.success.default");
    painter.setFont(labelFont);
    for (int index = 0; index < itemValues.size(); ++index) {
        const int itemWidth = itemWidths[index];
        const bool active = snapshotStateValue == "Active" && index == currentIndexValue;
        const bool completed = snapshotStateValue == "Completed" && index <= currentIndexValue;
        const bool error = snapshotStateValue == "Error" && index == currentIndexValue;
        const QRect indicator(x + (itemWidth - 28) / 2, 20, 28, 28);
        painter.setPen(QPen(error ? danger : (active || completed ? brand : border), 1));
        painter.setBrush(error ? danger : (active || completed ? subtle : surface));
        painter.drawRoundedRect(indicator, 4, 4);
        painter.setPen(error ? primary : (active || completed ? brand : primary));
        painter.drawText(indicator, Qt::AlignCenter, QString::number(index + 1));
        painter.setPen(primary);
        painter.drawText(QRect(x, 54, itemWidth, 18), Qt::AlignCenter, itemValues[index]);
        x += itemWidth;
        if (index + 1 < itemValues.size()) {
            x += gap;
            painter.fillRect(QRect(x, 33, connectorWidth, 2),
                             snapshotStateValue == "Completed" && index < currentIndexValue
                                 ? success : borderSecondary);
            x += connectorWidth + gap;
        }
    }
}

Z_Timeline::Z_Timeline(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    // Validate the profile immediately, matching the rest of the foundation batch.
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/Timeline", "commonWidget::Z_Timeline");
    for (const auto& path : {QStringLiteral(":/figma/timeline/created.svg"),
                             QStringLiteral(":/figma/timeline/processing.svg"),
                             QStringLiteral(":/figma/timeline/completed.svg")}) {
        QFile marker(path);
        if (!marker.open(QIODevice::ReadOnly) || !QSvgRenderer(marker.readAll()).isValid())
            throw std::runtime_error("Timeline marker resource is missing or invalid");
    }
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Timeline"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    refreshAccessibleDescription();
}
void Z_Timeline::setDensity(const QString& value) {
    requireOneOf(value, {"Comfortable", "Compact"}, "Timeline.density");
    if (densityValue == value) return;
    densityValue = value;
    updateGeometry();
    update();
    emit densityChanged(value);
}
void Z_Timeline::setEvents(const QStringList& labels, const QStringList& times,
                           const QStringList& states) {
    if (labels.isEmpty() || labels.size() > 12 || labels.size() != times.size() ||
        labels.size() != states.size())
        throw std::invalid_argument(
            "Timeline requires 1 to 12 labels, times and states of equal length");
    for (int index = 0; index < labels.size(); ++index) {
        if (labels[index].trimmed().isEmpty() || times[index].trimmed().isEmpty())
            throw std::invalid_argument("Timeline label and time cannot be empty");
        requireOneOf(states[index], {"Brand", "Process", "Success"},
                     "Timeline.eventState");
    }
    if (eventLabelValues == labels && eventTimeValues == times &&
        eventStateValues == states)
        return;
    eventLabelValues = labels;
    eventTimeValues = times;
    eventStateValues = states;
    refreshAccessibleDescription();
    update();
    emit eventsChanged();
}
QSize Z_Timeline::sizeHint() const {
    return {densityValue == QStringLiteral("Compact") ? 420 : 480, 190};
}
void Z_Timeline::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c("base.stoken.color.border.default"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);

    QFont titleFont(QStringLiteral("Inter"));
    titleFont.setPixelSize(13);
    titleFont.setWeight(QFont::Medium);
    QFont timeFont(QStringLiteral("Inter"));
    timeFont.setPixelSize(11);
    timeFont.setWeight(QFont::Normal);
    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics timeMetrics(timeFont);
    const int rowHeight = titleMetrics.height() + 3 + timeMetrics.height();
    const int rowGap = 8;
    const int totalHeight = eventLabelValues.size() * rowHeight +
                            qMax(0, eventLabelValues.size() - 1) * rowGap;
    int y = (height() - totalHeight) / 2;

    for (int index = 0; index < eventLabelValues.size(); ++index) {
        const int textWidth = qMax(titleMetrics.horizontalAdvance(eventLabelValues[index]),
                                   timeMetrics.horizontalAdvance(eventTimeValues[index]));
        const int rowWidth = 12 + 12 + textWidth;
        const int x = (width() - rowWidth) / 2;
        const QRect markerRect(x, y + (rowHeight - 12) / 2, 12, 12);
        QString markerPath;
        QString sourceColor;
        QColor targetColor;
        if (eventStateValues[index] == QStringLiteral("Brand")) {
            markerPath = QStringLiteral(":/figma/timeline/created.svg");
            sourceColor = QStringLiteral("#6235F7");
            targetColor = tokens.c("base.stoken.color.interactive.brand.default");
        } else if (eventStateValues[index] == QStringLiteral("Process")) {
            markerPath = QStringLiteral(":/figma/timeline/processing.svg");
            sourceColor = QStringLiteral("#C6C6C6");
            targetColor = tokens.c("base.stoken.color.status.process");
        } else {
            markerPath = QStringLiteral(":/figma/timeline/completed.svg");
            sourceColor = QStringLiteral("#F3FAF6");
            targetColor = tokens.c("base.stoken.color.status.success.default");
        }
        QFile markerFile(markerPath);
        if (!markerFile.open(QIODevice::ReadOnly)) return;
        QString svg = QString::fromUtf8(markerFile.readAll());
        svg.replace(sourceColor, targetColor.name(QColor::HexRgb), Qt::CaseInsensitive);
        QSvgRenderer renderer(svg.toUtf8());
        if (!renderer.isValid()) return;
        renderer.render(&painter, markerRect);

        painter.setPen(tokens.c("base.stoken.color.text.primary"));
        painter.setFont(titleFont);
        painter.drawText(QRect(x + 24, y, textWidth, titleMetrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, eventLabelValues[index]);
        painter.setPen(tokens.c("base.stoken.color.text.secondary"));
        painter.setFont(timeFont);
        painter.drawText(QRect(x + 24, y + titleMetrics.height() + 3, textWidth,
                               timeMetrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, eventTimeValues[index]);
        y += rowHeight + rowGap;
    }
}
void Z_Timeline::refreshAccessibleDescription() {
    if (eventLabelValues.isEmpty()) {
        setAccessibleDescription(QStringLiteral("No timeline events"));
        return;
    }
    setAccessibleDescription(
        QStringLiteral("%1 events; latest %2 at %3")
            .arg(eventLabelValues.size())
            .arg(eventLabelValues.last(), eventTimeValues.last()));
}

Z_AnchorNavigation::Z_AnchorNavigation(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), itemLayout(new QBoxLayout(QBoxLayout::TopToBottom, this)) {
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/AnchorNavigation", "commonWidget::Z_AnchorNavigation");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Anchor navigation"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rebuild();
}
QString Z_AnchorNavigation::snapshotState() const {
    return currentIndexValue == 0 ? QStringLiteral("Default") : QStringLiteral("Scrolled");
}
QString Z_AnchorNavigation::currentText() const {
    return currentIndexValue >= 0 && currentIndexValue < itemValues.size()
        ? itemValues[currentIndexValue] : QString();
}
void Z_AnchorNavigation::setOrientation(const QString& value) {
    requireOneOf(value, {"Vertical", "Horizontal"}, "AnchorNavigation.orientation");
    if (orientationValue == value) return;
    orientationValue = value;
    rebuild();
    updateGeometry();
    emit orientationChanged(value);
}
void Z_AnchorNavigation::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Scrolled"}, "AnchorNavigation.snapshotState");
    const int target = value == QStringLiteral("Default") ? 0 : qMin(1, itemValues.size() - 1);
    setCurrentIndex(target);
}
void Z_AnchorNavigation::setItems(const QStringList& values) {
    if (values.size() < 2 || values.size() > 8)
        throw std::invalid_argument("AnchorNavigation requires 2 to 8 items");
    for (const auto& value : values)
        if (value.trimmed().isEmpty())
            throw std::invalid_argument("AnchorNavigation item text cannot be empty");
    if (itemValues == values) return;
    const int previousIndex = currentIndexValue;
    const QString previousState = snapshotState();
    itemValues = values;
    currentIndexValue = qBound(0, currentIndexValue, itemValues.size() - 1);
    rebuild();
    refreshAccessibleDescription();
    updateGeometry();
    emit itemsChanged(values);
    if (previousIndex != currentIndexValue)
        emit currentIndexChanged(currentIndexValue);
    emit currentTextChanged(currentText());
    if (previousState != snapshotState())
        emit snapshotStateChanged(snapshotState());
}
void Z_AnchorNavigation::setCurrentIndex(int value) {
    if (value < 0 || value >= itemValues.size())
        throw std::out_of_range("AnchorNavigation currentIndex is outside the item range");
    if (currentIndexValue == value) return;
    const QString previousState = snapshotState();
    currentIndexValue = value;
    refreshStyle();
    refreshAccessibleDescription();
    emit currentIndexChanged(value);
    emit currentTextChanged(currentText());
    if (previousState != snapshotState())
        emit snapshotStateChanged(snapshotState());
}
QSize Z_AnchorNavigation::sizeHint() const {
    if (orientationValue == QStringLiteral("Horizontal"))
        return {qMax(520, 28 + itemValues.size() * 150 + qMax(0, itemValues.size() - 1) * 10),
                64};
    return {240,
            qMax(180, 24 + itemValues.size() * 36 + qMax(0, itemValues.size() - 1) * 10)};
}
bool Z_AnchorNavigation::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress && group) {
        const int index = group->id(qobject_cast<QAbstractButton*>(watched));
        if (index >= 0) {
            const int key = static_cast<QKeyEvent*>(event)->key();
            int next = index;
            if (key == Qt::Key_Home) next = 0;
            else if (key == Qt::Key_End) next = itemValues.size() - 1;
            else if ((orientationValue == QStringLiteral("Horizontal") && key == Qt::Key_Left) ||
                     (orientationValue == QStringLiteral("Vertical") && key == Qt::Key_Up))
                next = (index - 1 + itemValues.size()) % itemValues.size();
            else if ((orientationValue == QStringLiteral("Horizontal") && key == Qt::Key_Right) ||
                     (orientationValue == QStringLiteral("Vertical") && key == Qt::Key_Down))
                next = (index + 1) % itemValues.size();
            else
                return QWidget::eventFilter(watched, event);
            group->button(next)->setFocus(Qt::TabFocusReason);
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
void Z_AnchorNavigation::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c(isEnabled() ? "base.stoken.color.border.default"
                                             : "base.stoken.color.border.disabled"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}
void Z_AnchorNavigation::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) refreshStyle();
}
void Z_AnchorNavigation::rebuild() {
    if (group) delete group;
    group = new QButtonGroup(this);
    group->setExclusive(false);
    while (auto* item = itemLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    itemLayout->setDirection(orientationValue == QStringLiteral("Horizontal")
                                 ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    itemLayout->setContentsMargins(14, 12, 14, 12);
    itemLayout->setSpacing(10);
    itemLayout->addStretch(1);
    const Qt::Alignment alignment = orientationValue == QStringLiteral("Horizontal")
        ? Qt::AlignVCenter : Qt::AlignLeft;
    for (int index = 0; index < itemValues.size(); ++index) {
        auto* button = new QToolButton(this);
        button->setText(itemValues[index]);
        button->setFixedSize(orientationValue == QStringLiteral("Horizontal") ? 150 : 200, 36);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAccessibleName(itemValues[index]);
        button->installEventFilter(this);
        FocusIndicator::instance().watch(button);
        group->addButton(button, index);
        itemLayout->addWidget(button, 0, alignment);
        // Rebuild can occur while the parent is already visible. Newly created
        // child widgets are hidden until explicitly shown in that lifecycle.
        button->show();
        connect(button, &QToolButton::clicked, this, [this, index] {
            emit anchorActivated(index, itemValues[index]);
        });
    }
    itemLayout->addStretch(1);
    refreshStyle();
    refreshAccessibleDescription();
}
void Z_AnchorNavigation::refreshStyle() {
    if (!group) return;
    const Tokens tokens(profileValue);
    const QString disabled = tokens.c("base.stoken.color.text.disabled").name(QColor::HexArgb);
    const QString borderDisabled = tokens.c("base.stoken.color.border.disabled").name(QColor::HexArgb);
    const QString surface = tokens.c("base.stoken.color.background.surface").name(QColor::HexArgb);
    const QString border = tokens.c("base.stoken.color.border.default").name(QColor::HexArgb);
    const QString primary = tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb);
    const QString brand = tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb);
    const QString subtle = tokens.c("base.stoken.color.interactive.brand.subtle").name(QColor::HexArgb);
    const QString focus = FocusIndicator::instance().color(
        tokens.c("base.stoken.color.interactive.brand.default")).name(QColor::HexArgb);
    for (auto* abstract : group->buttons()) {
        auto* button = qobject_cast<QToolButton*>(abstract);
        const bool selected = group->id(button) == currentIndexValue;
        button->setProperty("currentAnchor", selected);
        button->setAccessibleDescription(selected ? QStringLiteral("Current section") : QString());
        const QString foreground = isEnabled() ? (selected ? brand : primary) : disabled;
        const QString edge = isEnabled() ? (selected ? brand : border) : borderDisabled;
        const QString background = selected ? subtle : surface;
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background:%1; color:%2; border:1px solid %3; border-radius:7px; padding:0 10px; }"
            "QToolButton[focusIndicatorVisible=\"true\"] { border:2px solid %4; }"
            "QToolButton:focus { outline:none; }").arg(background, foreground, edge, focus));
        QFont font(QStringLiteral("Inter"));
        font.setPixelSize(13);
        font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
        button->setFont(font);
    }
    update();
}
void Z_AnchorNavigation::refreshAccessibleDescription() {
    setAccessibleDescription(QStringLiteral("Current section: ") + currentText());
}

Z_Filter::Z_Filter(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile) {
    const Tokens tokens(profileValue);
    Q_UNUSED(tokens);
    identify(this, "ComponentLab/Filter", "commonWidget::Z_Filter");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Filter"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    for (int index = 0; index < criterionValues.size(); ++index) {
        auto* button = new QToolButton(this);
        button->setObjectName(QStringLiteral("filterCriterion%1").arg(index));
        button->setFixedSize(index == 0 ? 145 : 110, 36);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setProperty("filterRole", QStringLiteral("criterion"));
        button->installEventFilter(this);
        FocusIndicator::instance().watch(button);
        criterionButtons.append(button);
        connect(button, &QToolButton::clicked, this, [this, index] {
            emit criterionActivated(index, criterionValues[index]);
        });
    }
    applyButtonValue = new QToolButton(this);
    applyButtonValue->setObjectName(QStringLiteral("filterApply"));
    applyButtonValue->setFixedSize(90, 36);
    applyButtonValue->setFocusPolicy(Qt::StrongFocus);
    applyButtonValue->setProperty("filterRole", QStringLiteral("apply"));
    applyButtonValue->setAccessibleDescription(QStringLiteral("Apply filter criteria"));
    applyButtonValue->installEventFilter(this);
    FocusIndicator::instance().watch(applyButtonValue);
    connect(applyButtonValue, &QToolButton::clicked, this, &Z_Filter::applyRequested);
    layoutChildren();
    refreshStyle();
    refreshAccessibleDescription();
}
void Z_Filter::setLayoutMode(const QString& value) {
    requireOneOf(value, {"Inline", "Panel"}, "Filter.layoutMode");
    if (layoutModeValue == value) return;
    layoutModeValue = value;
    layoutChildren();
    updateGeometry();
    emit layoutModeChanged(value);
}
void Z_Filter::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Applied"}, "Filter.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    refreshStyle();
    refreshAccessibleDescription();
    emit snapshotStateChanged(value);
}
void Z_Filter::setCriteria(const QStringList& values) {
    if (values.size() != 2)
        throw std::invalid_argument("Filter requires exactly two visible criterion slots");
    for (const auto& value : values)
        if (value.trimmed().isEmpty())
            throw std::invalid_argument("Filter criterion text cannot be empty");
    if (criterionValues == values) return;
    criterionValues = values;
    refreshStyle();
    refreshAccessibleDescription();
    emit criteriaChanged(values);
}
void Z_Filter::setApplyLabel(const QString& value) {
    if (value.trimmed().isEmpty())
        throw std::invalid_argument("Filter apply label cannot be empty");
    if (applyLabelValue == value) return;
    applyLabelValue = value;
    refreshStyle();
    emit applyLabelChanged(value);
}
QSize Z_Filter::sizeHint() const {
    return layoutModeValue == QStringLiteral("Panel") ? QSize(430, 112) : QSize(430, 58);
}
bool Z_Filter::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(watched, event);
    QList<QToolButton*> order;
    for (const auto& button : criterionButtons)
        if (button) order.append(button);
    if (applyButtonValue) order.append(applyButtonValue);
    const int index = order.indexOf(qobject_cast<QToolButton*>(watched));
    if (index < 0) return QWidget::eventFilter(watched, event);
    const int key = static_cast<QKeyEvent*>(event)->key();
    int next = index;
    if (key == Qt::Key_Home) next = 0;
    else if (key == Qt::Key_End) next = order.size() - 1;
    else if ((layoutModeValue == QStringLiteral("Inline") && key == Qt::Key_Left) ||
             (layoutModeValue == QStringLiteral("Panel") && key == Qt::Key_Up))
        next = (index - 1 + order.size()) % order.size();
    else if ((layoutModeValue == QStringLiteral("Inline") && key == Qt::Key_Right) ||
             (layoutModeValue == QStringLiteral("Panel") && key == Qt::Key_Down))
        next = (index + 1) % order.size();
    else
        return QWidget::eventFilter(watched, event);
    order[next]->setFocus(Qt::TabFocusReason);
    event->accept();
    return true;
}
void Z_Filter::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c(isEnabled() ? "base.stoken.color.border.default"
                                             : "base.stoken.color.border.disabled"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}
void Z_Filter::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    layoutChildren();
}
void Z_Filter::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) refreshStyle();
}
void Z_Filter::layoutChildren() {
    if (criterionButtons.size() != 2 || !applyButtonValue) return;
    if (layoutModeValue == QStringLiteral("Panel")) {
        // Current Figma geometry intentionally places the first and last rows
        // eight pixels beyond the nominal 430x112 root (nodes 2148:230/237).
        criterionButtons[0]->setGeometry(15, -8, 145, 36);
        criterionButtons[1]->setGeometry(15, 38, 110, 36);
        applyButtonValue->setGeometry(15, 84, 90, 36);
        return;
    }
    const int totalWidth = 145 + 10 + 110 + 10 + 90;
    const int startX = (width() - totalWidth + 1) / 2;
    const int y = (height() - 36) / 2;
    criterionButtons[0]->setGeometry(startX, y, 145, 36);
    criterionButtons[1]->setGeometry(startX + 155, y, 110, 36);
    applyButtonValue->setGeometry(startX + 275, y, 90, 36);
}
void Z_Filter::refreshStyle() {
    if (criterionButtons.size() != 2 || !applyButtonValue) return;
    const Tokens tokens(profileValue);
    const QString disabled = tokens.c("base.stoken.color.text.disabled").name(QColor::HexArgb);
    const QString borderDisabled = tokens.c("base.stoken.color.border.disabled").name(QColor::HexArgb);
    const QString surface = tokens.c("base.stoken.color.background.surface").name(QColor::HexArgb);
    const QString border = tokens.c("base.stoken.color.border.default").name(QColor::HexArgb);
    const QString primary = tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb);
    const QString brand = tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb);
    const QString subtle = tokens.c("base.stoken.color.interactive.brand.subtle").name(QColor::HexArgb);
    const QString focus = FocusIndicator::instance().color(
        tokens.c("base.stoken.color.interactive.brand.default")).name(QColor::HexArgb);
    QList<QToolButton*> allButtons{criterionButtons[0], criterionButtons[1], applyButtonValue};
    for (int index = 0; index < allButtons.size(); ++index) {
        auto* button = allButtons[index];
        const bool active = index == 2 ||
            (snapshotStateValue == QStringLiteral("Applied") && index == 0);
        const QString foreground = isEnabled() ? (active ? brand : primary) : disabled;
        const QString edge = isEnabled() ? (active ? brand : border) : borderDisabled;
        const QString background = active ? subtle : surface;
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background:%1; color:%2; border:1px solid %3; border-radius:8px; padding:0 14px; }"
            "QToolButton[focusIndicatorVisible=\"true\"] { border:2px solid %4; }"
            "QToolButton:focus { outline:none; }").arg(background, foreground, edge, focus));
        QFont font(QStringLiteral("Inter"));
        font.setPixelSize(14);
        font.setWeight(active ? QFont::DemiBold : QFont::Normal);
        button->setFont(font);
        button->setAccessibleDescription(active && index == 0
            ? QStringLiteral("Applied criterion") : QString());
    }
    criterionButtons[0]->setText(criterionValues[0]);
    criterionButtons[1]->setText(criterionValues[1]);
    applyButtonValue->setText(applyLabelValue);
    criterionButtons[0]->setAccessibleName(criterionValues[0]);
    criterionButtons[1]->setAccessibleName(criterionValues[1]);
    applyButtonValue->setAccessibleName(applyLabelValue);
    update();
}
void Z_Filter::refreshAccessibleDescription() {
    setAccessibleDescription(QStringLiteral("%1 filter with %2 criteria")
        .arg(snapshotStateValue).arg(criterionValues.size()));
}

Z_Pagination::Z_Pagination(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), pageLayout(new QHBoxLayout(this)) {
    identify(this, "ComponentLab/Pagination", "commonWidget::Z_Pagination");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Pagination"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rebuild();
}
void Z_Pagination::setDensity(const QString& value) {
    requireOneOf(value, {"Default", "Compact"}, "Pagination.density");
    if (densityValue == value) return;
    densityValue = value; rebuild(); updateGeometry(); emit densityChanged(value);
}
void Z_Pagination::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Disabled"}, "Pagination.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != "Disabled");
    refreshStyle(); emit snapshotStateChanged(value);
}
void Z_Pagination::setCurrentPage(int value) {
    if (value < 1 || value > pageCountValue)
        throw std::out_of_range("Pagination currentPage is outside 1..pageCount");
    if (currentPageValue == value) return;
    currentPageValue = value; rebuild(); emit currentPageChanged(value);
}
void Z_Pagination::setPageCount(int value) {
    if (value < 1)
        throw std::invalid_argument("Pagination pageCount must be positive");
    if (pageCountValue == value) return;
    pageCountValue = value;
    currentPageValue = qMin(currentPageValue, pageCountValue);
    rebuild(); emit pageCountChanged(value);
}
QList<int> Z_Pagination::displayPages() const {
    QList<int> pages;
    if (pageCountValue <= 7) {
        for (int page = 1; page <= pageCountValue; ++page)
            pages.append(page);
        return pages;
    }
    QList<int> candidates{1};
    for (int page = qMax(2, currentPageValue - 2);
         page <= qMin(pageCountValue - 1, currentPageValue + 2); ++page)
        candidates.append(page);
    candidates.append(pageCountValue);
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    int previous = 0;
    for (const int page : candidates) {
        if (previous > 0 && page - previous > 1)
            pages.append(-1);
        pages.append(page);
        previous = page;
    }
    return pages;
}
QSize Z_Pagination::sizeHint() const {
    const Tokens tokens(profileValue);
    const bool compact = densityValue == QStringLiteral("Compact");
    const QByteArray prefix = compact ? "base.stoken.pagination.base.compact"
                                      : "base.stoken.pagination.base.default";
    const int itemWidth = tokens.n((prefix + ".item.visual-width").constData());
    const int itemHeight = tokens.n((prefix + ".item.visual-height").constData());
    const int paddingInline = tokens.n((prefix + ".container.padding-inline").constData());
    const int paddingBlock = tokens.n((prefix + ".container.padding-block").constData());
    const int gap = tokens.n((prefix + ".container.gap").constData());
    const auto pages = displayPages();
    const int ellipses = std::count(pages.cbegin(), pages.cend(), -1);
    const int controls = pages.size() + 2;
    const int width = paddingInline * 2 + (controls - ellipses) * itemWidth +
                      ellipses * (compact ? 16 : 20) + qMax(0, controls - 1) * gap;
    return {width, paddingBlock * 2 + itemHeight};
}
void Z_Pagination::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}
void Z_Pagination::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) refreshStyle();
}
void Z_Pagination::rebuild() {
    while (auto* item = pageLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    pageButtons.clear();
    ellipsisLabels.clear();
    const Tokens tokens(profileValue);
    const bool compact = densityValue == QStringLiteral("Compact");
    const QByteArray prefix = compact ? "base.stoken.pagination.base.compact"
                                      : "base.stoken.pagination.base.default";
    const int itemWidth = tokens.n((prefix + ".item.visual-width").constData());
    const int itemHeight = tokens.n((prefix + ".item.visual-height").constData());
    const int paddingInline = tokens.n((prefix + ".container.padding-inline").constData());
    const int paddingBlock = tokens.n((prefix + ".container.padding-block").constData());
    pageLayout->setContentsMargins(paddingInline, paddingBlock, paddingInline, paddingBlock);
    pageLayout->setSpacing(tokens.n((prefix + ".container.gap").constData()));
    const auto add = [this](const QString& text, int page, const QString& objectName) {
        auto* button = new QToolButton(this);
        button->setObjectName(objectName);
        button->setText(text);
        button->setProperty("pageNumber", page);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAccessibleName(page > 0 ? QStringLiteral("Page %1").arg(page) : text);
        FocusIndicator::instance().watch(button);
        connect(button, &QToolButton::clicked, this, [this, page] {
            if (page >= 1 && page <= pageCountValue) setCurrentPage(page);
        });
        pageLayout->addWidget(button);
        pageButtons.append(button);
    };
    add(QString::fromUtf8("‹"), currentPageValue - 1, QStringLiteral("previousPage"));
    for (const int page : displayPages()) {
        if (page < 0) {
            auto* ellipsis = new QLabel(QString::fromUtf8("…"), this);
            ellipsis->setObjectName(QStringLiteral("paginationEllipsis"));
            ellipsis->setAlignment(Qt::AlignCenter);
            ellipsis->setAccessibleName(QStringLiteral("Skipped pages"));
            ellipsis->setFixedSize(compact ? 16 : 20, itemHeight);
            pageLayout->addWidget(ellipsis);
            ellipsisLabels.append(ellipsis);
        } else {
            add(QString::number(page), page, QStringLiteral("page_%1").arg(page));
        }
    }
    add(QString::fromUtf8("›"), currentPageValue + 1, QStringLiteral("nextPage"));
    for (auto* button : pageButtons)
        button->setFixedSize(itemWidth, itemHeight);
    setAccessibleDescription(QStringLiteral("Page %1 of %2")
                                 .arg(currentPageValue).arg(pageCountValue));
    refreshStyle();
}
void Z_Pagination::refreshStyle() {
    if (pageButtons.isEmpty()) return;
    const Tokens tokens(profileValue);
    const QString disabled = tokens.c("base.stoken.pagination.base.disabled.item.label-color").name(QColor::HexArgb);
    const QString disabledBorder = tokens.c("base.stoken.pagination.base.disabled.navigation.border-color").name(QColor::HexArgb);
    const QString disabledBackground = tokens.c("base.stoken.pagination.base.disabled.navigation.background").name(QColor::HexArgb);
    const QString disabledIcon = tokens.c("base.stoken.pagination.base.disabled.navigation.icon-color").name(QColor::HexArgb);
    const QString primary = tokens.c("base.stoken.pagination.base.default.item.label-color").name(QColor::HexArgb);
    const QString surface = tokens.c("base.stoken.pagination.base.default.navigation.background").name(QColor::HexArgb);
    const QString border = tokens.c("base.stoken.pagination.base.default.navigation.border-color").name(QColor::HexArgb);
    const QString navigationIcon = tokens.c("base.stoken.pagination.base.default.navigation.icon-color").name(QColor::HexArgb);
    const QString selectedLabel = tokens.c("base.stoken.pagination.base.selected.item.label-color").name(QColor::HexArgb);
    const QString selectedBackground = tokens.c("base.stoken.pagination.base.selected.item.background").name(QColor::HexArgb);
    const QString hoverBackground = tokens.c("base.stoken.pagination.base.hover.item.background").name(QColor::HexArgb);
    const QString focus = tokens.c("base.stoken.pagination.base.focused.focus-ring.color").name(QColor::HexArgb);
    const int itemRadius = tokens.n("base.stoken.pagination.base.default.item.radius");
    const int navigationRadius = tokens.n("base.stoken.pagination.base.default.navigation.radius");
    const int borderWidth = tokens.n("base.stoken.pagination.base.default.border-width");
    for (auto* button : pageButtons) {
        const int page = button->property("pageNumber").toInt();
        const bool selected = page == currentPageValue && button->objectName().startsWith("page_");
        const bool navigation = button->objectName() == QStringLiteral("previousPage") ||
                                button->objectName() == QStringLiteral("nextPage");
        const bool boundaryEnabled = page >= 1 && page <= pageCountValue;
        button->setEnabled(isEnabled() && boundaryEnabled);
        const bool usable = button->isEnabled();
        const QString foreground = usable
            ? (selected ? selectedLabel : navigation ? navigationIcon : primary)
            : (navigation ? disabledIcon : disabled);
        const QString edge = navigation ? (usable ? border : disabledBorder)
                                        : (selected ? (usable ? selectedBackground : disabledBorder)
                                                    : QStringLiteral("transparent"));
        const QString background = navigation ? (usable ? surface : disabledBackground)
                                              : (selected ? (usable ? selectedBackground : disabledBackground)
                                                          : QStringLiteral("transparent"));
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background:%1; color:%2; border:%3px solid %4; border-radius:%5px; padding:0; }"
            "QToolButton:hover:enabled { background:%6; color:%7; }"
            "QToolButton[focusIndicatorVisible=\"true\"] { border:2px solid %8; }"
            "QToolButton:focus { outline:none; }")
            .arg(background, foreground).arg(borderWidth).arg(edge)
            .arg(navigation ? navigationRadius : itemRadius)
            .arg(selected ? selectedBackground : hoverBackground)
            .arg(selected ? selectedLabel : navigation ? navigationIcon : primary)
            .arg(focus));
        QFont font = componentFont(14);
        font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
        button->setFont(font);
    }
    for (auto* ellipsis : ellipsisLabels) {
        ellipsis->setStyleSheet(QStringLiteral("color:%1; background:transparent;")
                                    .arg(isEnabled() ? primary : disabled));
        ellipsis->setFont(componentFont(14));
    }
    update();
}

Z_PageHeader::Z_PageHeader(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), titleLabel(new QLabel(titleValue, this)),
      contextLabel(new QLabel(contextValue, this)), actionControl(new QToolButton(this)),
      headerLayout(new QVBoxLayout(this)) {
    identify(this, "ComponentLab/PageHeader", "commonWidget::Z_PageHeader");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(titleValue);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto* titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(8);
    titleRow->addWidget(titleLabel, 1);
    actionControl->setObjectName(QStringLiteral("pageHeaderAction"));
    actionControl->setText(actionValue);
    actionControl->setFixedSize(82, 34);
    actionControl->setFocusPolicy(Qt::StrongFocus);
    actionControl->setAccessibleName(actionValue);
    FocusIndicator::instance().watch(actionControl);
    titleRow->addWidget(actionControl);
    headerLayout->setContentsMargins(14, 12, 14, 12);
    headerLayout->setSpacing(7);
    headerLayout->addLayout(titleRow);
    headerLayout->addWidget(contextLabel);
    connect(actionControl, &QToolButton::clicked, this, &Z_PageHeader::actionTriggered);
    refreshStyle();
}
void Z_PageHeader::setTitle(const QString& value) {
    if (titleValue == value) return;
    titleValue = value; titleLabel->setText(value); setAccessibleName(value);
    emit titleChanged(value);
}
void Z_PageHeader::setContextText(const QString& value) {
    if (contextValue == value) return;
    contextValue = value; contextLabel->setText(value); emit contextTextChanged(value);
}
void Z_PageHeader::setActionText(const QString& value) {
    if (actionValue == value) return;
    actionValue = value; actionControl->setText(value); actionControl->setAccessibleName(value);
    emit actionTextChanged(value);
}
void Z_PageHeader::setDensity(const QString& value) {
    requireOneOf(value, {"Comfortable", "Compact"}, "PageHeader.density");
    if (densityValue == value) return;
    densityValue = value; updateGeometry(); emit densityChanged(value);
}
void Z_PageHeader::setActionsVisible(bool value) {
    if (actionsVisibleValue == value) return;
    actionsVisibleValue = value; actionControl->setVisible(value);
    emit actionsVisibleChanged(value);
}
QSize Z_PageHeader::sizeHint() const {
    return {420, densityValue == "Compact" ? 92 : 118};
}
void Z_PageHeader::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c("base.stoken.color.border.default"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}
void Z_PageHeader::refreshStyle() {
    const Tokens tokens(profileValue);
    titleLabel->setFont(componentFont(20));
    QFont titleFont = titleLabel->font(); titleFont.setWeight(QFont::DemiBold); titleLabel->setFont(titleFont);
    contextLabel->setFont(componentFont(13));
    titleLabel->setStyleSheet(QStringLiteral("color:%1;")
                                 .arg(tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb)));
    contextLabel->setStyleSheet(QStringLiteral("color:%1;")
                                   .arg(tokens.c("base.stoken.color.text.secondary").name(QColor::HexArgb)));
    actionControl->setStyleSheet(QStringLiteral(
        "QToolButton { background:%1; color:%2; border:1px solid %2; border-radius:8px; padding:0 14px; }")
        .arg(tokens.c("base.stoken.color.interactive.brand.subtle").name(QColor::HexArgb),
             tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb)));
    QFont actionFont = componentFont(14); actionFont.setWeight(QFont::DemiBold);
    actionControl->setFont(actionFont);
}

Z_TitleBar::Z_TitleBar(const QString& profile, QWidget* parent)
    : QWidget(parent), profileValue(profile), titleLabel(new QLabel(this)),
      minimizeButton(new QToolButton(this)), maximizeButton(new QToolButton(this)),
      closeButton(new QToolButton(this)) {
    identify(this, "ComponentLab/TitleBar", "commonWidget::Z_TitleBar");
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Application title bar"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(14, 12, 14, 12);
    outer->setSpacing(8);
    auto* application = new QWidget(this);
    auto* appLayout = new QHBoxLayout(application);
    appLayout->setContentsMargins(0, 0, 0, 0);
    appLayout->setSpacing(10);
    auto* mark = new QLabel(QString::fromUtf8("◆"), application);
    mark->setObjectName(QStringLiteral("applicationMark"));
    appLayout->addWidget(mark);
    appLayout->addWidget(titleLabel);
    outer->addWidget(application, 1);
    const auto configure = [this, outer](QToolButton* button, const QString& name,
                                         const QString& text, const QString& accessible) {
        button->setObjectName(name);
        button->setText(text);
        button->setFixedSize(44, 32);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAccessibleName(accessible);
        FocusIndicator::instance().watch(button);
        outer->addWidget(button);
    };
    configure(minimizeButton, QStringLiteral("minimizeWindow"), QString::fromUtf8("—"),
              QStringLiteral("Minimize window"));
    configure(maximizeButton, QStringLiteral("maximizeRestoreWindow"), QString::fromUtf8("□"),
              QStringLiteral("Maximize window"));
    configure(closeButton, QStringLiteral("closeWindow"), QString::fromUtf8("×"),
              QStringLiteral("Close window"));
    connect(minimizeButton, &QToolButton::clicked, this, &Z_TitleBar::minimizeRequested);
    connect(closeButton, &QToolButton::clicked, this, &Z_TitleBar::closeRequested);
    connect(maximizeButton, &QToolButton::clicked, this, [this] {
        const bool maximize = !isMaximizedState();
        setSnapshotState(maximize ? QStringLiteral("Maximized") : QStringLiteral("Default"));
        emit maximizeRestoreRequested(maximize);
    });
    refreshStyle();
}
void Z_TitleBar::setTitle(const QString& value) {
    if (titleValue == value) return;
    titleValue = value; refreshStyle(); emit titleChanged(value);
}
void Z_TitleBar::setSnapshotState(const QString& value) {
    requireOneOf(value, {"Default", "Maximized"}, "TitleBar.snapshotState");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value; refreshStyle(); emit snapshotStateChanged(value);
}
QSize Z_TitleBar::sizeHint() const { return {700, 58}; }
void Z_TitleBar::paintEvent(QPaintEvent*) {
    const Tokens tokens(profileValue);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(tokens.c("base.stoken.color.border.default"), 1));
    painter.setBrush(tokens.c("base.stoken.color.background.surface"));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}
void Z_TitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit dragStarted(event->globalPos());
    QWidget::mousePressEvent(event);
}
void Z_TitleBar::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons().testFlag(Qt::LeftButton)) emit dragMoved(event->globalPos());
    QWidget::mouseMoveEvent(event);
}
void Z_TitleBar::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit dragEnded();
    QWidget::mouseReleaseEvent(event);
}
void Z_TitleBar::refreshStyle() {
    const Tokens tokens(profileValue);
    titleLabel->setText(titleValue);
    titleLabel->setStyleSheet(QStringLiteral("color:%1;")
                                 .arg(tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb)));
    QFont titleFont = componentFont(14); titleFont.setWeight(QFont::DemiBold); titleLabel->setFont(titleFont);
    if (auto* mark = findChild<QLabel*>(QStringLiteral("applicationMark"))) {
        mark->setStyleSheet(QStringLiteral("color:%1;")
                                .arg(tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb)));
        mark->setFont(componentFont(16));
    }
    const QString surface = tokens.c("base.stoken.color.background.surface").name(QColor::HexArgb);
    const QString border = tokens.c("base.stoken.color.border.default").name(QColor::HexArgb);
    const QString primary = tokens.c("base.stoken.color.text.primary").name(QColor::HexArgb);
    const QString brand = tokens.c("base.stoken.color.interactive.brand.default").name(QColor::HexArgb);
    const QString subtle = tokens.c("base.stoken.color.interactive.brand.subtle").name(QColor::HexArgb);
    const QString normal = QStringLiteral(
        "QToolButton { background:%1; color:%2; border:1px solid %3; border-radius:7px; }")
        .arg(surface, primary, border);
    minimizeButton->setStyleSheet(normal);
    maximizeButton->setStyleSheet(normal);
    maximizeButton->setText(isMaximizedState() ? QString::fromUtf8("❐") : QString::fromUtf8("□"));
    maximizeButton->setAccessibleName(isMaximizedState() ? QStringLiteral("Restore window")
                                                          : QStringLiteral("Maximize window"));
    closeButton->setStyleSheet(QStringLiteral(
        "QToolButton { background:%1; color:%2; border:1px solid %2; border-radius:7px; }")
        .arg(subtle, brand));
    minimizeButton->setFont(componentFont(13));
    maximizeButton->setFont(componentFont(13));
    QFont closeFont = componentFont(13); closeFont.setWeight(QFont::DemiBold); closeButton->setFont(closeFont);
}

} // namespace commonWidget
