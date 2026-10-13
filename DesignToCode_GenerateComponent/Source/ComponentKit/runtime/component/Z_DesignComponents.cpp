#include "Z_DesignComponents.h"
#include "FocusIndicator.h"

#include <QApplication>
#include <QBrush>
#include <QButtonGroup>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
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
           name == QStringLiteral("Collapse") || name == QStringLiteral("OverlayDrawer") ||
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

    if (m_componentName == QStringLiteral("MetricCard") ||
        m_componentName == QStringLiteral("Statistic")) {
        const bool compact = axisIs(m_variantProperties, "Size", "Compact");
        const qreal inset = compact ? 12.0 : 18.0;
        const int titleSize = compact ? 12 : 14;
        const int valueSize = compact ? 24 : 30;
        drawCaption(painter, QRectF(inset, compact ? 10 : 14, width() - inset * 2, 24),
                    m_title, ink, titleSize, true);
        if (state == QStringLiteral("Loading")) {
            drawSkeletonLine(painter, QRectF(inset, compact ? 45 : 58, width() * .56,
                                             compact ? 17 : 22), panel);
        } else if (state == QStringLiteral("Empty")) {
            drawCaption(painter, QRectF(inset, compact ? 40 : 54, width() - inset * 2, 36),
                        QStringLiteral("—"), ink, compact ? 22 : 26, true);
        } else if (state == QStringLiteral("Error")) {
            drawCaption(painter, QRectF(inset, compact ? 40 : 54, width() - inset * 2, 36),
                        tr("Data unavailable"), QColor("#D14343"), compact ? 12 : 14, true);
        } else {
            drawCaption(painter, QRectF(inset, compact ? 38 : 50, width() - inset * 2, 44),
                        m_valueText, ink, valueSize, true);
            drawCaption(painter, QRectF(inset, compact ? 74 : 94, width() - inset * 2, 20), m_description,
                        colors.color(QPalette::Highlight), 12);
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

    if (m_componentName == QStringLiteral("OverlayDrawer")) {
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

#define Z_IMPLEMENT_CANVAS_COMPONENT(ClassName, Name, DefaultTuple) \
ClassName::ClassName(QWidget* parent) : Z_DesignComponent(QStringLiteral(Name), parent) { \
    setVariantTuple(QStringLiteral(DefaultTuple)); \
}

Z_IMPLEMENT_CANVAS_COMPONENT(Z_Navbar, "Navbar", "Density=Comfortable")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Sidebar, "Sidebar", "Density=Comfortable, Collapsed=False")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_MetricCard, "MetricCard", "State=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Chart, "Chart", "Kind=Bar, State=Data")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_ImagePanel, "ImagePanel", "Asset=Placeholder")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Empty, "Empty", "Size=Default, Illustration=Default, Action=None")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Statistic, "Statistic", "Size=Default, State=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Card, "Card", "Surface=Card, Density=Comfortable")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_IntegrationCard, "IntegrationCard", "Connection=False")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Divider, "Divider", "Orientation=Horizontal, Placement=None, Tone=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Collapse, "Collapse", "Mode=Multi, Border=Bordered, State=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_OverlayDrawer, "OverlayDrawer", "Placement=Right, Mode=Overlay, State=Default")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Alert, "Alert", "Type=Info, Appearance=Subtle")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Message, "Message", "Type=Info")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Notification, "Notification", "Type=Info, Placement=TopRight")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Spin, "Spin", "Size=Medium, Mode=Inline, State=Active")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Result, "Result", "Status=Success, Action=None")
Z_IMPLEMENT_CANVAS_COMPONENT(Z_Skeleton, "Skeleton", "Layout=Text, Motion=Still")

#undef Z_IMPLEMENT_CANVAS_COMPONENT

Z_DesktopBottomNavigation::Z_DesktopBottomNavigation(QWidget* parent) : QWidget(parent) {
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

    setStyleSheet(QStringLiteral(
        "QWidget#desktopBottomNavigation{background:#ffffff;border:1px solid #d0d0d0;}"
        "QWidget#desktopBottomNavigationSecondaryTier{background:#f3f3f3;border:0;}"
        "QWidget#desktopBottomNavigationPrimaryTier{background:#ffffff;border:0;}"
        "QPushButton#desktopBottomNavigationSubItem{background:#ffffff;color:#191919;"
        "border:1px solid #d0d0d0;padding:0 4px;font-size:14px;font-weight:600;}"
        "QPushButton#desktopBottomNavigationSubItem:hover,"
        "QPushButton#desktopBottomNavigationSubItem:pressed,"
        "QPushButton#desktopBottomNavigationSubItem:checked{background:#f3f3f3;color:#6235f7;}"
        "QPushButton#desktopBottomNavigationSubItem[focusIndicatorVisible=\"true\"]{"
        "border:2px solid #6235f7;color:#6235f7;}"
        "QPushButton#desktopBottomNavigationSubItem:disabled{color:#8a8a8a;}"
        "QToolButton#desktopBottomNavigationMainItem{background:#ffffff;color:#191919;"
        "border:1px solid #d0d0d0;font-size:14px;font-weight:600;}"
        "QToolButton#desktopBottomNavigationMainItem:hover,"
        "QToolButton#desktopBottomNavigationMainItem:pressed,"
        "QToolButton#desktopBottomNavigationMainItem:checked{background:#f3f3f3;color:#6235f7;}"
        "QToolButton#desktopBottomNavigationMainItem[focusIndicatorVisible=\"true\"]{"
        "border:2px solid #6235f7;color:#6235f7;}"
        "QToolButton#desktopBottomNavigationMainItem:disabled{color:#8a8a8a;}"));

    m_primaryItems = QStringList{tr("首页"), tr("内容"), tr("设置")};
    m_primaryIconSources = QStringList{QStringLiteral("home.svg"),
                                       QStringLiteral("document.svg"),
                                       QStringLiteral("settings.svg")};
    m_secondaryItems = QStringList{tr("概览"), tr("最近"), tr("收藏")};
    setVariantTuple(QStringLiteral("IconPlacement=Top"));
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
            separator->setStyleSheet(QStringLiteral("color:#d0d0d0;background:#d0d0d0;border:0;"));
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
    const QColor normal(QStringLiteral("#191919"));
    const QColor selected(QStringLiteral("#6235f7"));
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

Z_Switch::Z_Switch(QWidget* parent) : QAbstractButton(parent) {
    configureRuntimeIdentity(this, QStringLiteral("Switch"));
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    FocusIndicator::instance().watch(this);
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
