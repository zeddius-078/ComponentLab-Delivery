#include "DialogButtonBoxAppearance.h"
#include "FocusIndicator.h"
#include "Z_PushButton.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QPointer>
#include <QStyle>
#include <QTimer>
#include <stdexcept>

namespace commonWidget {
namespace {
QColor color(const QJsonValue& value) {
    const auto a = value.toArray();
    return QColor(a[0].toInt(), a[1].toInt(), a[2].toInt(), qRound(255 * a[3].toDouble(1)));
}

const QJsonObject& recipes() {
    static const auto result = [] {
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
        QFile file(":/figma/button-visuals.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Captured Button recipes unavailable");
        return QJsonDocument::fromJson(file.readAll()).object()["recipes"].toObject();
    }();
    return result;
}

// Standard buttons must stay the exact QPushButton objects created by Qt so
// standardButton(), retranslation and native shortcut/default handling remain valid.
class NativeButtonPaint final : public QObject {
public:
    NativeButtonPaint(QPushButton* button, QDialogButtonBox* box)
        : QObject(button), button_(button), box_(box) {
        setObjectName("figmaDialogNativeButtonPaint");
        button_->installEventFilter(this);
    }

    void setBox(QDialogButtonBox* box) {
        box_ = box;
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() != QEvent::Paint || !box_)
            return false;
        const auto role = box_->buttonRole(button_);
        if (role == QDialogButtonBox::InvalidRole)
            return false;
        const auto family = role == QDialogButtonBox::DestructiveRole ? "DANGER"
                            : role == QDialogButtonBox::AcceptRole ||
                                    role == QDialogButtonBox::YesRole ||
                                    role == QDialogButtonBox::ApplyRole
                                ? "PRIMARY"
                                : "DEFAULT";
        const auto state = !button_->isEnabled()                            ? "disabled"
                           : button_->isDown()                              ? "active"
                           : FocusIndicator::instance().visibleFor(button_) ? "focus-visible"
                           : button_->underMouse()                          ? "hover"
                                                                            : "default";
        const auto recipe = recipes()[family].toObject()[state].toObject();
        const qreal stroke = recipe["strokeWidth"].toDouble(), radius = recipe["radius"].toDouble();
        QPainter p(button_);
        p.setRenderHint(QPainter::Antialiasing);
        p.setOpacity(recipe["opacity"].toDouble(1));
        QPainterPath path;
        path.addRoundedRect(
            QRectF(button_->rect()).adjusted(stroke / 2, stroke / 2, -stroke / 2, -stroke / 2),
            qMax(0., radius - stroke / 2), qMax(0., radius - stroke / 2));
        p.setPen(QPen(color(recipe["stroke"]), stroke));
        p.setBrush(color(recipe["fill"]));
        p.drawPath(path);
        QFont font("Inter");
        font.setPixelSize(14);
        font.setWeight(QFont::Normal);
        p.setFont(font);
        p.setPen(color(recipe["foreground"]));
        QRect textRect = button_->rect().adjusted(12, 0, -12, 0);
        if (!button_->icon().isNull()) {
            const auto size = button_->iconSize().boundedTo(QSize(24, 24));
            const int total =
                QFontMetrics(font).size(Qt::TextShowMnemonic, button_->text()).width() +
                size.width() + 4;
            QRect slot((button_->width() - total) / 2, (button_->height() - size.height()) / 2,
                       size.width(), size.height());
            slot = QStyle::visualRect(button_->layoutDirection(), button_->rect(), slot);
            button_->icon().paint(&p, slot, Qt::AlignCenter,
                                  button_->isEnabled() ? QIcon::Normal : QIcon::Disabled,
                                  button_->isChecked() ? QIcon::On : QIcon::Off);
            textRect = QRect((button_->width() - total) / 2 + size.width() + 4, 0,
                             total - size.width() - 4, button_->height());
            textRect = QStyle::visualRect(button_->layoutDirection(), button_->rect(), textRect);
        }
        const bool mnemonic =
            button_->style()->styleHint(QStyle::SH_UnderlineShortcut, nullptr, button_);
        p.drawText(textRect,
                   Qt::AlignCenter | (mnemonic ? Qt::TextShowMnemonic : Qt::TextHideMnemonic),
                   button_->text());
        button_->setProperty("figmaDialogButtonRole", int(role));
        button_->setProperty("figmaDialogButtonRecipe", family);
        return true;
    }

private:
    QPushButton* button_;
    QPointer<QDialogButtonBox> box_;
};

class BoxBinding final : public QObject {
public:
    explicit BoxBinding(QDialogButtonBox* box) : QObject(box), box_(box) {
        setObjectName("figmaDialogBoxBinding");
        box_->installEventFilter(this);
    }

    void refresh() {
        if (refreshing_)
            return;
        refreshing_ = true;
        if (auto* layout = box_->layout()) {
            layout->setContentsMargins(16, 16, 16, 16);
            layout->setSpacing(8);
        }
        for (auto* item : box_->buttons()) {
            if (qobject_cast<Z_PushButton*>(item))
                continue; // Existing canonical Button owns its rendering.
            auto* button = qobject_cast<QPushButton*>(item);
            if (!button)
                continue;
            if (auto* binding = button->findChild<QObject*>("figmaDialogNativeButtonPaint",
                                                            Qt::FindDirectChildrenOnly)) {
                static_cast<NativeButtonPaint*>(binding)->setBox(box_);
            } else {
                new NativeButtonPaint(button, box_);
                button->setStyleSheet(
                    "QPushButton {background:transparent;border:0;padding:0 "
                    "12px;min-height:32px;font-family:'Inter';font-size:14px;font-weight:400;}");
            }
            button->update();
        }
        refreshing_ = false;
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::ChildAdded || event->type() == QEvent::LayoutRequest ||
            event->type() == QEvent::Show || event->type() == QEvent::StyleChange) {
            if (!queued_) {
                queued_ = true;
                QTimer::singleShot(0, this, [this] {
                    queued_ = false;
                    refresh();
                });
            }
        }
        return false;
    }

private:
    QDialogButtonBox* box_;
    bool queued_ = false, refreshing_ = false;
};
} // namespace

void applyFigmaDialogButtonBoxAppearance(QDialogButtonBox* box) {
    if (!box)
        throw std::invalid_argument("Dialog button box is required");
    recipes();
    box->setProperty("designSystemStyleOwner", "figma-dialogactions-captured");
    const QString qss = "QDialogButtonBox {background:transparent;border:0;padding:0;}";
    if (box->styleSheet() != qss)
        box->setStyleSheet(qss);
    auto* binding = static_cast<BoxBinding*>(
        box->findChild<QObject*>("figmaDialogBoxBinding", Qt::FindDirectChildrenOnly));
    if (!binding)
        binding = new BoxBinding(box);
    binding->refresh();
}
} // namespace commonWidget
