#include "Z_DesignComponents.h"
#include "FocusIndicator.h"
#include "FieldChrome.h"
#include "TokenResources.h"
#include "Z_InputField.h"

#include <QApplication>
#include <QBrush>
#include <QButtonGroup>
#include <QFormLayout>
#include <QFrame>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOptionButton>
#include <QSvgRenderer>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {

QVariantMap parseVariantTuple(const QString& tuple) {
    QVariantMap result;
    for (const auto& part : tuple.split(',', Qt::SkipEmptyParts)) {
        const int separator = part.indexOf('=');
        if (separator <= 0)
            continue;
        result.insert(part.left(separator).trimmed(), part.mid(separator + 1).trimmed());
    }
    return result;
}

QString tupleFromMap(const QVariantMap& properties) {
    QStringList parts;
    for (auto it = properties.cbegin(); it != properties.cend(); ++it)
        parts.append(it.key() + '=' + it.value().toString());
    return parts.join(QStringLiteral(", "));
}

QString axis(const QVariantMap& properties, const char* name, const char* fallback = "") {
    return properties.value(QString::fromLatin1(name), QString::fromLatin1(fallback)).toString();
}

bool axisIs(const QVariantMap& properties, const char* name, const char* value) {
    return axis(properties, name).compare(QString::fromLatin1(value), Qt::CaseInsensitive) == 0;
}

QColor mix(const QColor& first, const QColor& second, qreal amount) {
    return QColor::fromRgbF(first.redF() * (1.0 - amount) + second.redF() * amount,
                            first.greenF() * (1.0 - amount) + second.greenF() * amount,
                            first.blueF() * (1.0 - amount) + second.blueF() * amount,
                            first.alphaF() * (1.0 - amount) + second.alphaF() * amount);
}

QColor semanticColor(const QWidget* widget, const QString& value) {
    const auto palette = widget->palette();
    if (value.compare(QStringLiteral("Error"), Qt::CaseInsensitive) == 0)
        return QColor(QStringLiteral("#D14343"));
    if (value.compare(QStringLiteral("Warning"), Qt::CaseInsensitive) == 0)
        return QColor(QStringLiteral("#C17D00"));
    if (value.compare(QStringLiteral("Success"), Qt::CaseInsensitive) == 0)
        return QColor(QStringLiteral("#2F7D4A"));
    if (value.compare(QStringLiteral("Info"), Qt::CaseInsensitive) == 0)
        return QColor(QStringLiteral("#3567C9"));
    return palette.color(QPalette::Highlight);
}

void configureRuntimeIdentity(QWidget* widget, const QString& name) {
    const QString sdkClass = QStringLiteral("commonWidget::Z_") + name;
    widget->setProperty("designSystemClass", sdkClass);
    widget->setProperty("designComponent", QStringLiteral("ComponentLab/") + name);
    widget->setProperty("runtimeImplementation", true);
    widget->setProperty("figmaVariantContract", true);
    widget->setAccessibleName(name);
}

bool isInteractiveCanvasComponent(const QString& name) {
    return name == QStringLiteral("Navbar") || name == QStringLiteral("Sidebar") ||
           name == QStringLiteral("Empty") || name == QStringLiteral("IntegrationCard") ||
           name == QStringLiteral("Collapse") || name == QStringLiteral("Drawer") ||
           name == QStringLiteral("Result");
}

void clearLayout(QLayout* layout) {
    while (auto* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

QString compositeAssetPath(const QString& source) {
    return source.startsWith(QStringLiteral(":"))
               ? source
               : QStringLiteral(":/figma/composite/") + source;
}

QIcon tintedCompositeIcon(const QString& source, const QColor& color) {
    QPixmap pixels(16, 16);
    pixels.fill(Qt::transparent);
    QPainter painter(&pixels);
    QSvgRenderer svg(compositeAssetPath(source));
    svg.render(&painter, QRectF(0, 0, 16, 16));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixels.rect(), color);
    return QIcon(pixels);
}

struct CompositeTokens {
    QJsonObject colors;
    QJsonObject layout;

    explicit CompositeTokens(const QString& profile) {
        const auto document = QJsonDocument::fromJson(
            token_resources::read(QStringLiteral(":/tokens/tokens_qt.json")));
        const auto selected =
            document.object()[QStringLiteral("profiles")].toObject()[profile].toObject();
        if (selected.isEmpty())
            throw std::invalid_argument("Unknown composite component profile");
        colors = selected[QStringLiteral("color")].toObject();
        layout = selected[QStringLiteral("layout")].toObject();
    }

    QColor color(const char* name) const {
        const QString value = colors[QString::fromLatin1(name)].toString();
        if (value.isEmpty())
            throw std::runtime_error("Missing composite component color token");
        if (value.startsWith(QStringLiteral("rgba("))) {
            const auto parts = value.mid(value.indexOf('(') + 1,
                                         value.lastIndexOf(')') - value.indexOf('(') - 1)
                                   .split(',');
            if (parts.size() == 4) {
                QColor result(parts[0].trimmed().toInt(), parts[1].trimmed().toInt(),
                              parts[2].trimmed().toInt());
                result.setAlphaF(parts[3].trimmed().toDouble());
                return result;
            }
        }
        const QColor result(value);
        if (!result.isValid())
            throw std::runtime_error("Invalid composite component color token");
        return result;
    }

    int metric(const char* name) const {
        const auto value = layout[QString::fromLatin1(name)];
        if (!value.isDouble())
            throw std::runtime_error("Missing composite component layout token");
        return value.toInt();
    }
};

QString qssColor(const QColor& value) {
    return value.alpha() == 255
               ? value.name(QColor::HexRgb)
               : QStringLiteral("rgba(%1,%2,%3,%4)")
                     .arg(value.red()).arg(value.green()).arg(value.blue()).arg(value.alpha());
}

QString profileForDensity(const QString& profile, const QString& density) {
    const QString appearance = profile.startsWith(QStringLiteral("dark-"))
                                   ? QStringLiteral("dark")
                                   : QStringLiteral("light");
    return appearance + (density == QStringLiteral("Compact")
                             ? QStringLiteral("-compact")
                             : QStringLiteral("-comfortable"));
}

QString compositeCjkFontFamily() {
    static const QString family = [] {
        const int id = QFontDatabase::addApplicationFont(
            QStringLiteral(":/figma/fonts/NotoSansSC-Regular.otf"));
        return QFontDatabase::applicationFontFamilies(id).value(
            0, QStringLiteral("Noto Sans SC"));
    }();
    return family;
}

QIcon initialsIcon(const QString& initials, const QColor& background,
                   const QColor& foreground) {
    QPixmap pixels(32, 32);
    pixels.fill(Qt::transparent);
    QPainter painter(&pixels);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawEllipse(pixels.rect());
    QFont font = painter.font();
    font.setPixelSize(11);
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);
    painter.setPen(foreground);
    painter.drawText(pixels.rect(), Qt::AlignCenter, initials);
    return QIcon(pixels);
}

void drawCaption(QPainter& painter, const QRectF& rect, const QString& text,
                 const QColor& color, int size = 12, bool strong = false) {
    QFont font = painter.font();
    font.setPixelSize(size);
    font.setWeight(strong ? QFont::DemiBold : QFont::Normal);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawSkeletonLine(QPainter& painter, const QRectF& rect, const QColor& color) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(rect, rect.height() / 2.0, rect.height() / 2.0);
}

void applyButtonTuple(QAbstractButton* button, QVariantMap& properties, QString& tuple,
                      const QString& incoming) {
    tuple = incoming.trimmed();
    properties = parseVariantTuple(tuple);
    button->setProperty("figmaVariant", tuple);
    button->setProperty("figmaVariantProperties", properties);
}

} // namespace

Z_DesignComponent::Z_DesignComponent(const QString& componentName, QWidget* parent)
    : QWidget(parent), m_componentName(componentName) {
    configureRuntimeIdentity(this, componentName);
    const bool interactive = isInteractiveCanvasComponent(componentName);
    setFocusPolicy(interactive ? Qt::StrongFocus : Qt::NoFocus);
    if (interactive)
        FocusIndicator::instance().watch(this);
    setAttribute(Qt::WA_Hover, true);
    m_title = componentName;
    m_description = tr("Design-system component");
    m_valueText = QStringLiteral("98.6%");
    m_items = QStringList{tr("Overview"), tr("Equipment"), tr("Recipes"), tr("Reports")};
    m_values = {34, 58, 42, 76, 64, 82, 71};
    if (componentName == QStringLiteral("Spin") || componentName == QStringLiteral("Skeleton")) {
        m_animation = new QTimer(this);
        m_animation->setInterval(60);
        connect(m_animation, &QTimer::timeout, this, [this] {
            m_animationPhase = (m_animationPhase + 1) % 120;
            update();
        });
        m_animation->start();
    }
}

QString Z_DesignComponent::componentName() const { return m_componentName; }
QString Z_DesignComponent::variantTuple() const { return m_variantTuple; }
QVariantMap Z_DesignComponent::variantProperties() const { return m_variantProperties; }
QString Z_DesignComponent::variantValue(const QString& name) const {
    return m_variantProperties.value(name).toString();
}
QString Z_DesignComponent::title() const { return m_title; }
QString Z_DesignComponent::description() const { return m_description; }
QString Z_DesignComponent::valueText() const { return m_valueText; }
QStringList Z_DesignComponent::items() const { return m_items; }
int Z_DesignComponent::currentIndex() const { return m_currentIndex; }
int Z_DesignComponent::expandedIndex() const { return m_expandedIndex; }
bool Z_DesignComponent::isOpen() const { return m_open; }
QVector<qreal> Z_DesignComponent::values() const { return m_values; }
QPixmap Z_DesignComponent::image() const { return m_image; }

QSize Z_DesignComponent::sizeHint() const {
    if (m_componentName == QStringLiteral("Navbar")) return QSize(720, 64);
    if (m_componentName == QStringLiteral("Sidebar")) return QSize(240, 360);
    if (m_componentName == QStringLiteral("Divider"))
        return axisIs(m_variantProperties, "Orientation", "Vertical") ? QSize(32, 180)
                                                                        : QSize(480, 32);
    if (m_componentName == QStringLiteral("Message")) return QSize(320, 64);
    if (m_componentName == QStringLiteral("Spin")) return QSize(180, 140);
    return QSize(360, 220);
}

QSize Z_DesignComponent::minimumSizeHint() const {
    const QSize hint = sizeHint();
    return QSize(qMin(120, hint.width()), qMin(48, hint.height()));
}

void Z_DesignComponent::setVariantTuple(const QString& tuple) {
    const QString normalized = tuple.trimmed();
    if (m_variantTuple == normalized)
        return;
    m_variantTuple = normalized;
    m_variantProperties = parseVariantTuple(normalized);
    setProperty("figmaVariant", m_variantTuple);
    setProperty("figmaVariantProperties", m_variantProperties);
    updateRuntimeState();
    emit variantChanged(m_variantTuple);
    updateGeometry();
    update();
}

void Z_DesignComponent::setVariantProperty(const QString& name, const QString& value) {
    if (name.trimmed().isEmpty() || m_variantProperties.value(name).toString() == value)
        return;
    m_variantProperties.insert(name.trimmed(), value);
    m_variantTuple = tupleFromMap(m_variantProperties);
    setProperty("figmaVariant", m_variantTuple);
    setProperty("figmaVariantProperties", m_variantProperties);
    updateRuntimeState();
    emit variantChanged(m_variantTuple);
    updateGeometry();
    update();
}

void Z_DesignComponent::setTitle(const QString& value) {
    if (m_title == value) return;
    m_title = value; emit contentChanged(); update();
}
void Z_DesignComponent::setDescription(const QString& value) {
    if (m_description == value) return;
    m_description = value; emit contentChanged(); update();
}
void Z_DesignComponent::setValueText(const QString& value) {
    if (m_valueText == value) return;
    m_valueText = value; emit contentChanged(); update();
}
void Z_DesignComponent::setItems(const QStringList& value) {
    if (m_items == value) return;
    m_items = value; m_currentIndex = qBound(0, m_currentIndex, qMax(0, m_items.size() - 1));
    emit contentChanged(); update();
}
void Z_DesignComponent::setCurrentIndex(int value) {
    const int bounded = m_items.isEmpty() ? -1 : qBound(0, value, m_items.size() - 1);
    if (m_currentIndex == bounded) return;
    m_currentIndex = bounded; emit currentIndexChanged(bounded); update();
}
void Z_DesignComponent::setExpandedIndex(int value) {
    const int bounded = qMax(-1, value);
    if (m_expandedIndex == bounded) return;
    m_expandedIndex = bounded; emit expandedIndexChanged(bounded); update();
}
void Z_DesignComponent::setOpen(bool value) {
    if (m_open == value) return;
    m_open = value; emit openChanged(value); update();
}
void Z_DesignComponent::setValues(const QVector<qreal>& value) {
    if (m_values == value) return;
    m_values = value; emit contentChanged(); update();
}
void Z_DesignComponent::setImage(const QPixmap& value) {
    m_image = value; emit contentChanged(); update();
}

