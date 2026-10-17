#include "SpinBoxAppearance.h"
#include <QAbstractSpinBox>
#include <QApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOptionSpinBox>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QString controlsFor(const QAbstractSpinBox* spin) {
    if (spin->buttonSymbols() == QAbstractSpinBox::NoButtons)
        return "None";
    if (spin->buttonSymbols() == QAbstractSpinBox::UpDownArrows)
        return "UpDown";
    return spin->property("figmaSpinSplit").toBool() ? "SplitPlusMinus" : "PlusMinus";
}

bool focused(const QAbstractSpinBox* spin) {
    if (!spin->isEnabled() || spin->isReadOnly())
        return false;
    const auto state = spin->property("figmaSpinSnapshotState").toString();
    return state.isEmpty() ? spin->hasFocus() : state == "Focus";
}

QRectF fieldRect(const QAbstractSpinBox* spin) {
    const auto mode = controlsFor(spin);
    const int border = focused(spin) ? 2 : 1;
    QRectF rect;
    if (mode == "SplitPlusMinus")
        rect = QRectF(border + 33, 0, spin->width() - 2 * border - 66, spin->height());
    else
        rect = QRectF(border, 0, spin->width() - (mode == "None" ? 2 : 45), spin->height());
    if (spin->layoutDirection() == Qt::RightToLeft)
        rect.moveLeft(spin->width() - rect.right());
    return rect;
}

QRectF stepRect(const QAbstractSpinBox* spin, bool up, bool visual) {
    const auto mode = controlsFor(spin);
    const int border = focused(spin) ? 2 : 1;
    if (mode == "None")
        return {};
    QRectF rect;
    if (mode == "SplitPlusMinus")
        rect = QRectF(up ? spin->width() - border - 32 : border, 0, 32, spin->height());
    else {
        const qreal half = (spin->height() - 1) / 2.0;
        rect =
            QRectF(spin->width() - 45 + border, up ? 0 : (visual ? half + 1 : spin->height() / 2.0),
                   43, visual ? half : spin->height() / 2.0);
    }
    if (spin->layoutDirection() == Qt::RightToLeft)
        rect.moveLeft(spin->width() - rect.right());
    return rect;
}

QFont textFont(bool affix) {
    QFont font;
    font.setFamilies({"HarmonyOS Sans", "HarmonyOS Sans SC"});
    font.setPixelSize(affix ? 13 : 14);
    font.setWeight(affix ? QFont::Normal : QFont::Medium);
    return font;
}

