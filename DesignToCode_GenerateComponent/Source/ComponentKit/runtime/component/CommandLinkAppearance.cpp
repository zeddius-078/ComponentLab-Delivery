#include "CommandLinkAppearance.h"
#include <QCommandLinkButton>
#include <QEvent>
#include <QFontDatabase>
#include <QPainter>
#include <QStyle>
#include <QtMath>
#include <QTextLayout>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QString state(const QCommandLinkButton* button) {
    if (!button->isEnabled())
        return "Disabled";
    const auto fixed = button->property("figmaCommandSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (button->isDown())
        return "Pressed";
    const QString checked = button->isChecked() ? "Checked" : "";
    if (button->hasFocus())
        return checked + "Focus";
    if (button->underMouse())
        return checked + "Hover";
    return checked.isEmpty() ? "Default" : checked;
}

QFont textFont(bool title) {
    QFont font("HarmonyOS Sans");
    font.setPixelSize(title ? 14 : 12);
    font.setWeight(title ? QFont::Medium : QFont::Normal);
    return font;
}

// Only presentation line boxes are customized. No text/value/shortcut is rewritten.
qreal paragraph(QPainter* painter, const QString& text, const QRectF& bounds,
                Qt::LayoutDirection direction) {
    if (text.isEmpty())
        return 0;
    QTextLayout layout(text, textFont(false));
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    option.setTextDirection(direction);
    option.setAlignment(direction == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft);
    layout.setTextOption(option);
    layout.beginLayout();
    qreal y = 0;
    while (true) {
        auto line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(qMax(1.0, bounds.width()));
        line.setPosition(QPointF(0, y + (18 - line.height()) / 2));
        y += 18;
    }
    layout.endLayout();
    if (painter)
        layout.draw(painter, bounds.topLeft());
    return y;
}

class CommandBinding final : public QObject {
public:
    explicit CommandBinding(QCommandLinkButton* button) : QObject(button), button_(button) {
        setObjectName("figmaCommandBinding");
        button_->installEventFilter(this);
        QObject::connect(button_, &QAbstractButton::toggled, this, [this] {
            refresh();
        });
        QObject::connect(button_, &QAbstractButton::pressed, this, [this] {
            refresh();
        });
        QObject::connect(button_, &QAbstractButton::released, this, [this] {
            refresh();
        });
    }

    bool showIcon() const {
        return button_->property("figmaCommandShowIcon").toBool() && !button_->icon().isNull();
    }

    int iconSlot() const {
        return showIcon() ? qMax(24, button_->iconSize().width()) + 12 : 0;
    }

    void refresh() {
        if (refreshing_)
            return;
        refreshing_ = true;
        if (!button_->property("figmaCommandSourceSnapshot").toBool()) {
            const int width = qMax(1, button_->width() - 36 - iconSlot());
            const bool description = button_->property("figmaCommandShowDescription").toBool() &&
                                     !button_->description().isEmpty();
            const int lines =
                description ? qCeil(paragraph(nullptr, button_->description(),
                                              QRectF(0, 0, width, 0), button_->layoutDirection()))
                            : 0;
            // Reserve the 2px focus border in live layout; never clip the host's description.
            button_->setMinimumHeight(qMax(24, 22 + (description ? 4 + lines : 0)) + 28);
        }
        lastDescription_ = button_->description();
        button_->update();
        refreshing_ = false;
    }