void Z_DesignComponent::updateRuntimeState() {
    const QString state = axis(m_variantProperties, "State");
    setEnabled(state.compare(QStringLiteral("Disabled"), Qt::CaseInsensitive) != 0);
    if (state.compare(QStringLiteral("Focus"), Qt::CaseInsensitive) == 0 ||
        state.compare(QStringLiteral("Focus-visible"), Qt::CaseInsensitive) == 0)
        setFocus(Qt::OtherFocusReason);
    if (m_animation) {
        const bool shouldAnimate = !axisIs(m_variantProperties, "Motion", "Still") &&
                                   !axisIs(m_variantProperties, "State", "Disabled");
        shouldAnimate ? m_animation->start() : m_animation->stop();
    }
}

void Z_DesignComponent::mousePressEvent(QMouseEvent* event) {
    if (!isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    if ((m_componentName == QStringLiteral("Navbar") ||
         m_componentName == QStringLiteral("Sidebar")) && !m_items.isEmpty()) {
        const bool horizontal = m_componentName == QStringLiteral("Navbar");
        const int extent = horizontal ? width() : height();
        const int coordinate = horizontal ? event->pos().x() : event->pos().y();
        const int index = qBound(0, coordinate * m_items.size() / qMax(1, extent),
                                 m_items.size() - 1);
        setCurrentIndex(index);
        emit activated(index);
        event->accept();
        return;
    }
    if (m_componentName == QStringLiteral("Collapse")) {
        const int index = qBound(0, event->pos().y() * 3 / qMax(1, height()), 2);
        const bool accordion = axisIs(m_variantProperties, "Mode", "Accordion");
        setExpandedIndex(m_expandedIndex == index && !accordion ? -1 : index);
        emit activated(index);
        event->accept();
        return;
    }
    if (m_componentName == QStringLiteral("IntegrationCard") ||
        m_componentName == QStringLiteral("Empty") ||
        m_componentName == QStringLiteral("Result")) {
        emit actionTriggered();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Z_DesignComponent::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto colors = palette();
    const QColor ink = isEnabled() ? colors.color(QPalette::Text)
                                   : colors.color(QPalette::Disabled, QPalette::Text);
    // Composite canvases are QWidget surfaces, not text-entry viewports. Using Window keeps
    // disabled/focus previews in the active light/dark profile instead of inheriting a style's
    // disabled Base brush (which can be opaque black on Windows/Fusion).
    const QColor activeInk = colors.color(QPalette::Active, QPalette::Text);
    QColor surface = colors.color(QPalette::Window);
    if (qAbs(surface.lightnessF() - activeInk.lightnessF()) < .25)
        surface = activeInk.lightnessF() > .55 ? QColor(QStringLiteral("#202124"))
                                               : QColor(QStringLiteral("#FFFFFF"));
    const QColor panel = colors.color(QPalette::AlternateBase);
    const QColor border = mix(ink, surface, 0.76);
    const QColor accent = semanticColor(this, axis(m_variantProperties, "Type"));
    const QString state = axis(m_variantProperties, "State");
    const auto& focusIndicator = FocusIndicator::instance();
    const bool focus = focusIndicator.enabledFor(this) &&
                       (focusIndicator.visibleFor(this) ||
                        state.compare(QStringLiteral("Focus"), Qt::CaseInsensitive) == 0 ||
                        state.compare(QStringLiteral("Focus-visible"), Qt::CaseInsensitive) == 0);
    const QRectF outer = rect().adjusted(1.5, 1.5, -1.5, -1.5);

    if (m_componentName == QStringLiteral("Divider")) {
        const bool vertical = axisIs(m_variantProperties, "Orientation", "Vertical");
        const QColor line = axisIs(m_variantProperties, "Tone", "Muted") ? mix(border, surface, .35) : border;
        painter.setPen(QPen(line, 1));
        if (vertical)
            painter.drawLine(QPointF(outer.center().x(), outer.top()), QPointF(outer.center().x(), outer.bottom()));
        else
            painter.drawLine(QPointF(outer.left(), outer.center().y()), QPointF(outer.right(), outer.center().y()));
        const QString placement = axis(m_variantProperties, "Placement");
        if (!placement.isEmpty() && placement != QStringLiteral("None") && !m_title.isEmpty()) {
            const QRectF label = vertical ? QRectF(outer.left(), outer.center().y() - 14, outer.width(), 28)
                                          : QRectF(placement == QStringLiteral("Start") ? outer.left() + 12
                                                   : placement == QStringLiteral("End") ? outer.right() - 112
                                                                                         : outer.center().x() - 50,
                                                   outer.top(), 100, outer.height());
            painter.fillRect(label.adjusted(-4, 0, 4, 0), surface);
            painter.setPen(ink);
            painter.drawText(label, Qt::AlignCenter, m_title);
        }
        return;
    }

    if (m_componentName != QStringLiteral("Spin") &&
        !(m_componentName == QStringLiteral("Navbar") &&
          axisIs(m_variantProperties, "Density", "Compact"))) {
        painter.setPen(QPen(focus ? colors.color(QPalette::Highlight) : border, focus ? 2 : 1));
        painter.setBrush(surface);
        painter.drawRoundedRect(outer, 8, 8);
    }

    if (m_componentName == QStringLiteral("Navbar") || m_componentName == QStringLiteral("Sidebar")) {
        const bool horizontal = m_componentName == QStringLiteral("Navbar");
        const bool collapsed = axisIs(m_variantProperties, "Collapsed", "True");
        const int count = qMax(1, m_items.size());
        for (int i = 0; i < count; ++i) {
            QRectF item;
            if (horizontal)
                item = QRectF(i * width() / qreal(count), 0, width() / qreal(count), height());
            else
                item = QRectF(0, i * height() / qreal(count), width(), height() / qreal(count));
            if (i == m_currentIndex) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(mix(colors.color(QPalette::Highlight), surface, .82));
                painter.drawRoundedRect(item.adjusted(5, 5, -5, -5), 6, 6);
                painter.setPen(colors.color(QPalette::Highlight));
                if (horizontal)
                    painter.drawLine(item.bottomLeft() + QPointF(8, -2), item.bottomRight() + QPointF(-8, -2));
                else
                    painter.drawLine(item.topLeft() + QPointF(2, 8), item.bottomLeft() + QPointF(2, -8));
            }
            painter.setPen(i == m_currentIndex ? colors.color(QPalette::Highlight) : ink);
            const QString label = collapsed && !m_items.value(i).isEmpty()
                                      ? m_items.value(i).left(1).toUpper()
                                      : m_items.value(i);
            painter.drawText(item.adjusted(12, 0, -12, 0), Qt::AlignCenter, label);
        }
        return;
    }

    if (m_componentName == QStringLiteral("Chart")) {
        drawCaption(painter, QRectF(18, 12, width() - 36, 24), m_title, ink, 14, true);
        const QRectF plot(38, 54, width() - 58, height() - 78);
        painter.setPen(mix(border, surface, .4));
        for (int i = 0; i < 4; ++i)
            painter.drawLine(plot.left(), plot.top() + i * plot.height() / 3.0,
                             plot.right(), plot.top() + i * plot.height() / 3.0);
        if (axisIs(m_variantProperties, "State", "Empty") || m_values.isEmpty()) {
            painter.setPen(mix(ink, surface, .45));
            painter.drawText(plot, Qt::AlignCenter, tr("No data"));
            return;
        }
        qreal maximum = 1;
        for (qreal value : m_values) maximum = qMax(maximum, value);
        const QString kind = axis(m_variantProperties, "Kind", "Bar");
        if (kind == QStringLiteral("Line")) {
            QPainterPath path;
            for (int i = 0; i < m_values.size(); ++i) {
                const QPointF point(plot.left() + i * plot.width() / qMax(1, m_values.size() - 1),
                                    plot.bottom() - m_values[i] / maximum * plot.height());
                i ? path.lineTo(point) : path.moveTo(point);
            }
            painter.setPen(QPen(colors.color(QPalette::Highlight), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        } else if (kind == QStringLiteral("Status")) {
            const QColor statuses[]{QColor("#2F7D4A"), QColor("#C17D00"), QColor("#D14343")};
            for (int i = 0; i < qMin(3, m_values.size()); ++i) {
                painter.setPen(Qt::NoPen); painter.setBrush(statuses[i]);
                painter.drawRoundedRect(QRectF(plot.left(), plot.top() + i * plot.height() / 3.0 + 8,
                                               plot.width() * m_values[i] / maximum, 18), 4, 4);
            }
        } else {
            const qreal slot = plot.width() / qMax(1, m_values.size());
            painter.setPen(Qt::NoPen); painter.setBrush(colors.color(QPalette::Highlight));
            for (int i = 0; i < m_values.size(); ++i) {
                const qreal barHeight = m_values[i] / maximum * plot.height();
                painter.drawRoundedRect(QRectF(plot.left() + i * slot + slot * .2,
                                               plot.bottom() - barHeight, slot * .6, barHeight), 3, 3);
            }
        }
        return;
    }

    if (m_componentName == QStringLiteral("ImagePanel")) {
        const QRect target = rect().adjusted(14, 14, -14, -46);
        if (!m_image.isNull() && !axisIs(m_variantProperties, "Asset", "Missing")) {
            painter.setClipRect(target);
            painter.drawPixmap(target, m_image.scaled(target.size(), Qt::KeepAspectRatioByExpanding,
                                                       Qt::SmoothTransformation));
        } else {
            QLinearGradient gradient(target.topLeft(), target.bottomRight());
            gradient.setColorAt(0, panel); gradient.setColorAt(1, mix(panel, accent, .18));
            painter.setPen(Qt::NoPen); painter.setBrush(gradient); painter.drawRoundedRect(target, 6, 6);
            painter.setPen(QPen(axisIs(m_variantProperties, "Asset", "Missing")
                                    ? QColor("#D14343") : mix(ink, surface, .35), 2));
            painter.drawLine(target.topLeft() + QPoint(18, 18), target.bottomRight() - QPoint(18, 18));
            painter.drawLine(target.topRight() + QPoint(-18, 18), target.bottomLeft() + QPoint(18, -18));
        }
        drawCaption(painter, QRectF(16, height() - 38, width() - 32, 22), m_title, ink, 12, true);
        return;
    }

    if (m_componentName == QStringLiteral("Card") ||
        m_componentName == QStringLiteral("IntegrationCard")) {
        const bool connected = axisIs(m_variantProperties, "Connection", "True");
        const bool compact = axisIs(m_variantProperties, "Density", "Compact");
        const qreal inset = compact ? 10.0 : 18.0;
        painter.setPen(Qt::NoPen);
        painter.setBrush(axisIs(m_variantProperties, "Surface", "Soft") ? panel
                         : axisIs(m_variantProperties, "Surface", "Modal")
                               ? mix(surface, colors.color(QPalette::Highlight), .06) : surface);
        painter.drawRoundedRect(outer.adjusted(compact ? 3 : 6, compact ? 3 : 6,
                                               compact ? -3 : -6, compact ? -3 : -6), 8, 8);
        painter.setBrush(connected ? QColor("#2F7D4A") : mix(border, surface, .2));
        painter.drawEllipse(QPointF(inset + 12, compact ? 24 : 30), compact ? 6 : 8, compact ? 6 : 8);
        drawCaption(painter, QRectF(inset + 32, compact ? 10 : 16,
                                   width() - inset * 2 - 32, 28), m_title, ink,
                    compact ? 12 : 14, true);
        drawCaption(painter, QRectF(inset, compact ? 44 : 58, width() - inset * 2,
                                   compact ? 42 : 54), m_description,
                    mix(ink, surface, .32), 12);
        if (m_componentName == QStringLiteral("IntegrationCard")) {
            const QRectF button(width() - 122, height() - 48, 104, 30);
            painter.setPen(Qt::NoPen); painter.setBrush(colors.color(QPalette::Highlight));
            painter.drawRoundedRect(button, 5, 5);
            painter.setPen(colors.color(QPalette::HighlightedText));
            painter.drawText(button, Qt::AlignCenter, connected ? tr("Manage") : tr("Connect"));
        }
        return;
    }

    if (m_componentName == QStringLiteral("Collapse")) {
        const bool bordered = !axisIs(m_variantProperties, "Border", "Borderless");
        for (int i = 0; i < 3; ++i) {
            const qreal rowHeight = height() / 3.0;
            const QRectF row(10, i * rowHeight + 8, width() - 20, rowHeight - 10);
            painter.setPen(bordered ? QPen(border) : Qt::NoPen);
            // Paint every row explicitly. Transparent row brushes exposed a black backing
            // store on Windows for unexpanded light-theme rows even though the outer canvas
            // had already been filled with the Window palette brush.
            painter.setBrush(i == m_expandedIndex ? panel : surface);
            painter.drawRoundedRect(row, 5, 5);
            painter.setPen(ink);
            painter.drawText(row.adjusted(12, 0, -12, 0), Qt::AlignVCenter,
                             QStringLiteral("%1  %2").arg(i == m_expandedIndex ? QStringLiteral("⌄")
                                                                               : QStringLiteral("›"),
                                                               m_items.value(i, tr("Section %1").arg(i + 1))));
        }
        return;
    }

    if (m_componentName == QStringLiteral("Drawer")) {
        const bool overlay = axisIs(m_variantProperties, "Mode", "Overlay");
        painter.fillRect(rect(), overlay ? mix(ink, Qt::transparent, .72) : panel);
        if (!m_open) return;
        QRectF drawer = outer.adjusted(8, 8, -8, -8);
        const QString placement = axis(m_variantProperties, "Placement", "Right");
        if (placement == QStringLiteral("Left")) drawer.setWidth(drawer.width() * .55);
        else if (placement == QStringLiteral("Right")) drawer.setLeft(drawer.left() + drawer.width() * .45);
        else if (placement == QStringLiteral("Top")) drawer.setHeight(drawer.height() * .55);
        else drawer.setTop(drawer.top() + drawer.height() * .45);
        if (!overlay) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(surface);
            QRectF host = outer;
            if (placement == QStringLiteral("Left")) host.setLeft(drawer.right());
            else if (placement == QStringLiteral("Right")) host.setRight(drawer.left());
            else if (placement == QStringLiteral("Top")) host.setTop(drawer.bottom());
            else host.setBottom(drawer.top());
            painter.drawRect(host);
        }
        painter.setPen(QPen(focus ? colors.color(QPalette::Highlight) : border, focus ? 2 : 1));
        painter.setBrush(surface); painter.drawRoundedRect(drawer, 6, 6);
        drawCaption(painter, drawer.adjusted(16, 8, -16, -drawer.height() + 40), m_title, ink, 14, true);
        return;
    }

    if (m_componentName == QStringLiteral("Alert") ||
        m_componentName == QStringLiteral("Message") ||
        m_componentName == QStringLiteral("Notification")) {
        const QColor statusColor = semanticColor(this, axis(m_variantProperties, "Type", "Info"));
        const bool banner = m_componentName == QStringLiteral("Alert") &&
                            axisIs(m_variantProperties, "Appearance", "Banner");
        const QRectF content = banner ? outer : outer.adjusted(10, 10, -10, -10);
        painter.setPen(banner ? Qt::NoPen : QPen(statusColor, 1));
        painter.setBrush(mix(statusColor, surface, banner ? .80 : .88));
        painter.drawRoundedRect(content, banner ? 0 : 7, banner ? 0 : 7);
        painter.setPen(Qt::NoPen); painter.setBrush(statusColor);
        painter.drawEllipse(QPointF(content.left() + 22, content.top() + 22), 9, 9);
        drawCaption(painter, QRectF(content.left() + 42, content.top() + 8,
                                   content.width() - 54, 26), m_title, ink, 13, true);
        if (m_componentName != QStringLiteral("Message"))
            drawCaption(painter, QRectF(content.left() + 42, content.top() + 36,
                                       content.width() - 54, content.height() - 44),
                        m_description, mix(ink, surface, .3), 11);
        return;
    }

    if (m_componentName == QStringLiteral("Spin")) {
        if (axisIs(m_variantProperties, "Mode", "Overlay")) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(mix(ink, Qt::transparent, .86));
            painter.drawRoundedRect(outer, 8, 8);
        }
        const int requested = axisIs(m_variantProperties, "Size", "Large") ? 52
                              : axisIs(m_variantProperties, "Size", "Small") ? 24 : 36;
        const QRectF wheel(width() / 2.0 - requested / 2.0, height() / 2.0 - requested / 2.0,
                           requested, requested);
        painter.setPen(QPen(mix(border, surface, .3), 4)); painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(wheel);
        painter.setPen(QPen(isEnabled() ? colors.color(QPalette::Highlight) : border,
                            4, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(wheel, (m_animationPhase * 3) * 16, 120 * 16);
        return;
    }

    if (m_componentName == QStringLiteral("Skeleton")) {
        QColor skeleton = panel;
        if (axisIs(m_variantProperties, "Motion", "Shimmer")) {
            const qreal wave = (qSin(m_animationPhase / 120.0 * M_PI * 2.0) + 1.0) / 2.0;
            skeleton = mix(panel, surface, .18 + wave * .34);
        }
        const QString layout = axis(m_variantProperties, "Layout", "Text");
        if (layout == QStringLiteral("Card"))
            drawSkeletonLine(painter, QRectF(16, 16, width() - 32, height() * .48), skeleton);
        if (layout == QStringLiteral("List")) {
            painter.setPen(Qt::NoPen); painter.setBrush(skeleton);
            painter.drawEllipse(QPointF(34, 40), 18, 18);
        }
        for (int i = 0; i < 4; ++i)
            drawSkeletonLine(painter, QRectF(layout == QStringLiteral("List") ? 66 : 18,
                                             (layout == QStringLiteral("Card") ? height() * .57 : 22) + i * 27,
                                             width() - (layout == QStringLiteral("List") ? 88 : 36) - i * 22,
                                             11), skeleton);
        return;
    }

    if (m_componentName == QStringLiteral("Empty") ||
        m_componentName == QStringLiteral("Result")) {
        const QColor statusColor = semanticColor(this, axis(m_variantProperties, "Status", "Info"));
        const bool compact = m_componentName == QStringLiteral("Empty") &&
                             axisIs(m_variantProperties, "Size", "Compact");
        const bool simple = m_componentName == QStringLiteral("Empty") &&
                            axisIs(m_variantProperties, "Illustration", "Simple");
        const qreal iconRadius = compact ? 19.0 : 28.0;
        const qreal iconY = compact ? 40.0 : 55.0;
        painter.setPen(QPen(statusColor, 3)); painter.setBrush(mix(statusColor, surface, .88));
        if (simple)
            painter.drawRoundedRect(QRectF(width() / 2.0 - iconRadius, iconY - iconRadius,
                                           iconRadius * 2, iconRadius * 2), 5, 5);
        else
            painter.drawEllipse(QPointF(width() / 2.0, iconY), iconRadius, iconRadius);
        drawCaption(painter, QRectF(20, compact ? 65 : 92, width() - 40, 28), m_title,
                    ink, compact ? 13 : 15, true);
        drawCaption(painter, QRectF(20, compact ? 89 : 120, width() - 40, 42), m_description,
                    mix(ink, surface, .32), 11);
        if (axisIs(m_variantProperties, "Action", "Primary")) {
            const QRectF button(width() / 2.0 - 55, height() - 46, 110, 30);
            painter.setPen(Qt::NoPen); painter.setBrush(colors.color(QPalette::Highlight));
            painter.drawRoundedRect(button, 5, 5);
            painter.setPen(colors.color(QPalette::HighlightedText));
            painter.drawText(button, Qt::AlignCenter, tr("Continue"));
        }
        return;
    }

    drawCaption(painter, QRectF(18, 18, width() - 36, 28), m_title, ink, 15, true);
    drawCaption(painter, QRectF(18, 52, width() - 36, height() - 70), m_description,
                mix(ink, surface, .32), 12);
}

namespace {
QFont metricFont(int pixelSize, QFont::Weight weight) {
    static const bool loaded = [] {
        QFontDatabase::addApplicationFont(QStringLiteral(":/figma/fonts/Inter-Regular.otf"));
        QFontDatabase::addApplicationFont(QStringLiteral(":/figma/fonts/Inter-Medium.otf"));
        QFontDatabase::addApplicationFont(QStringLiteral(":/figma/fonts/Inter-SemiBold.otf"));
        QFontDatabase::addApplicationFont(QStringLiteral(":/figma/fonts/NotoSansSC-Bold.otf"));
        QFontDatabase::addApplicationFont(QStringLiteral(":/figma/fonts/NotoSansSC-Regular.otf"));
        return true;
    }();
    Q_UNUSED(loaded)
    QFont font;
    font.setFamilies({QStringLiteral("Inter"), QStringLiteral("Noto Sans SC")});
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

void drawMetricText(QPainter& painter, const QRectF& rect, const QString& text,
                    const QColor& color, int pixelSize, QFont::Weight weight,
                    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter) {
    painter.setFont(metricFont(pixelSize, weight));
    painter.setPen(color);
    painter.drawText(rect, alignment, text);
}

QString joinedAccessibleText(const QStringList& parts) {
    QStringList visible;
    for (const auto& part : parts) {
        if (!part.trimmed().isEmpty())
            visible.append(part.trimmed());
    }
    return visible.join(QStringLiteral(", "));
}
} // namespace

Z_MetricCard::Z_MetricCard(QWidget* parent)
    : Z_DesignComponent(QStringLiteral("MetricCard"), parent) {
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    Z_DesignComponent::setTitle(QStringLiteral("Total revenue"));
    Z_DesignComponent::setValueText(QString::fromUtf8("¥128,430"));
    Z_DesignComponent::setDescription(QStringLiteral("+12.8% vs last month"));
    connect(this, &Z_DesignComponent::variantChanged, this,
            [this] { refreshAccessibility(); });
    setVariantTuple(QStringLiteral("State=Default"));
    refreshAccessibility();
}

QString Z_MetricCard::state() const { return variantValue(QStringLiteral("State")); }
QString Z_MetricCard::titleText() const { return title(); }
QString Z_MetricCard::detailText() const { return description(); }
QSize Z_MetricCard::sizeHint() const { return QSize(360, 176); }
QSize Z_MetricCard::minimumSizeHint() const { return QSize(240, 176); }

void Z_MetricCard::setState(const QString& value) {
    if (!QStringList{QStringLiteral("Default"), QStringLiteral("Loading"),
                     QStringLiteral("Empty"), QStringLiteral("Error")}.contains(value))
        throw std::invalid_argument("MetricCard State requires Default, Loading, Empty or Error");
    if (state() == value)
        return;
    setVariantProperty(QStringLiteral("State"), value);
    emit stateChanged(value);
}

void Z_MetricCard::setTitleText(const QString& value) {
    Z_DesignComponent::setTitle(value);
    refreshAccessibility();
}

void Z_MetricCard::setValueText(const QString& value) {
    Z_DesignComponent::setValueText(value);
    refreshAccessibility();
}

void Z_MetricCard::setDetailText(const QString& value) {
    Z_DesignComponent::setDescription(value);
    refreshAccessibility();
}

void Z_MetricCard::refreshAccessibility() {
    setAccessibleName(title());
    const QString current = state();
    if (current == QStringLiteral("Loading"))
        setAccessibleDescription(tr("Loading"));
    else if (current == QStringLiteral("Empty"))
        setAccessibleDescription(tr("No data"));
    else if (current == QStringLiteral("Error"))
        setAccessibleDescription(tr("Data loading failed. Retry from the owning view."));
    else
        setAccessibleDescription(joinedAccessibleText({valueText(), description()}));
}

void Z_MetricCard::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto colors = palette();
    const QColor surface = colors.color(QPalette::Window);
    const QColor ink = colors.color(QPalette::Text);
    const QColor meta = colors.color(QPalette::Disabled, QPalette::Text);
    const QColor skeleton = colors.color(QPalette::AlternateBase);
    painter.setPen(Qt::NoPen);
    painter.setBrush(surface);
    painter.drawRoundedRect(rect(), 32, 32);
    drawMetricText(painter, QRectF(24, 24, width() - 48, 22), title(), ink, 14,
                   QFont::Medium);
    const QString current = state();
    if (current == QStringLiteral("Loading")) {
        drawSkeletonLine(painter, QRectF(24, 54, 152, 32), skeleton);
        drawSkeletonLine(painter, QRectF(24, 94, 196, 14), skeleton);
    } else if (current == QStringLiteral("Empty") || current == QStringLiteral("Error")) {
        drawMetricText(painter, QRectF(24, 54, width() - 48, 40),
                       current == QStringLiteral("Empty") ? QStringLiteral("—")
                                                            : QStringLiteral("--"),
                       ink, 40, QFont::Bold);
        drawMetricText(painter, QRectF(24, 102, width() - 48, 20),
                       current == QStringLiteral("Empty") ? QString::fromUtf8("暂无数据")
                                                            : QString::fromUtf8("数据加载失败，请重试"),
                       meta, 12, QFont::Normal);
    } else {
        drawMetricText(painter, QRectF(24, 54, width() - 48, 40), valueText(), ink, 40,
                       QFont::Bold);
        drawMetricText(painter, QRectF(24, 102, width() - 48, 20), description(), meta, 12,
                       QFont::Normal);
    }
}

Z_Statistic::Z_Statistic(QWidget* parent)
    : Z_DesignComponent(QStringLiteral("Statistic"), parent),
      m_prefix(QStringLiteral("$")), m_suffix(QStringLiteral("users")),
      m_localeName(QStringLiteral("en_US")) {
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    Z_DesignComponent::setTitle(QStringLiteral("Active users"));
    Z_DesignComponent::setValueText(QStringLiteral("93,241"));
    connect(this, &Z_DesignComponent::variantChanged, this,
            [this] { refreshAccessibility(); });
    setVariantTuple(QStringLiteral("Size=Default, State=Default"));
    refreshAccessibility();
}

QString Z_Statistic::size() const { return variantValue(QStringLiteral("Size")); }
QString Z_Statistic::state() const { return variantValue(QStringLiteral("State")); }
QString Z_Statistic::titleText() const { return title(); }
bool Z_Statistic::showTitle() const { return m_showTitle; }
QString Z_Statistic::prefix() const { return m_prefix; }
QString Z_Statistic::suffix() const { return m_suffix; }
bool Z_Statistic::showPrefix() const { return m_showPrefix; }
bool Z_Statistic::showSuffix() const { return m_showSuffix; }
double Z_Statistic::numericValue() const { return m_numericValue; }
bool Z_Statistic::hasNumericValue() const { return m_hasNumericValue; }
QString Z_Statistic::localeName() const { return m_localeName; }
int Z_Statistic::precision() const { return m_precision; }
bool Z_Statistic::useGrouping() const { return m_useGrouping; }
QSize Z_Statistic::sizeHint() const { return QSize(300, 150); }
QSize Z_Statistic::minimumSizeHint() const { return QSize(180, 96); }

void Z_Statistic::setSize(const QString& value) {
    if (!QStringList{QStringLiteral("Compact"), QStringLiteral("Default")}.contains(value))
        throw std::invalid_argument("Statistic Size requires Compact or Default");
    if (size() == value)
        return;
    setVariantProperty(QStringLiteral("Size"), value);
    emit sizeChanged(value);
}

void Z_Statistic::setState(const QString& value) {
    if (!QStringList{QStringLiteral("Default"), QStringLiteral("Loading"),
                     QStringLiteral("Empty"), QStringLiteral("Error")}.contains(value))
        throw std::invalid_argument("Statistic State requires Default, Loading, Empty or Error");
    if (state() == value)
        return;
    setVariantProperty(QStringLiteral("State"), value);
    emit stateChanged(value);
}

void Z_Statistic::setTitleText(const QString& value) {
    Z_DesignComponent::setTitle(value);
    refreshAccessibility();
}

void Z_Statistic::setValueText(const QString& value) {
    const bool hadNumericValue = m_hasNumericValue;
    m_hasNumericValue = false;
    Z_DesignComponent::setValueText(value);
    if (hadNumericValue)
        emit numericValueChanged();
    refreshAccessibility();
}

void Z_Statistic::setPrefix(const QString& value) {
    if (m_prefix == value) return;
    m_prefix = value; emit prefixChanged(value); emit contentChanged(); update();
    refreshAccessibility();
}

void Z_Statistic::setSuffix(const QString& value) {
    if (m_suffix == value) return;
    m_suffix = value; emit suffixChanged(value); emit contentChanged(); update();
    refreshAccessibility();
}

void Z_Statistic::setShowTitle(bool value) {
    if (m_showTitle == value) return;
    m_showTitle = value; emit showTitleChanged(value); update(); refreshAccessibility();
}

void Z_Statistic::setShowPrefix(bool value) {
    if (m_showPrefix == value) return;
    m_showPrefix = value; emit showPrefixChanged(value); update(); refreshAccessibility();
}

void Z_Statistic::setShowSuffix(bool value) {
    if (m_showSuffix == value) return;
    m_showSuffix = value; emit showSuffixChanged(value); update(); refreshAccessibility();
}

void Z_Statistic::setNumericValue(double value) {
    if (!qIsFinite(value))
        throw std::invalid_argument("Statistic numericValue must be finite");
    if (m_hasNumericValue && qFuzzyCompare(m_numericValue + 1.0, value + 1.0))
        return;
    m_numericValue = value;
    m_hasNumericValue = true;
    refreshFormattedValue();
    emit numericValueChanged();
}

void Z_Statistic::clearNumericValue() {
    if (!m_hasNumericValue)
        return;
    m_hasNumericValue = false;
    emit numericValueChanged();
    refreshAccessibility();
}

void Z_Statistic::setLocaleName(const QString& value) {
    const QString normalized = value.trimmed();
    if (normalized.isEmpty())
        throw std::invalid_argument("Statistic localeName must not be empty");
    if (m_localeName == normalized)
        return;
    m_localeName = normalized;
    if (m_hasNumericValue) refreshFormattedValue();
    emit formattingChanged();
}

void Z_Statistic::setPrecision(int value) {
    if (value < 0 || value > 12)
        throw std::invalid_argument("Statistic precision must be between 0 and 12");
    if (m_precision == value)
        return;
    m_precision = value;
    if (m_hasNumericValue) refreshFormattedValue();
    emit formattingChanged();
}

void Z_Statistic::setUseGrouping(bool value) {
    if (m_useGrouping == value)
        return;
    m_useGrouping = value;
    if (m_hasNumericValue) refreshFormattedValue();
    emit formattingChanged();
}

void Z_Statistic::refreshFormattedValue() {
    if (!m_hasNumericValue)
        return;
    QLocale locale(m_localeName);
    auto options = locale.numberOptions();
    if (m_useGrouping)
        options &= ~QLocale::OmitGroupSeparator;
    else
        options |= QLocale::OmitGroupSeparator;
    locale.setNumberOptions(options);
    Z_DesignComponent::setValueText(locale.toString(m_numericValue, 'f', m_precision));
    refreshAccessibility();
}

void Z_Statistic::refreshAccessibility() {
    setAccessibleName(m_showTitle ? title() : tr("Statistic"));
    const QString current = state();
    if (current == QStringLiteral("Loading"))
        setAccessibleDescription(tr("Loading"));
    else if (current == QStringLiteral("Empty"))
        setAccessibleDescription(tr("No data"));
    else if (current == QStringLiteral("Error"))
        setAccessibleDescription(tr("Unavailable"));
    else
        setAccessibleDescription(joinedAccessibleText(
            {m_showPrefix ? m_prefix : QString(), valueText(),
             m_showSuffix ? m_suffix : QString()}));
}

void Z_Statistic::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto colors = palette();
    const QColor ink = colors.color(QPalette::Text);
    const QColor affix = colors.color(QPalette::Disabled, QPalette::Text);
    const QColor skeleton = colors.color(QPalette::Midlight);
    const bool compact = size() == QStringLiteral("Compact");
    const qreal titleTop = compact ? 12.0 : 14.0;
    if (m_showTitle)
        drawMetricText(painter, QRectF(14, titleTop, width() - 28, 22), title(), ink, 14,
                       QFont::Medium);
    const QString current = state();
    if (current == QStringLiteral("Loading")) {
        drawSkeletonLine(painter, QRectF(14, compact ? 48 : 54, compact ? 110 : 150,
                                         compact ? 24 : 32), skeleton);
        drawSkeletonLine(painter, QRectF(14, compact ? 84 : 96, 48, 12), skeleton);
        return;
    }
    if (current == QStringLiteral("Empty")) {
        drawMetricText(painter, QRectF(14, compact ? 44 : 50, width() - 28,
                                       compact ? 32 : 40), QStringLiteral("—"), affix,
                       compact ? 24 : 40, QFont::Bold);
        return;
    }
    if (current == QStringLiteral("Error")) {
        drawMetricText(painter, QRectF(14, compact ? 48 : 54, width() - 28, 26),
                       QStringLiteral("Unavailable"), QColor(QStringLiteral("#E02128")), 18,
                       QFont::DemiBold);
        return;
    }

    const qreal rowTop = compact ? 42.0 : 48.0;
    qreal x = 14.0;
    if (m_showPrefix) {
        painter.setFont(metricFont(14, QFont::Normal));
        const qreal width = painter.fontMetrics().horizontalAdvance(m_prefix);
        drawMetricText(painter, QRectF(x, rowTop + 13, width, 22), m_prefix, affix, 14,
                       QFont::Normal);
        x += width + 4;
    }
    painter.setFont(metricFont(40, QFont::Bold));
    const qreal valueWidth = painter.fontMetrics().horizontalAdvance(valueText());
    drawMetricText(painter, QRectF(x, rowTop, valueWidth, 48), valueText(), ink, 40,
                   QFont::Bold);
    x += valueWidth + 4;
    if (m_showSuffix)
        drawMetricText(painter, QRectF(x, rowTop + (compact ? 16 : 13), width() - x - 14,
                                       compact ? 20 : 22), m_suffix, affix,
                       compact ? 12 : 14, QFont::Normal);
}

void configureFigmaMetricCard(Z_MetricCard* card,
                              const QMap<QString, QString>& properties) {
    if (!card)
        throw std::invalid_argument("MetricCard target is null");
    card->setState(properties.value(QStringLiteral("State")));
}

void configureFigmaStatistic(Z_Statistic* statistic,
                             const QMap<QString, QString>& properties) {
    if (!statistic)
        throw std::invalid_argument("Statistic target is null");
    const auto parseBoolean = [&](const char* name) {
        const QString value = properties.value(QString::fromLatin1(name));
        if (value != QStringLiteral("True") && value != QStringLiteral("False"))
            throw std::invalid_argument("Statistic boolean properties require True or False");
        return value == QStringLiteral("True");
    };
    statistic->setSize(properties.value(QStringLiteral("Size")));
    statistic->setState(properties.value(QStringLiteral("State")));
    statistic->setShowTitle(parseBoolean("Show title"));
    statistic->setShowPrefix(parseBoolean("Show prefix"));
    statistic->setShowSuffix(parseBoolean("Show suffix"));
}

#define Z_IMPLEMENT_CANVAS_COMPONENT(ClassName, Name, DefaultTuple) \
ClassName::ClassName(QWidget* parent) : Z_DesignComponent(QStringLiteral(Name), parent) { \
    setVariantTuple(QStringLiteral(DefaultTuple)); \
}

Z_IMPLEMENT_CANVAS_COMPONENT(Z_Chart, "Chart", "Kind=Bar, State=Data")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_ImagePanel, "ImagePanel", "Asset=Placeholder")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Empty, "Empty", "Size=Default, Illustration=Default, Action=None")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Card, "Card", "Surface=Card, Density=Comfortable")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_IntegrationCard, "IntegrationCard", "Connection=False")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Divider, "Divider", "Orientation=Horizontal, Placement=None, Tone=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Collapse, "Collapse", "Mode=Multi, Border=Bordered, State=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_OverlayDrawer, "Drawer", "Placement=Right, Mode=Overlay, State=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Alert, "Alert", "Type=Info, Appearance=Subtle")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Message, "Message", "Type=Info")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Notification, "Notification", "Type=Info, Placement=TopRight")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Spin, "Spin", "Size=Medium, Mode=Inline, State=Active")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Result, "Result", "Status=Success, Action=None")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Skeleton, "Skeleton", "Layout=Text, Motion=Still")