class SpinStyle final : public QProxyStyle {
public:
    explicit SpinStyle(QObject* owner) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(owner);
    }

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl sub,
                         const QWidget* widget) const override {
        const auto* spin = qobject_cast<const QAbstractSpinBox*>(widget);
        if (control != CC_SpinBox || !spin)
            return QProxyStyle::subControlRect(control, option, sub, widget);
        if (sub == SC_SpinBoxUp || sub == SC_SpinBoxDown)
            return stepRect(spin, sub == SC_SpinBoxUp, false).toRect();
        if (sub == SC_SpinBoxEditField) {
            auto field = fieldRect(spin);
            field.adjust(12, 1, -10, -1);
            return field.toRect();
        }
        return spin->rect();
    }

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                                     const QPoint& point, const QWidget* widget) const override {
        const auto* spin = qobject_cast<const QAbstractSpinBox*>(widget);
        if (control != CC_SpinBox || !spin)
            return QProxyStyle::hitTestComplexControl(control, option, point, widget);
        for (const auto sub : {SC_SpinBoxUp, SC_SpinBoxDown})
            if (subControlRect(control, option, sub, widget).contains(point))
                return sub;
        return SC_SpinBoxEditField;
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* p,
                            const QWidget* widget) const override {
        const auto* spin = qobject_cast<const QAbstractSpinBox*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
        if (control != CC_SpinBox || !spin || !opt) {
            QProxyStyle::drawComplexControl(control, option, p, widget);
            return;
        }
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QRectF root(spin->rect());
        QPainterPath clip;
        clip.addRoundedRect(root, 4, 4);
        p->setClipPath(clip);
        p->fillPath(clip, QColor(!spin->isEnabled() || spin->isReadOnly() ? "#f3f3f3" : "#ffffff"));
        const auto mode = controlsFor(spin);
        for (const bool up : {true, false}) {
            const auto slot = stepRect(spin, up, true);
            if (slot.isEmpty())
                continue;
            p->fillRect(slot, QColor("#f3f3f3"));
            const bool available = spin->isEnabled() && !spin->isReadOnly() &&
                                   (opt->stepEnabled & (up ? QAbstractSpinBox::StepUpEnabled
                                                           : QAbstractSpinBox::StepDownEnabled));
            const auto glyph = (mode == "UpDown" ? (up ? QString("Up") : QString("Down"))
                                                 : (up ? QString("Plus") : QString("Minus"))) +
                               (available ? "Primary" : "Disabled");
            const QRectF asset(slot.center() - QPointF(8, 8), QSizeF(16, 16));
            QSvgRenderer svg(":/figma/spinbox/" + glyph + ".svg");
            svg.render(p, asset);
        }
        if (mode == "SplitPlusMinus") {
            const auto field = fieldRect(spin);
            p->fillRect(QRectF(field.left() - 1, 0, 1, spin->height()), QColor("#d0d0d0"));
            p->fillRect(QRectF(field.right(), 0, 1, spin->height()), QColor("#d0d0d0"));
        } else if (mode != "None") {
            const auto slot = stepRect(spin, true, true);
            p->fillRect(QRectF(slot.left(), (spin->height() - 1) / 2.0, 43, 1), QColor("#d0d0d0"));
        }
        const qreal border = focused(spin) ? 2 : 1;
        p->setPen(QPen(QColor(!spin->isEnabled() ? "#d0d0d0"
                              : focused(spin)    ? "#6235f7"
                                                 : "#a7a7a7"),
                       border));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(root.adjusted(border / 2, border / 2, -border / 2, -border / 2),
                           4 - border / 2, 4 - border / 2);
        p->restore();
    }
};

class SpinBinding final : public QObject {
public:
    explicit SpinBinding(QAbstractSpinBox* spin)
        : QObject(spin), spin_(spin), edit_(spin->findChild<QLineEdit*>()) {
        setObjectName("figmaSpinBinding");
        spin_->installEventFilter(this);
        edit_->installEventFilter(this);
        QObject::connect(edit_, &QLineEdit::textChanged, this, [this] {
            edit_->update();
            spin_->update();
        });
        QObject::connect(edit_, &QLineEdit::selectionChanged, this, [this] {
            edit_->update();
        });
    }