    void paint() {
        const auto visual = state(button_);
        const bool disabled = visual == "Disabled";
        const bool checked = visual.startsWith("Checked");
        const bool focus = visual.endsWith("Focus");
        const bool hover = visual.endsWith("Hover"), pressed = visual == "Pressed";
        const int borderWidth = focus ? 2 : 1;
        const bool transparent = button_->isFlat() && (visual == "Default" || visual == "Focus");
        const QColor background(disabled           ? "#f3f3f3"
                                : hover || pressed ? "#f1ecff"
                                : checked          ? "#f3efff"
                                : transparent      ? "#00000000"
                                                   : "#ffffff");
        const QColor border(disabled           ? "#d0d0d0"
                            : focus || checked ? "#6235f7"
                            : pressed          ? "#532cd1"
                            : transparent      ? "#00000000"
                                               : "#a7a7a7");
        const QColor title(disabled ? "#8a8a8a" : checked ? "#6235f7" : "#191919");
        QPainter painter(button_);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(background);
        painter.setPen(QPen(border, borderWidth));
        const qreal half = borderWidth / 2.0;
        painter.drawRoundedRect(QRectF(button_->rect()).adjusted(half, half, -half, -half),
                                4 - half, 4 - half);
        const bool source = button_->property("figmaCommandSourceSnapshot").toBool();
        const int inset = source ? borderWidth : 2;
        const int contentX = 16 + inset + iconSlot();
        const int contentWidth = source ? 292 : qMax(1, button_->width() - contentX - 16 - inset);
        const int contentHeight = source ? 52 : qMax(0, button_->height() - 24 - inset * 2);
        QRect content(contentX, 12 + inset, contentWidth, contentHeight);
        content = QStyle::visualRect(button_->layoutDirection(), button_->rect(), content);
        if (showIcon()) {
            const int slotHeight = qMax(24, button_->iconSize().height());
            const QRect logicalIcon(16 + inset, (button_->height() - slotHeight) / 2,
                                    button_->iconSize().width(), button_->iconSize().height());
            const auto icon =
                QStyle::visualRect(button_->layoutDirection(), button_->rect(), logicalIcon);
            button_->icon().paint(&painter, icon, Qt::AlignLeft | Qt::AlignTop,
                                  disabled ? QIcon::Disabled : QIcon::Normal,
                                  checked ? QIcon::On : QIcon::Off);
            button_->setProperty("figmaCommandIconBounds", icon);
        } else
            button_->setProperty("figmaCommandIconBounds", QRect());
        painter.setClipRect(content);
        painter.setFont(textFont(true));
        painter.setPen(title);
        const int align =
            button_->layoutDirection() == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft;
        painter.drawText(QRect(content.x(), content.y(), content.width(), 22),
                         align | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextShowMnemonic,
                         button_->text());
        if (button_->property("figmaCommandShowDescription").toBool()) {
            painter.setPen(QColor(disabled ? "#8a8a8a" : "#444444"));
            paragraph(&painter, button_->description(),
                      QRectF(content.x(), content.y() + 26, content.width(),
                             qMax(0, content.height() - 26)),
                      button_->layoutDirection());
        }
        button_->setProperty("figmaCommandVisualState", visual);
        button_->setProperty("figmaCommandContentBounds", content);
    }

protected:
    bool eventFilter(QObject*, QEvent* e) override {
        if (e->type() == QEvent::Paint) {
            if (lastDescription_ != button_->description())
                QTimer::singleShot(0, this, [this] { refresh(); });
            paint();
            return true;
        }
        switch (e->type()) {
        case QEvent::Resize:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::EnabledChange:
        case QEvent::StyleChange:
        case QEvent::LayoutRequest:
        case QEvent::Show:
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
            break;
        default:
            break;
        }
        return false;
    }

private:
    QCommandLinkButton* button_;
    QString lastDescription_;
    bool refreshing_ = false;
};
}

QIcon figmaCommandLinkIcon() {
    QIcon icon;
    for (const auto mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled})
        for (const auto value : {QIcon::Off, QIcon::On})
            icon.addFile(":/figma/commandlink/Arrow" +
                             QString(mode == QIcon::Disabled ? "Disabled"
                                     : value == QIcon::On    ? "Accent"
                                                             : "Primary") +
                             ".svg",
                         QSize(16, 16), mode, value);
    return icon;
}

void configureFigmaCommandLinkButton(QCommandLinkButton* button,
                                     const QMap<QString, QString>& properties,
                                     bool sourceSnapshot) {
    if (!button)
        throw std::invalid_argument("CommandLinkButton is required");
    const auto surface = properties.value("Surface");
    const auto visual = properties.value("State");
    const auto showDescription = properties.value("Show description");
    const auto showIcon = properties.value("Show icon");
    if (!QStringList{"Raised", "Flat"}.contains(surface) ||
        !QStringList{"Default", "Hover", "Pressed", "Focus", "Disabled", "Checked",
                     "CheckedHover", "CheckedFocus"}
             .contains(visual) ||
        !QStringList{"False", "True"}.contains(showDescription) ||
        !QStringList{"False", "True"}.contains(showIcon))
        throw std::invalid_argument("Invalid Figma CommandLinkButton variant");

    button->setFlat(surface == "Flat");
    button->setEnabled(visual != "Disabled");
    const bool checked = visual.startsWith("Checked");
    if (checked)
        button->setCheckable(true);
    button->setChecked(checked);
    button->setProperty("figmaCommandSurface", surface);
    applyFigmaCommandLinkAppearance(button, visual == "Disabled" ? QString() : visual,
                                    showDescription == "True", showIcon == "True",
                                    sourceSnapshot);
}

void applyFigmaCommandLinkAppearance(QCommandLinkButton* button, const QString& snapshotState,
                                     bool showDescription, bool showIcon, bool sourceSnapshot) {
    if (!button || !QStringList{"", "Default", "Hover", "Pressed", "Focus", "Checked",
                                "CheckedHover", "CheckedFocus"}
                        .contains(snapshotState))
        throw std::invalid_argument(
            "Captured CommandLink requires native QCommandLinkButton and source-backed state");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        return true;
    }();
    Q_UNUSED(fonts);
    button->setProperty("figmaAppearance", "figma-light");
    button->setProperty("designSystemStyleOwner", "figma-commandlink-captured");
    button->setProperty("figmaCommandSnapshotState", snapshotState);
    button->setProperty("figmaCommandShowDescription", showDescription);
    button->setProperty("figmaCommandShowIcon", showIcon);
    button->setProperty("figmaCommandSourceSnapshot", sourceSnapshot);
    auto* binding = dynamic_cast<CommandBinding*>(
        button->findChild<QObject*>("figmaCommandBinding", Qt::FindDirectChildrenOnly));
    if (!binding)
        binding = new CommandBinding(button);
    binding->refresh();
}
}