#undef Z_IMPLEMENT_CANVAS_COMPONENT

Z_Navbar::Z_Navbar(QWidget* parent)
    : Z_Navbar(QStringLiteral("light-comfortable"), parent) {}

Z_Navbar::Z_Navbar(const QString& profile, QWidget* parent)
    : QWidget(parent), m_profile(profile) {
    configureRuntimeIdentity(this, QStringLiteral("Navbar"));
    setObjectName(QStringLiteral("navbar"));
    setFocusPolicy(Qt::NoFocus);
    setProperty("focusIndicatorEnabled", true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_brandLogo = new QLabel(QStringLiteral("CL"), this);
    m_brandLogo->setObjectName(QStringLiteral("navbarBrandLogo"));
    m_brandLogo->setAlignment(Qt::AlignCenter);
    m_brandLogo->setAccessibleName(tr("ComponentLab brand"));
    m_brandTitle = new QLabel(QStringLiteral("ComponentLab"), this);
    m_brandTitle->setObjectName(QStringLiteral("navbarBrandTitle"));
    m_brandTitle->setTextFormat(Qt::PlainText);

    m_search = new Z_InputField(std::make_shared<field::Tokens>(profile), this);
    m_search->setObjectName(QStringLiteral("navbarSearch"));
    m_search->setKind(Z_InputField::Kind::Text);
    m_search->setLabel(QString());
    m_search->setHint(QString());
    m_search->setPlaceholder(tr("Search workspace"));
    m_search->setPrefixIconSource(QStringLiteral(":/figma/composite/search.svg"));
    m_search->setAccessibleName(tr("Search workspace"));
    m_search->setFixedWidth(320);

    m_notification = new QToolButton(this);
    m_notification->setObjectName(QStringLiteral("navbarNotification"));
    m_notification->setAccessibleName(tr("Notifications"));
    m_notification->setToolTip(tr("Notifications"));
    m_notification->setFocusPolicy(Qt::StrongFocus);
    FocusIndicator::instance().watch(m_notification);

    m_profileAction = new QToolButton(this);
    m_profileAction->setObjectName(QStringLiteral("navbarProfile"));
    m_profileAction->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_profileAction->setAccessibleName(tr("Open profile menu"));
    m_profileAction->setFocusPolicy(Qt::StrongFocus);
    FocusIndicator::instance().watch(m_profileAction);

    m_layout->addWidget(m_brandLogo);
    m_layout->addWidget(m_brandTitle);
    m_layout->addWidget(m_search);
    m_layout->addStretch(1);
    m_layout->addWidget(m_notification);
    m_layout->addWidget(m_profileAction);

    connect(m_search, &Z_InputField::valueChanged, this, &Z_Navbar::searchTextChanged);
    connect(m_notification, &QToolButton::clicked, this, &Z_Navbar::notificationRequested);
    connect(m_profileAction, &QToolButton::clicked, this, &Z_Navbar::profileRequested);
    setVariantTuple(QStringLiteral("Density=Comfortable"));
    setProfileName(m_profileName);
    refreshGeometryAndStyle();
}

QString Z_Navbar::variantTuple() const { return m_variantTuple; }
QVariantMap Z_Navbar::variantProperties() const { return m_variantProperties; }
QString Z_Navbar::density() const { return m_density; }
QString Z_Navbar::profileName() const { return m_profileName; }
QString Z_Navbar::searchText() const { return m_search->value(); }
bool Z_Navbar::focusIndicatorEnabled() const {
    return property("focusIndicatorEnabled").toBool();
}
Z_InputField* Z_Navbar::searchInput() const { return m_search; }
QToolButton* Z_Navbar::notificationButton() const { return m_notification; }
QToolButton* Z_Navbar::profileButton() const { return m_profileAction; }
QSize Z_Navbar::sizeHint() const {
    return QSize(1200, m_density == QStringLiteral("Compact") ? 56 : 63);
}
QSize Z_Navbar::minimumSizeHint() const {
    return QSize(720, m_density == QStringLiteral("Compact") ? 56 : 63);
}

void Z_Navbar::setVariantTuple(const QString& tuple) {
    const auto properties = parseVariantTuple(tuple);
    setDensity(axis(properties, "Density", "Comfortable"));
}

void Z_Navbar::setVariantProperty(const QString& name, const QString& value) {
    if (name.compare(QStringLiteral("Density"), Qt::CaseInsensitive) != 0)
        throw std::invalid_argument("Navbar supports only the Density variant axis");
    setDensity(value);
}

void Z_Navbar::setDensity(const QString& value) {
    if (value != QStringLiteral("Comfortable") && value != QStringLiteral("Compact"))
        throw std::invalid_argument("Navbar density must be Comfortable or Compact");
    const QString tuple = QStringLiteral("Density=") + value;
    if (m_density == value && m_variantTuple == tuple)
        return;
    const bool changed = m_density != value;
    m_density = value;
    m_variantTuple = tuple;
    m_variantProperties = {{QStringLiteral("Density"), value}};
    setProperty("figmaVariant", m_variantTuple);
    setProperty("figmaVariantProperties", m_variantProperties);
    refreshGeometryAndStyle();
    if (changed)
        emit densityChanged(value);
    emit variantChanged(m_variantTuple);
}

void Z_Navbar::setProfileName(const QString& value) {
    if (m_profileName == value && m_profileAction && !m_profileAction->text().isEmpty())
        return;
    m_profileName = value;
    const QStringList words = value.split(' ', Qt::SkipEmptyParts);
    QString initials;
    for (const auto& word : words)
        initials += word.left(1).toUpper();
    if (initials.isEmpty()) initials = QStringLiteral("U");
    if (initials.size() > 2) initials = initials.left(2);
    const CompositeTokens tokens(profileForDensity(m_profile, m_density));
    m_profileAction->setText(value + QStringLiteral("  \u2304"));
    m_profileAction->setIcon(initialsIcon(
        initials, tokens.color("base.stoken.color.interactive.brand.default"),
        tokens.color("base.stoken.color.text.invert")));
    m_profileAction->setIconSize(QSize(32, 32));
    m_profileAction->setAccessibleDescription(value);
    emit profileNameChanged(value);
}

void Z_Navbar::setSearchText(const QString& value) {
    m_search->setValue(value);
}

void Z_Navbar::setFocusIndicatorEnabled(bool enabled) {
    if (focusIndicatorEnabled() == enabled)
        return;
    FocusIndicator::instance().setEnabledFor(this, enabled);
    emit focusIndicatorEnabledChanged(enabled);
}

void Z_Navbar::refreshGeometryAndStyle() {
    if (!m_layout)
        return;
    const CompositeTokens tokens(profileForDensity(m_profile, m_density));
    const bool compact = m_density == QStringLiteral("Compact");
    const int height = tokens.metric("component.composite.navbar.height");
    const int padding = tokens.metric("component.composite.navbar.paddingInline");
    const int gap = tokens.metric("component.composite.navbar.gap");
    const QColor background = tokens.color("component.composite.navbar.background");
    const QColor border = tokens.color("component.composite.navbar.border");
    const QColor title = tokens.color("component.composite.navbar.title");
    const QColor meta = tokens.color("component.composite.navbar.meta");
    const QColor accent = tokens.color("base.stoken.color.interactive.brand.default");
    const QColor onBrand = tokens.color("base.stoken.color.text.invert");
    m_layout->setContentsMargins(padding, 0, padding, 0);
    m_layout->setSpacing(gap);
    setFixedHeight(height);
    m_search->setControlSize(compact ? QStringLiteral("md") : QStringLiteral("lg"));
    m_brandLogo->setFixedSize(32, 32);
    m_brandTitle->setMinimumWidth(112);
    m_notification->setFixedSize(compact ? QSize(32, 32) : QSize(42, 42));
    m_notification->setIconSize(compact ? QSize(32, 32) : QSize(42, 42));
    m_notification->setIcon(QIcon(compositeAssetPath(
        compact ? QStringLiteral("notifications-compact.svg")
                : QStringLiteral("notifications.svg"))));
    m_profileAction->setFixedHeight(compact ? 32 : 42);
    auto rootPalette = palette();
    rootPalette.setColor(QPalette::Window, background);
    setPalette(rootPalette);
    setAutoFillBackground(true);
    setProperty("navbarBorderColor", border);
    m_brandLogo->setStyleSheet(QStringLiteral(
        "QLabel{background:%1;color:%2;border-radius:10px;font-size:12px;font-weight:600;}")
                                   .arg(qssColor(accent), qssColor(onBrand)));
    auto titlePalette = m_brandTitle->palette();
    titlePalette.setColor(QPalette::WindowText, title);
    m_brandTitle->setPalette(titlePalette);
    QFont titleFont = m_brandTitle->font();
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Medium);
    m_brandTitle->setFont(titleFont);
    const QString actionStyle = QStringLiteral(
        "QToolButton{background:transparent;color:%1;border:1px solid transparent;border-radius:4px;padding:0;}"
        "QToolButton:hover{background:%2;}"
        "QToolButton[focusIndicatorVisible=\"true\"]{border:2px solid %3;}")
                                    .arg(qssColor(meta),
                                         qssColor(tokens.color("base.stoken.color.background.hover")),
                                         qssColor(accent));
    m_notification->setStyleSheet(actionStyle);
    m_profileAction->setStyleSheet(actionStyle + QStringLiteral(
        "QToolButton{font-size:14px;text-align:left;padding-right:4px;}"));
    updateGeometry();
}