    void refresh() {
        // QStyleSheetStyle computes the native editor rectangle itself, even when
        // its step subcontrols delegate to the proxy. Keep both on this shared geometry.
        if (spin_->minimumHeight() != 32 || spin_->maximumHeight() != 32)
            spin_->setFixedHeight(32);
        auto field = fieldRect(spin_);
        field.adjust(12, 1, -10, -1);
        edit_->setGeometry(field.toRect());
        spin_->setAlignment(controlsFor(spin_) == "SplitPlusMinus" ? Qt::AlignCenter
                                                                   : Qt::AlignLeading);
        spin_->setProperty("figmaSpinControls", controlsFor(spin_));
        spin_->setProperty(
            "figmaSpinUpAssetBounds",
            QRectF(stepRect(spin_, true, true).center() - QPointF(8, 8), QSizeF(16, 16)));
        spin_->setProperty(
            "figmaSpinDownAssetBounds",
            QRectF(stepRect(spin_, false, true).center() - QPointF(8, 8), QSizeF(16, 16)));
        edit_->update();
        spin_->update();
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        if ((object == edit_ && event->type() == QEvent::Resize) ||
            (object == spin_ &&
             (event->type() == QEvent::Resize || event->type() == QEvent::StyleChange ||
              event->type() == QEvent::LayoutDirectionChange ||
              event->type() == QEvent::EnabledChange))) {
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
        }
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
        }
        if (object != edit_ || event->type() != QEvent::Paint)
            return false;
        // Rich affix typography is a presentation layer only. While the actual editor owns focus
        // or a selection, Qt paints its native caret, selection, scrolling and IME preedit intact.
        // No text, prefix/suffix, validator or editor replacement is used to fake those behaviours.
        if (edit_->hasFocus() || edit_->hasSelectedText())
            return false;
        QString prefix, suffix, value;
        if (auto* s = qobject_cast<QSpinBox*>(spin_)) {
            prefix = s->prefix();
            suffix = s->suffix();
            value = s->cleanText();
        } else if (auto* decimal = qobject_cast<QDoubleSpinBox*>(spin_)) {
            prefix = decimal->prefix();
            suffix = decimal->suffix();
            value = decimal->cleanText();
        }
        if (!spin_->specialValueText().isEmpty() && edit_->text() == spin_->specialValueText()) {
            prefix.clear();
            suffix.clear();
            value = edit_->text();
        }
        const auto numeric = textFont(false), affix = textFont(true);
        const qreal pw = prefix.isEmpty() ? 0
                                          : qRound(QFontMetricsF(affix).horizontalAdvance(prefix)),
                    vw = qRound(QFontMetricsF(numeric).horizontalAdvance(value)),
                    sw = suffix.isEmpty() ? 0
                                          : qRound(QFontMetricsF(affix).horizontalAdvance(suffix));
        const qreal total = pw + vw + sw + (pw > 0 ? 4 : 0) + (sw > 0 ? 4 : 0);
        qreal x = controlsFor(spin_) == "SplitPlusMinus" ? (edit_->width() - total) / 2.0 : 0;
        if (spin_->layoutDirection() == Qt::RightToLeft && controlsFor(spin_) != "SplitPlusMinus")
            x = edit_->width() - total;
        QPainter painter(edit_);
        painter.setRenderHint(QPainter::TextAntialiasing);
        const auto draw = [&](const QString& text, const QFont& font, qreal width,
                              const QColor& color) {
            painter.setFont(font);
            painter.setPen(color);
            painter.drawText(QRectF(x, 0, width, edit_->height()),
                             Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, text);
            x += width + 4;
        };
        const QColor affixColor(spin_->isEnabled() ? "#444444" : "#8a8a8a"),
            valueColor(spin_->isEnabled() ? "#191919" : "#8a8a8a");
        if (pw > 0)
            draw(prefix, affix, pw, affixColor);
        draw(value, numeric, vw, valueColor);
        if (sw > 0)
            draw(suffix, affix, sw, affixColor);
        return true;
    }

private:
    QAbstractSpinBox* spin_;
    QLineEdit* edit_;
};
}

void applyFigmaSpinBoxAppearance(QAbstractSpinBox* spin, const QString& controls,
                                 const QString& snapshotState) {
    if ((!qobject_cast<QSpinBox*>(spin) && !qobject_cast<QDoubleSpinBox*>(spin)) ||
        !QStringList{"UpDown", "PlusMinus", "SplitPlusMinus", "None"}.contains(controls) ||
        !QStringList{"", "Default", "Focus"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured SpinBox requires a native numeric spin box and source-backed controls/state");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_SC_Regular.ttf");
        return true;
    }();
    Q_UNUSED(fonts);
    spin->setProperty("figmaAppearance", "figma-light");
    spin->setProperty("figmaSpinSplit", controls == "SplitPlusMinus");
    spin->setProperty("figmaSpinSnapshotState", snapshotState);
    auto* binding = dynamic_cast<SpinBinding*>(
        spin->findChild<QObject*>("figmaSpinBinding", Qt::FindDirectChildrenOnly));
    if (!binding) {
        spin->setStyle(new SpinStyle(spin));
        spin->setStyleSheet(
            "QAbstractSpinBox {font-family:'HarmonyOS Sans','HarmonyOS Sans SC';font-size:14px;min-height:0;}"
            "QLineEdit {background:transparent;border:0;padding:0;min-height:0;"
            "font-family:'HarmonyOS Sans','HarmonyOS Sans SC';font-size:14px;color:#191919;"
            "selection-background-color:#6235f7;selection-color:white;} QLineEdit:disabled "
            "{color:#8a8a8a;}");
        spin->ensurePolished();
        auto* edit = spin->findChild<QLineEdit*>();
        edit->setFont(textFont(false));
        binding = new SpinBinding(spin);
    }
    spin->setButtonSymbols(controls == "None"     ? QAbstractSpinBox::NoButtons
                           : controls == "UpDown" ? QAbstractSpinBox::UpDownArrows
                                                  : QAbstractSpinBox::PlusMinus);
    spin->setFixedHeight(32);
    binding->refresh();
}
}