Z_Sidebar::Z_Sidebar(QWidget* parent)
    : Z_Sidebar(QStringLiteral("light-comfortable"), parent) {}

Z_Sidebar::Z_Sidebar(const QString& profile, QWidget* parent)
    : QWidget(parent), m_profile(profile) {
    configureRuntimeIdentity(this, QStringLiteral("Sidebar"));
    setObjectName(QStringLiteral("sidebar"));
    setFocusPolicy(Qt::NoFocus);
    setProperty("focusIndicatorEnabled", true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(10);
    auto* header = new QWidget(this);
    header->setObjectName(QStringLiteral("sidebarHeader"));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(10);
    m_brandLogo = new QLabel(QStringLiteral("CL"), header);
    m_brandLogo->setObjectName(QStringLiteral("sidebarBrandLogo"));
    m_brandLogo->setAlignment(Qt::AlignCenter);
    m_brandLogo->setFixedSize(32, 32);
    m_brandTitle = new QLabel(QStringLiteral("ComponentLab"), header);
    m_brandTitle->setObjectName(QStringLiteral("sidebarBrandTitle"));
    m_brandTitle->setTextFormat(Qt::PlainText);
    m_collapseAction = new QToolButton(header);
    m_collapseAction->setObjectName(QStringLiteral("sidebarCollapse"));
    m_collapseAction->setAccessibleName(tr("Collapse sidebar"));
    m_collapseAction->setFocusPolicy(Qt::StrongFocus);
    m_collapseAction->setFixedSize(24, 32);
    m_collapseAction->setIconSize(QSize(16, 16));
    FocusIndicator::instance().watch(m_collapseAction);
    headerLayout->addWidget(m_brandLogo);
    headerLayout->addWidget(m_brandTitle, 1);
    headerLayout->addWidget(m_collapseAction);
    outer->addWidget(header);

    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("sidebarDivider"));
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(1);
    outer->addWidget(divider);

    m_routeHost = new QWidget(this);
    m_routeHost->setObjectName(QStringLiteral("sidebarRoutes"));
    m_routeLayout = new QVBoxLayout(m_routeHost);
    m_routeLayout->setContentsMargins(0, 0, 0, 0);
    m_routeLayout->setSpacing(4);
    outer->addWidget(m_routeHost, 1);

    m_routeIds = QStringList{QStringLiteral("dashboard"), QStringLiteral("orders"),
                             QStringLiteral("products"), QStringLiteral("integrations"),
                             QStringLiteral("chats"), QStringLiteral("settings")};
    m_routeLabels = QStringList{tr("Dashboard"), tr("Orders"), tr("Products"),
                                tr("Integrations"), tr("Chats"), tr("Settings")};
    connect(m_collapseAction, &QToolButton::clicked, this,
            [this] { emit collapsedChangeRequested(!m_collapsed); });
    setVariantTuple(QStringLiteral("Density=Comfortable, Collapsed=False"));
    rebuildRoutes();
    refreshGeometryAndStyle();
}

QString Z_Sidebar::variantTuple() const { return m_variantTuple; }
QVariantMap Z_Sidebar::variantProperties() const { return m_variantProperties; }
QString Z_Sidebar::density() const { return m_density; }
bool Z_Sidebar::isCollapsed() const { return m_collapsed; }
QString Z_Sidebar::activeRoute() const { return m_activeRoute; }
QStringList Z_Sidebar::routeIds() const { return m_routeIds; }
QStringList Z_Sidebar::routeLabels() const { return m_routeLabels; }
bool Z_Sidebar::focusIndicatorEnabled() const {
    return property("focusIndicatorEnabled").toBool();
}
QToolButton* Z_Sidebar::collapseButton() const { return m_collapseAction; }
QSize Z_Sidebar::sizeHint() const {
    return QSize(m_collapsed ? 80 : 280,
                 m_density == QStringLiteral("Compact") ? 466 : 520);
}
QSize Z_Sidebar::minimumSizeHint() const { return sizeHint(); }

void Z_Sidebar::setVariantTuple(const QString& tuple) {
    const auto properties = parseVariantTuple(tuple);
    const QString densityValue = axis(properties, "Density", "Comfortable");
    const QString collapsedValue = axis(properties, "Collapsed", "False");
    if (densityValue != QStringLiteral("Comfortable") && densityValue != QStringLiteral("Compact"))
        throw std::invalid_argument("Sidebar density must be Comfortable or Compact");
    if (collapsedValue != QStringLiteral("False") && collapsedValue != QStringLiteral("True"))
        throw std::invalid_argument("Sidebar Collapsed must be False or True");
    const bool collapsed = collapsedValue == QStringLiteral("True");
    const QString normalized = QStringLiteral("Density=%1, Collapsed=%2")
                                   .arg(densityValue, collapsed ? QStringLiteral("True")
                                                                : QStringLiteral("False"));
    if (m_variantTuple == normalized)
        return;
    const bool densityWasChanged = m_density != densityValue;
    const bool collapsedWasChanged = m_collapsed != collapsed;
    m_density = densityValue;
    m_collapsed = collapsed;
    m_variantTuple = normalized;
    m_variantProperties = {{QStringLiteral("Density"), m_density},
                           {QStringLiteral("Collapsed"), m_collapsed ? QStringLiteral("True")
                                                                      : QStringLiteral("False")}};
    setProperty("figmaVariant", m_variantTuple);
    setProperty("figmaVariantProperties", m_variantProperties);
    rebuildRoutes();
    refreshGeometryAndStyle();
    if (densityWasChanged) emit densityChanged(m_density);
    if (collapsedWasChanged) emit collapsedChanged(m_collapsed);
    emit variantChanged(m_variantTuple);
}

void Z_Sidebar::setVariantProperty(const QString& name, const QString& value) {
    if (name.compare(QStringLiteral("Density"), Qt::CaseInsensitive) == 0) {
        setDensity(value);
        return;
    }
    if (name.compare(QStringLiteral("Collapsed"), Qt::CaseInsensitive) == 0) {
        if (value != QStringLiteral("True") && value != QStringLiteral("False"))
            throw std::invalid_argument("Sidebar Collapsed must be False or True");
        setCollapsed(value == QStringLiteral("True"));
        return;
    }
    throw std::invalid_argument("Sidebar supports only Density and Collapsed variant axes");
}

void Z_Sidebar::setDensity(const QString& value) {
    setVariantTuple(QStringLiteral("Density=%1, Collapsed=%2")
                        .arg(value, m_collapsed ? QStringLiteral("True")
                                               : QStringLiteral("False")));
}

void Z_Sidebar::setCollapsed(bool value) {
    setVariantTuple(QStringLiteral("Density=%1, Collapsed=%2")
                        .arg(m_density, value ? QStringLiteral("True")
                                             : QStringLiteral("False")));
}

void Z_Sidebar::setActiveRoute(const QString& value) {
    if (!m_routeIds.contains(value))
        throw std::invalid_argument("Sidebar activeRoute must match a routeIds entry");
    if (m_activeRoute == value)
        return;
    m_activeRoute = value;
    updateSelection();
    emit activeRouteChanged(value);
}

void Z_Sidebar::setRouteIds(const QStringList& values) {
    if (values.isEmpty() || QSet<QString>(values.begin(), values.end()).size() != values.size())
        throw std::invalid_argument("Sidebar routeIds must be non-empty and unique");
    if (m_routeIds == values)
        return;
    m_routeIds = values;
    if (m_routeLabels.size() != values.size())
        m_routeLabels = values;
    if (!m_routeIds.contains(m_activeRoute))
        m_activeRoute = m_routeIds.first();
    rebuildRoutes();
    emit routesChanged();
    emit activeRouteChanged(m_activeRoute);
}

void Z_Sidebar::setRouteLabels(const QStringList& values) {
    if (values.size() != m_routeIds.size())
        throw std::invalid_argument("Sidebar routeLabels must match routeIds length");
    if (m_routeLabels == values)
        return;
    m_routeLabels = values;
    rebuildRoutes();
    emit routesChanged();
}

void Z_Sidebar::setFocusIndicatorEnabled(bool enabled) {
    if (focusIndicatorEnabled() == enabled)
        return;
    FocusIndicator::instance().setEnabledFor(this, enabled);
    emit focusIndicatorEnabledChanged(enabled);
}

void Z_Sidebar::rebuildRoutes() {
    if (!m_routeLayout)
        return;
    delete m_routeGroup;
    m_routeGroup = new QButtonGroup(this);
    m_routeGroup->setExclusive(true);
    clearLayout(m_routeLayout);
    m_routeButtons.clear();
    for (int index = 0; index < m_routeIds.size(); ++index) {
        if (index == m_routeIds.size() - 1)
            m_routeLayout->addStretch(1);
        auto* button = new QToolButton(m_routeHost);
        button->setObjectName(QStringLiteral("sidebarRoute%1").arg(index));
        button->setProperty("sidebarRoute", true);
        button->setText(m_routeLabels.value(index, m_routeIds[index]));
        button->setAccessibleName(button->text());
        button->setToolTip(m_collapsed ? button->text() : QString());
        button->setCheckable(true);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setToolButtonStyle(m_collapsed ? Qt::ToolButtonIconOnly
                                               : Qt::ToolButtonTextBesideIcon);
        FocusIndicator::instance().watch(button);
        m_routeGroup->addButton(button, index);
        m_routeButtons.append(button);
        m_routeLayout->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, index] {
            updateSelection();
            emit routeRequested(m_routeIds[index]);
        });
    }
    updateSelection();
}

void Z_Sidebar::refreshGeometryAndStyle() {
    if (!m_collapseAction)
        return;
    const CompositeTokens tokens(profileForDensity(m_profile, m_density));
    const bool compact = m_density == QStringLiteral("Compact");
    const int itemHeight = tokens.metric("component.composite.sidebar.navItemHeight");
    const int width = m_collapsed ? 80 : tokens.metric("component.composite.sidebar.width");
    const int radius = tokens.metric("component.composite.sidebar.radius");
    const int itemRadius = tokens.metric("component.composite.sidebar.navItemRadius");
    const QColor background = tokens.color("component.composite.sidebar.background");
    const QColor text = tokens.color("component.composite.sidebar.navItemText");
    const QColor selectedText = tokens.color("component.composite.sidebar.navItemActiveText");
    const QColor selectedBackground = tokens.color("component.composite.sidebar.navItemActiveBg");
    const QColor border = tokens.color("base.stoken.color.border.secondary");
    const QColor accent = tokens.color("base.stoken.color.interactive.brand.default");
    const QColor onBrand = tokens.color("base.stoken.color.text.invert");
    setFixedWidth(width);
    setMinimumHeight(compact ? 466 : 520);
    if (auto* outer = qobject_cast<QVBoxLayout*>(layout()))
        outer->setContentsMargins(16, 16, m_collapsed ? 8 : 16, 16);
    if (auto* headerLayout = qobject_cast<QHBoxLayout*>(m_brandLogo->parentWidget()->layout()))
        headerLayout->setSpacing(m_collapsed ? 8 : 10);
    m_brandTitle->setVisible(!m_collapsed);
    m_collapseAction->setFixedWidth(m_collapsed ? 16 : 24);
    m_collapseAction->setAccessibleName(m_collapsed ? tr("Expand sidebar")
                                                     : tr("Collapse sidebar"));
    m_collapseAction->setIcon(tintedCompositeIcon(
        m_collapsed ? QStringLiteral("chevron-right.svg")
                    : QStringLiteral("chevron-left.svg"), text));
    for (auto* button : m_routeButtons) {
        button->setFixedSize(m_collapsed ? QSize(48, itemHeight) : QSize(248, itemHeight));
        button->setToolButtonStyle(m_collapsed ? Qt::ToolButtonIconOnly
                                               : Qt::ToolButtonTextBesideIcon);
        button->setToolTip(m_collapsed ? button->text() : QString());
    }
    setStyleSheet(QStringLiteral(
        "QWidget#sidebar{background:%1;border:0;border-radius:%2px;}"
        "QLabel#sidebarBrandLogo{background:%3;color:%4;border-radius:10px;font-size:12px;font-weight:600;}"
        "QLabel#sidebarBrandTitle{color:%5;font-size:14px;font-weight:500;}"
        "QFrame#sidebarDivider{background:%6;border:0;}"
        "QToolButton#sidebarCollapse{background:transparent;color:%5;border:1px solid transparent;border-radius:4px;}"
        "QToolButton#sidebarCollapse:hover{background:%7;}"
        "QToolButton[sidebarRoute=\"true\"]{background:transparent;color:%5;border:1px solid transparent;border-radius:%8px;text-align:left;padding:0 12px;font-size:14px;}"
        "QToolButton[sidebarRoute=\"true\"]:hover,QToolButton[sidebarRoute=\"true\"]:checked{background:%7;color:%9;}"
        "QToolButton#sidebarCollapse[focusIndicatorVisible=\"true\"],QToolButton[sidebarRoute=\"true\"][focusIndicatorVisible=\"true\"]{border:2px solid %3;}"
        "QToolButton[sidebarRoute=\"true\"]:disabled{color:%10;}")
        .arg(qssColor(background)).arg(radius).arg(qssColor(accent)).arg(qssColor(onBrand))
        .arg(qssColor(text)).arg(qssColor(border)).arg(qssColor(selectedBackground))
        .arg(itemRadius).arg(qssColor(selectedText))
        .arg(qssColor(tokens.color("base.stoken.color.text.disabled"))));
    updateSelection();
    updateGeometry();
}

void Z_Sidebar::updateSelection() {
    if (!m_routeGroup)
        return;
    const CompositeTokens tokens(profileForDensity(m_profile, m_density));
    const QColor normal = tokens.color("component.composite.sidebar.navItemText");
    const QColor selected = tokens.color("component.composite.sidebar.navItemActiveText");
    const QStringList icons{QStringLiteral("home.svg"), QStringLiteral("document.svg"),
                            QStringLiteral("document.svg"), QStringLiteral("document.svg"),
                            QStringLiteral("document.svg"), QStringLiteral("settings.svg")};
    for (int index = 0; index < m_routeButtons.size(); ++index) {
        const bool active = m_routeIds.value(index) == m_activeRoute;
        auto* button = m_routeButtons[index];
        const QSignalBlocker blocker(button);
        button->setChecked(active);
        button->setIcon(tintedCompositeIcon(icons.value(index, QStringLiteral("document.svg")),
                                            active ? selected : normal));
        button->setIconSize(QSize(16, 16));
    }
}

Z_DesktopBottomNavigation::Z_DesktopBottomNavigation(QWidget* parent)
    : Z_DesktopBottomNavigation(QStringLiteral("light-comfortable"), parent) {}

Z_DesktopBottomNavigation::Z_DesktopBottomNavigation(const QString& profile, QWidget* parent)
    : QWidget(parent), m_profile(profile) {
    static const int interFont =
        QFontDatabase::addApplicationFont(QStringLiteral(":/figma/fonts/Inter-Regular.otf"));
    Q_UNUSED(interFont)
    configureRuntimeIdentity(this, QStringLiteral("DesktopBottomNavigation"));
    setObjectName(QStringLiteral("desktopBottomNavigation"));
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    setProperty("focusIndicatorEnabled", true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(9, 9, 9, 9);
    root->setSpacing(8);
    m_secondaryHost = new QWidget(this);
    m_secondaryHost->setObjectName(QStringLiteral("desktopBottomNavigationSecondaryTier"));
    m_secondaryHost->setFixedHeight(48);
    m_secondaryLayout = new QHBoxLayout(m_secondaryHost);
    m_secondaryLayout->setContentsMargins(0, 8, 0, 8);
    m_secondaryLayout->setSpacing(8);
    m_primaryHost = new QWidget(this);
    m_primaryHost->setObjectName(QStringLiteral("desktopBottomNavigationPrimaryTier"));
    m_primaryLayout = new QHBoxLayout(m_primaryHost);
    m_primaryLayout->setContentsMargins(0, 0, 0, 0);
    m_primaryLayout->setSpacing(0);
    root->addWidget(m_secondaryHost);
    root->addWidget(m_primaryHost);

    m_primaryItems = QStringList{tr("首页"), tr("内容"), tr("设置")};
    m_primaryIconSources = QStringList{QStringLiteral("home.svg"),
                                       QStringLiteral("document.svg"),
                                       QStringLiteral("settings.svg")};
    m_secondaryItems = QStringList{tr("概览"), tr("最近"), tr("收藏")};
    setVariantTuple(QStringLiteral("IconPlacement=Top"));
    refreshStyle();
    rebuildSecondaryTier();
    rebuildPrimaryTier();
}

QString Z_DesktopBottomNavigation::variantTuple() const { return m_variantTuple; }
QVariantMap Z_DesktopBottomNavigation::variantProperties() const { return m_variantProperties; }
QStringList Z_DesktopBottomNavigation::primaryItems() const { return m_primaryItems; }
QStringList Z_DesktopBottomNavigation::primaryIconSources() const { return m_primaryIconSources; }
QStringList Z_DesktopBottomNavigation::secondaryItems() const { return m_secondaryItems; }
int Z_DesktopBottomNavigation::currentIndex() const { return m_currentIndex; }
int Z_DesktopBottomNavigation::secondaryIndex() const { return m_secondaryIndex; }
QString Z_DesktopBottomNavigation::iconPlacement() const {
    return axis(m_variantProperties, "IconPlacement", "Top");
}
bool Z_DesktopBottomNavigation::focusIndicatorEnabled() const {
    return property("focusIndicatorEnabled").toBool();
}
QSize Z_DesktopBottomNavigation::sizeHint() const {
    return axisIs(m_variantProperties, "IconPlacement", "Start") ? QSize(840, 122)
                                                                    : QSize(840, 138);
}
QSize Z_DesktopBottomNavigation::minimumSizeHint() const {
    return axisIs(m_variantProperties, "IconPlacement", "Start") ? QSize(360, 122)
                                                                    : QSize(360, 138);
}

void Z_DesktopBottomNavigation::setVariantTuple(const QString& tuple) {
    const QString normalized = tuple.trimmed();
    const QVariantMap properties = parseVariantTuple(normalized);
    const QString placement = axis(properties, "IconPlacement", "Top");
    if (placement != QStringLiteral("Top") && placement != QStringLiteral("Start"))
        return;
    if (m_variantTuple == normalized)
        return;
    m_variantTuple = normalized;
    m_variantProperties = properties;
    setProperty("figmaVariant", m_variantTuple);
    setProperty("figmaVariantProperties", m_variantProperties);
    updatePlacement();
    emit variantChanged(m_variantTuple);
    updateGeometry();
}
void Z_DesktopBottomNavigation::setVariantProperty(const QString& name, const QString& value) {
    if (name.trimmed().isEmpty() || m_variantProperties.value(name).toString() == value)
        return;
    m_variantProperties.insert(name.trimmed(), value);
    setVariantTuple(tupleFromMap(m_variantProperties));
}
void Z_DesktopBottomNavigation::setPrimaryItems(const QStringList& items) {
    if (m_primaryItems == items)
        return;
    m_primaryItems = items;
    m_currentIndex = items.isEmpty() ? -1 : qBound(0, m_currentIndex, items.size() - 1);
    rebuildPrimaryTier();
    emit itemsChanged();
    emit currentIndexChanged(m_currentIndex);
}
void Z_DesktopBottomNavigation::setPrimaryIconSources(const QStringList& sources) {
    if (m_primaryIconSources == sources)
        return;
    m_primaryIconSources = sources;
    rebuildPrimaryTier();
    emit itemsChanged();
}
void Z_DesktopBottomNavigation::setSecondaryItems(const QStringList& items) {
    if (m_secondaryItems == items)
        return;
    m_secondaryItems = items;
    m_secondaryIndex = items.isEmpty() ? -1 : qBound(0, m_secondaryIndex, items.size() - 1);
    rebuildSecondaryTier();
    emit itemsChanged();
    emit secondaryIndexChanged(m_secondaryIndex);
}
void Z_DesktopBottomNavigation::setCurrentIndex(int index) {
    const int bounded = m_primaryItems.isEmpty() ? -1 : qBound(0, index, m_primaryItems.size() - 1);
    if (m_currentIndex == bounded)
        return;
    m_currentIndex = bounded;
    updateSelection();
    emit currentIndexChanged(bounded);
}
void Z_DesktopBottomNavigation::setSecondaryIndex(int index) {
    const int bounded = m_secondaryItems.isEmpty() ? -1 : qBound(0, index, m_secondaryItems.size() - 1);
    if (m_secondaryIndex == bounded)
        return;
    m_secondaryIndex = bounded;
    updateSelection();
    emit secondaryIndexChanged(bounded);
}
void Z_DesktopBottomNavigation::setIconPlacement(const QString& value) {
    if (value != QStringLiteral("Top") && value != QStringLiteral("Start"))
        throw std::invalid_argument("DesktopBottomNavigation IconPlacement must be Top or Start");
    setVariantTuple(QStringLiteral("IconPlacement=") + value);
}
void Z_DesktopBottomNavigation::setFocusIndicatorEnabled(bool enabled) {
    if (focusIndicatorEnabled() == enabled)
        return;
    FocusIndicator::instance().setEnabledFor(this, enabled);
    emit focusIndicatorEnabledChanged(enabled);
}

void Z_DesktopBottomNavigation::rebuildSecondaryTier() {
    clearLayout(m_secondaryLayout);
    delete m_secondaryGroup;
    m_secondaryGroup = new QButtonGroup(this);
    m_secondaryGroup->setExclusive(true);
    m_secondaryButtons.clear();
    m_secondaryLayout->addStretch(1);
    for (int i = 0; i < m_secondaryItems.size(); ++i) {
        auto* button = new QPushButton(m_secondaryItems[i], m_secondaryHost);
        button->setObjectName(QStringLiteral("desktopBottomNavigationSubItem"));
        button->setAccessibleName(m_secondaryItems[i]);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::StrongFocus);
        QFont buttonFont(compositeCjkFontFamily());
        buttonFont.setPixelSize(14);
        buttonFont.setWeight(QFont::DemiBold);
        button->setFont(buttonFont);
        button->setFixedSize(55, 32);
        FocusIndicator::instance().watch(button);
        m_secondaryGroup->addButton(button, i);
        m_secondaryButtons.append(button);
        m_secondaryLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, i] {
            setSecondaryIndex(i);
            emit secondaryActivated(i);
        });
        if (i + 1 < m_secondaryItems.size()) {
            auto* separator = new QFrame(m_secondaryHost);
            separator->setObjectName(QStringLiteral("desktopBottomNavigationSeparator"));
            separator->setFrameShape(QFrame::VLine);
            separator->setFrameShadow(QFrame::Plain);
            separator->setLineWidth(1);
            separator->setFixedSize(1, 24);
            separator->setFocusPolicy(Qt::NoFocus);
            m_secondaryLayout->addWidget(separator);
        }
    }
    m_secondaryLayout->addStretch(1);
    updateSelection();
}

void Z_DesktopBottomNavigation::rebuildPrimaryTier() {
    clearLayout(m_primaryLayout);
    delete m_primaryGroup;
    m_primaryGroup = new QButtonGroup(this);
    m_primaryGroup->setExclusive(true);
    m_primaryButtons.clear();
    for (int i = 0; i < m_primaryItems.size(); ++i) {
        auto* button = new QToolButton(m_primaryHost);
        button->setObjectName(QStringLiteral("desktopBottomNavigationMainItem"));
        button->setText(m_primaryItems[i]);
        button->setAccessibleName(m_primaryItems[i]);
        button->setIconSize(QSize(16, 16));
        button->setCheckable(true);
        button->setFocusPolicy(Qt::StrongFocus);
        QFont buttonFont(compositeCjkFontFamily());
        buttonFont.setPixelSize(14);
        buttonFont.setWeight(QFont::DemiBold);
        button->setFont(buttonFont);
        FocusIndicator::instance().watch(button);
        m_primaryGroup->addButton(button, i);
        m_primaryButtons.append(button);
        m_primaryLayout->addWidget(button);
        if (i + 1 < m_primaryItems.size())
            m_primaryLayout->addStretch(1);
        connect(button, &QToolButton::clicked, this, [this, i] {
            setCurrentIndex(i);
            emit primaryActivated(i);
        });
    }
    updatePlacement();
    updateSelection();
}

void Z_DesktopBottomNavigation::updatePlacement() {
    if (!m_primaryHost)
        return;
    const bool start = axisIs(m_variantProperties, "IconPlacement", "Start");
    m_primaryHost->setFixedHeight(start ? 48 : 64);
    for (auto* button : m_primaryButtons) {
        button->setToolButtonStyle(start ? Qt::ToolButtonTextBesideIcon
                                         : Qt::ToolButtonTextUnderIcon);
        button->setFixedSize(start ? QSize(71, 48) : QSize(120, 64));
    }
    setFixedHeight(start ? 122 : 138);
}

void Z_DesktopBottomNavigation::updateSelection() {
    const CompositeTokens tokens(m_profile);
    const QColor normal = tokens.color("base.stoken.color.text.primary");
    const QColor selected = tokens.color("base.stoken.color.interactive.brand.default");
    for (int i = 0; i < m_secondaryButtons.size(); ++i)
        m_secondaryButtons[i]->setChecked(i == m_secondaryIndex);
    for (int i = 0; i < m_primaryButtons.size(); ++i) {
        const bool active = i == m_currentIndex;
        auto* button = m_primaryButtons[i];
        button->setChecked(active);
        const QString source = m_primaryIconSources.value(i, QStringLiteral("home.svg"));
        button->setIcon(tintedCompositeIcon(source, active ? selected : normal));
    }
}

void Z_DesktopBottomNavigation::refreshStyle() {
    const CompositeTokens tokens(m_profile);
    const QColor background = tokens.color("base.stoken.color.background.control-default");
    const QColor hover = tokens.color("base.stoken.color.background.hover");
    const QColor border = tokens.color("base.stoken.color.border.secondary");
    const QColor text = tokens.color("base.stoken.color.text.primary");
    const QColor disabled = tokens.color("base.stoken.color.text.disabled");
    const QColor accent = tokens.color("base.stoken.color.interactive.brand.default");
    setStyleSheet(QStringLiteral(
        "QWidget#desktopBottomNavigation{background:%1;border:1px solid %2;}"
        "QWidget#desktopBottomNavigationSecondaryTier{background:%3;border:0;}"
        "QWidget#desktopBottomNavigationPrimaryTier{background:%1;border:0;}"
        "QPushButton#desktopBottomNavigationSubItem{background:%1;color:%4;"
        "border:1px solid %2;padding:0 4px;font-size:14px;font-weight:600;}"
        "QPushButton#desktopBottomNavigationSubItem:hover,"
        "QPushButton#desktopBottomNavigationSubItem:pressed,"
        "QPushButton#desktopBottomNavigationSubItem:checked{background:%3;color:%5;}"
        "QPushButton#desktopBottomNavigationSubItem[focusIndicatorVisible=\"true\"]{"
        "border:2px solid %5;color:%5;}"
        "QPushButton#desktopBottomNavigationSubItem:disabled{color:%6;}"
        "QFrame#desktopBottomNavigationSeparator{color:%2;background:%2;border:0;}"
        "QToolButton#desktopBottomNavigationMainItem{background:%1;color:%4;"
        "border:1px solid %2;font-size:14px;font-weight:600;}"
        "QToolButton#desktopBottomNavigationMainItem:hover,"
        "QToolButton#desktopBottomNavigationMainItem:pressed,"
        "QToolButton#desktopBottomNavigationMainItem:checked{background:%3;color:%5;}"
        "QToolButton#desktopBottomNavigationMainItem[focusIndicatorVisible=\"true\"]{"
        "border:2px solid %5;color:%5;}"
        "QToolButton#desktopBottomNavigationMainItem:disabled{color:%6;}")
                      .arg(qssColor(background), qssColor(border), qssColor(hover),
                           qssColor(text), qssColor(accent), qssColor(disabled)));
}

Z_Switch::Z_Switch(QWidget* parent) : QAbstractButton(parent) {
    configureRuntimeIdentity(this, QStringLiteral("Switch"));
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    FocusIndicator::instance().watch(this);
    // A switch is an intrinsic-size control. Layouts may position it but must
    // never stretch its track in either axis.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setVariantTuple(QStringLiteral("Size=Medium, Value=Off, State=Default"));
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        setVariantProperty(QStringLiteral("Value"), checked ? QStringLiteral("On")
                                                            : QStringLiteral("Off"));
    });
}

QString Z_Switch::variantTuple() const { return m_variantTuple; }
QVariantMap Z_Switch::variantProperties() const { return m_variantProperties; }
QSize Z_Switch::sizeHint() const {
    return axisIs(m_variantProperties, "Size", "Small") ? QSize(36, 20) : QSize(44, 24);
}
void Z_Switch::setVariantTuple(const QString& tuple) {
    applyButtonTuple(this, m_variantProperties, m_variantTuple, tuple);
    const bool on = axisIs(m_variantProperties, "Value", "On");
    if (isChecked() != on) {
        const QSignalBlocker blocker(this);
        setChecked(on);
    }
    setEnabled(!axisIs(m_variantProperties, "State", "Disabled") &&
               !axisIs(m_variantProperties, "State", "Loading"));
    setFixedSize(sizeHint());
    emit variantChanged(m_variantTuple); updateGeometry(); update();
}
void Z_Switch::setVariantProperty(const QString& name, const QString& value) {
    m_variantProperties.insert(name, value);
    setVariantTuple(tupleFromMap(m_variantProperties));
}
void Z_Switch::paintEvent(QPaintEvent*) {
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    const QColor surface = palette().color(QPalette::Base);
    const QColor accent = palette().color(QPalette::Highlight);
    const QColor disabled = palette().color(QPalette::Disabled, QPalette::Mid);
    const QString state = axis(m_variantProperties, "State");
    const bool previewFocus = axisIs(m_variantProperties, "State", "Focus-visible");
    const QRectF track = rect().adjusted(1.5, 1.5, -1.5, -1.5);
    const auto& focusIndicator = FocusIndicator::instance();
    const bool focus = focusIndicator.enabledFor(this) &&
                       (focusIndicator.visibleFor(this) || previewFocus);
    painter.setPen(QPen(focus ? accent : Qt::transparent, focus ? 2 : 0));
    QColor trackColor = isChecked() ? accent : palette().color(QPalette::Mid);
    if (state == QStringLiteral("Hover"))
        trackColor = mix(trackColor, surface, .12);
    else if (state == QStringLiteral("Pressed"))
        trackColor = mix(trackColor, palette().color(QPalette::Text), .18);
    painter.setBrush(!isEnabled() ? disabled : trackColor);
    painter.drawRoundedRect(track, track.height() / 2.0, track.height() / 2.0);
    const qreal radius = track.height() / 2.0 - 3;
    const qreal x = isChecked() ? track.right() - radius - 3 : track.left() + radius + 3;
    painter.setPen(Qt::NoPen); painter.setBrush(surface);
    painter.drawEllipse(QPointF(x, track.center().y()), radius, radius);
    if (axisIs(m_variantProperties, "State", "Loading")) {
        painter.setPen(QPen(accent, 2)); painter.setBrush(Qt::NoBrush);
        painter.drawArc(QRectF(x - radius + 3, track.center().y() - radius + 3,
                               (radius - 3) * 2, (radius - 3) * 2), 30 * 16, 250 * 16);
    }
}

Z_Chip::Z_Chip(QWidget* parent) : QAbstractButton(parent) {
    configureRuntimeIdentity(this, QStringLiteral("Chip"));
    setText(tr("Process tag"));
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    FocusIndicator::instance().watch(this);
    setVariantTuple(QStringLiteral("Orientation=Horizontal, Value=Unselected, State=Default"));
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        setVariantProperty(QStringLiteral("Value"), checked ? QStringLiteral("Selected")
                                                            : QStringLiteral("Unselected"));
    });
}
QString Z_Chip::variantTuple() const { return m_variantTuple; }
QVariantMap Z_Chip::variantProperties() const { return m_variantProperties; }
QSize Z_Chip::sizeHint() const {
    return axisIs(m_variantProperties, "Orientation", "Vertical") ? QSize(72, 64) : QSize(116, 32);
}
void Z_Chip::setVariantTuple(const QString& tuple) {
    applyButtonTuple(this, m_variantProperties, m_variantTuple, tuple);
    const bool selected = axisIs(m_variantProperties, "Value", "Selected");
    if (isChecked() != selected) {
        const QSignalBlocker blocker(this); setChecked(selected);
    }
    setEnabled(!axisIs(m_variantProperties, "State", "Disabled"));
    emit variantChanged(m_variantTuple); updateGeometry(); update();
}
void Z_Chip::setVariantProperty(const QString& name, const QString& value) {
    m_variantProperties.insert(name, value); setVariantTuple(tupleFromMap(m_variantProperties));
}
void Z_Chip::paintEvent(QPaintEvent*) {
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    const QColor surface = palette().color(QPalette::Base);
    const QColor accent = palette().color(QPalette::Highlight);
    const QColor ink = isEnabled() ? palette().color(QPalette::Text)
                                   : palette().color(QPalette::Disabled, QPalette::Text);
    const QString state = axis(m_variantProperties, "State");
    const auto& focusIndicator = FocusIndicator::instance();
    const bool focus = focusIndicator.enabledFor(this) &&
                       (focusIndicator.visibleFor(this) ||
                        state == QStringLiteral("Focus-visible"));
    QColor fill = isChecked() ? mix(accent, surface, .82) : surface;
    if (state == QStringLiteral("Hover")) fill = mix(accent, surface, .9);
    if (state == QStringLiteral("Pressed")) fill = mix(accent, surface, .75);
    painter.setPen(QPen(focus || isChecked() ? accent : mix(ink, surface, .7), focus ? 2 : 1));
    painter.setBrush(fill);
    painter.drawRoundedRect(rect().adjusted(1.5, 1.5, -1.5, -1.5), 12, 12);
    painter.setPen(isChecked() ? accent : ink);
    painter.drawText(rect().adjusted(12, 4, -12, -4), Qt::AlignCenter | Qt::TextWordWrap, text());
}

Z_DataTable::Z_DataTable(QWidget* parent) : QTableWidget(3, 3, parent) {
    configureRuntimeIdentity(this, QStringLiteral("DataTable"));
    setHorizontalHeaderLabels({tr("Equipment"), tr("State"), tr("Yield")});
    const QString rows[][3] = {{QStringLiteral("ETCH-01"), tr("Running"), QStringLiteral("98.6%")},
                               {QStringLiteral("CVD-02"), tr("Idle"), QStringLiteral("97.9%")},
                               {QStringLiteral("CMP-04"), tr("Warning"), QStringLiteral("94.2%")}};
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column)
            setItem(row, column, new QTableWidgetItem(rows[row][column]));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->hide();
    setVariantTuple(QStringLiteral("Size=Medium, State=Default"));
}
QString Z_DataTable::variantTuple() const { return m_variantTuple; }
QVariantMap Z_DataTable::variantProperties() const { return m_variantProperties; }
void Z_DataTable::setVariantTuple(const QString& tuple) {
    m_variantTuple = tuple.trimmed(); m_variantProperties = parseVariantTuple(m_variantTuple);
    setProperty("figmaVariant", m_variantTuple); setProperty("figmaVariantProperties", m_variantProperties);
    updateRuntimeState(); emit variantChanged(m_variantTuple);
}
void Z_DataTable::setVariantProperty(const QString& name, const QString& value) {
    m_variantProperties.insert(name, value); setVariantTuple(tupleFromMap(m_variantProperties));
}
void Z_DataTable::updateRuntimeState() {
    const int height = axisIs(m_variantProperties, "Size", "Small") ? 28
                       : axisIs(m_variantProperties, "Size", "Large") ? 44 : 36;
    verticalHeader()->setDefaultSectionSize(height);
    const QString state = axis(m_variantProperties, "State");
    setEnabled(state != QStringLiteral("Disabled"));
    for (int row = 0; row < rowCount(); ++row) {
        for (int column = 0; column < columnCount(); ++column) {
            if (auto* cell = item(row, column))
                cell->setBackground(QBrush());
        }
    }
    if (state.contains(QStringLiteral("Selected"), Qt::CaseInsensitive) && rowCount()) {
        selectRow(0);
        if (state == QStringLiteral("Selected-hover")) {
            const QColor selectedHover = mix(palette().color(QPalette::Highlight),
                                             palette().color(QPalette::Base), .70);
            for (int column = 0; column < columnCount(); ++column)
                item(0, column)->setBackground(selectedHover);
        }
    } else {
        clearSelection();
        if (state == QStringLiteral("Hover") && rowCount()) {
            const QColor hover = mix(palette().color(QPalette::Highlight),
                                     palette().color(QPalette::Base), .90);
            for (int column = 0; column < columnCount(); ++column)
                item(0, column)->setBackground(hover);
        }
    }
    setProperty("figmaPreviewState", state);
    viewport()->update();
}

Z_Form::Z_Form(QWidget* parent) : QWidget(parent), m_layout(new QFormLayout(this)) {
    configureRuntimeIdentity(this, QStringLiteral("Form"));
    addRow(tr("Equipment"), new QLineEdit(QStringLiteral("ETCH-01"), this));
    addRow(tr("Recipe"), new QLineEdit(QStringLiteral("OXIDE-120"), this));
    setVariantTuple(QStringLiteral("Layout=Horizontal, Density=Comfortable, State=Default"));
}
QString Z_Form::variantTuple() const { return m_variantTuple; }
QVariantMap Z_Form::variantProperties() const { return m_variantProperties; }
void Z_Form::addRow(const QString& label, QWidget* field) { m_layout->addRow(label, field); }
void Z_Form::setVariantTuple(const QString& tuple) {
    m_variantTuple = tuple.trimmed(); m_variantProperties = parseVariantTuple(m_variantTuple);
    setProperty("figmaVariant", m_variantTuple); setProperty("figmaVariantProperties", m_variantProperties);
    updateRuntimeState(); emit variantChanged(m_variantTuple);
}
void Z_Form::setVariantProperty(const QString& name, const QString& value) {
    m_variantProperties.insert(name, value); setVariantTuple(tupleFromMap(m_variantProperties));
}
void Z_Form::updateRuntimeState() {
    m_layout->setRowWrapPolicy(axisIs(m_variantProperties, "Layout", "Vertical")
                                   ? QFormLayout::WrapAllRows : QFormLayout::DontWrapRows);
    const bool compact = axisIs(m_variantProperties, "Density", "Compact");
    m_layout->setHorizontalSpacing(compact ? 8 : 16);
    m_layout->setVerticalSpacing(compact ? 6 : 12);
    const QString state = axis(m_variantProperties, "State");
    setEnabled(state != QStringLiteral("Disabled"));
    setProperty("validationState", state);
    const QString borderColor = state == QStringLiteral("Error") ? QStringLiteral("#D14343")
                                : state == QStringLiteral("Warning") ? QStringLiteral("#C17D00")
                                                                     : QString();
    const auto fields = findChildren<QLineEdit*>();
    for (auto* field : fields) {
        field->setProperty("validationState", state);
        field->setStyleSheet(borderColor.isEmpty()
                                 ? QString()
                                 : QStringLiteral("QLineEdit { border: 1px solid %1; border-radius: 4px; }")
                                       .arg(borderColor));
    }
    style()->unpolish(this); style()->polish(this); update();
}

Z_Descriptions::Z_Descriptions(QWidget* parent)
    : QWidget(parent), m_layout(new QGridLayout(this)) {
    configureRuntimeIdentity(this, QStringLiteral("Descriptions"));
    m_items = {{tr("Equipment"), QStringLiteral("ETCH-01")},
               {tr("Recipe"), QStringLiteral("OXIDE-120")},
               {tr("Owner"), tr("Process team")},
               {tr("State"), tr("Running")}};
    setVariantTuple(QStringLiteral("Layout=Horizontal, Border=Plain, Density=Comfortable"));
}
QString Z_Descriptions::variantTuple() const { return m_variantTuple; }
QVariantMap Z_Descriptions::variantProperties() const { return m_variantProperties; }
void Z_Descriptions::addItem(const QString& label, const QString& value) {
    m_items.append({label, value}); rebuild();
}
void Z_Descriptions::clearItems() { m_items.clear(); rebuild(); }
void Z_Descriptions::setVariantTuple(const QString& tuple) {
    m_variantTuple = tuple.trimmed(); m_variantProperties = parseVariantTuple(m_variantTuple);
    setProperty("figmaVariant", m_variantTuple); setProperty("figmaVariantProperties", m_variantProperties);
    rebuild(); emit variantChanged(m_variantTuple);
}
void Z_Descriptions::setVariantProperty(const QString& name, const QString& value) {
    m_variantProperties.insert(name, value); setVariantTuple(tupleFromMap(m_variantProperties));
}
void Z_Descriptions::rebuild() {
    while (auto* item = m_layout->takeAt(0)) {
        delete item->widget(); delete item;
    }
    const bool vertical = axisIs(m_variantProperties, "Layout", "Vertical");
    const bool bordered = axisIs(m_variantProperties, "Border", "Bordered");
    const bool compact = axisIs(m_variantProperties, "Density", "Compact");
    m_layout->setSpacing(compact ? 4 : 10);
    for (int index = 0; index < m_items.size(); ++index) {
        auto* label = new QLabel(m_items[index].first, this);
        auto* value = new QLabel(m_items[index].second, this);
        QFont strong = label->font(); strong.setWeight(QFont::DemiBold); label->setFont(strong);
        if (bordered) {
            label->setFrameStyle(QFrame::Box | QFrame::Plain);
            value->setFrameStyle(QFrame::Box | QFrame::Plain);
        }
        if (vertical) {
            m_layout->addWidget(label, index * 2, 0);
            m_layout->addWidget(value, index * 2 + 1, 0);
        } else {
            m_layout->addWidget(label, index, 0);
            m_layout->addWidget(value, index, 1);
        }
    }
}

} // namespace commonWidget
