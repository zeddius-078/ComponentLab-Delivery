#include "ComboBoxAppearance.h"
#include "../surface/Surface.h"
#include <QComboBox>
#include <QFontComboBox>
#include <QEvent>
#include <QFontDatabase>
#include <QLineEdit>
#include <QPainter>
#include <QPointer>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionComboBox>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QString state(const QComboBox* combo) {
    if (!combo->isEnabled())
        return "Disabled";
    const auto fixed = combo->property("figmaComboSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (combo->hasFocus() || (combo->lineEdit() && combo->lineEdit()->hasFocus()))
        return "Focus";
    return combo->underMouse() ? "Hover" : "Default";
}

int padding(const QComboBox* combo) {
    return combo->property("figmaComboSize").toString() == "Small" ? 8 : 12;
}

QColor foreground(bool enabled, bool placeholder) {
    if (enabled)
        return QColor(placeholder ? "#676767" : "#191919");
    QColor disabled("#8a8a8a");
    disabled.setAlphaF(.7);
    return disabled;
}

class ComboStyle final : public QProxyStyle {
public:
    explicit ComboStyle(QComboBox* combo) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(combo);
    }

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc,
                         const QWidget* widget) const override {
        const auto* combo = qobject_cast<const QComboBox*>(widget);
        if (cc == CC_ComboBox && combo) {
            const int pad = padding(combo), w = opt->rect.width(), h = opt->rect.height();
            QRect logical;
            if (sc == SC_ComboBoxArrow)
                logical = QRect(w - pad - 20, 1, pad + 19, h - 2);
            else if (sc == SC_ComboBoxEditField) {
                // Native label adds 1px; frameless QLineEdit adds its own 2px.
                const int inset = pad - (combo->isEditable() ? 2 : 1);
                logical = QRect(inset, 1, qMax(0, w - pad - 24 - inset), h - 2);
            } else if (sc == SC_ComboBoxFrame)
                logical = QRect(0, 0, w, h);
            else
                return QProxyStyle::subControlRect(cc, opt, sc, widget);
            logical.translate(opt->rect.topLeft());
            return visualRect(opt->direction, opt->rect, logical);
        }
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt, QPainter* p,
                            const QWidget* widget) const override {
        const auto* combo = qobject_cast<const QComboBox*>(widget);
        if (cc != CC_ComboBox || !combo) {
            QProxyStyle::drawComplexControl(cc, opt, p, widget);
            return;
        }
        const auto visual = state(combo);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const QColor border(visual == "Focus"   ? "#6235f7"
                            : visual == "Hover" ? "#191919"
                                                : "#d9d9d9");
        QColor fill(Qt::white);
        if (visual == "Disabled") {
            fill = QColor(Qt::black);
            fill.setAlphaF(.04);
        }
        p->setBrush(fill);
        p->setPen(QPen(border, 1));
        p->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), 3.5, 3.5);
        const QRect logical(opt->rect.right() + 1 - padding(combo) - 16, opt->rect.center().y() - 7,
                            16, 16);
        const auto arrow = visualRect(opt->direction, opt->rect, logical);
        p->setOpacity(combo->isEnabled() ? .65 : .25);
        QSvgRenderer svg(QStringLiteral(":/figma/combo/ChevronDown.svg"));
        svg.render(p, arrow);
        p->restore();
        const_cast<QComboBox*>(combo)->setProperty("figmaComboVisualState", visual);
        const_cast<QComboBox*>(combo)->setProperty("figmaComboArrowBounds", arrow);
    }

    void drawControl(ControlElement ce, const QStyleOption* opt, QPainter* p,
                     const QWidget* widget) const override {
        const auto* combo = qobject_cast<const QComboBox*>(widget);
        const auto* source = qstyleoption_cast<const QStyleOptionComboBox*>(opt);
        if (ce == CE_ComboBoxLabel && combo && source) {
            auto label = *source;
            const bool placeholder = combo->currentIndex() < 0 && !combo->isEditable();
            if (placeholder)
                label.currentText = combo->placeholderText();
            if (combo->property("figmaComboSourceSnapshot").toBool() &&
                qobject_cast<const QFontComboBox*>(combo)) {
                label.currentText = "Arial";
                label.currentIcon = QIcon();
            }
            for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
                label.palette.setColor(group, QPalette::ButtonText,
                                       foreground(combo->isEnabled(), placeholder));
            // Base label painting retains DecorationRole, iconSize, RTL and native elision.
            QProxyStyle::drawControl(ce, &label, p, widget);
            return;
        }
        QProxyStyle::drawControl(ce, opt, p, widget);
    }
};

class ComboBinding final : public QObject {
public:
    explicit ComboBinding(QComboBox* combo) : QObject(combo), combo_(combo) {
        setObjectName("figmaComboBinding");
        combo_->installEventFilter(this);
        shadow_ = new surface::OuterShadowEffect(combo_);
        combo_->setGraphicsEffect(shadow_);
        QObject::connect(combo_, &QComboBox::currentTextChanged, this, [this] {
            refresh();
        });
    }

    void refresh() {
        if (refreshing_)
            return;
        refreshing_ = true;
        if (editor_ != combo_->lineEdit()) {
            if (editor_)
                editor_->removeEventFilter(this);
            editor_ = combo_->lineEdit();
            if (editor_)
                editor_->installEventFilter(this);
        }
        QFont font("HarmonyOS Sans");
        font.setPixelSize(14);
        font.setWeight(QFont::Medium);
        if (combo_->font() != font)
            combo_->setFont(font);
        QPalette palette = combo_->palette();
        for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            palette.setColor(group, QPalette::Text, foreground(group != QPalette::Disabled, false));
            palette.setColor(group, QPalette::PlaceholderText,
                             foreground(group != QPalette::Disabled, true));
            palette.setColor(group, QPalette::Highlight, QColor("#6235f7"));
            palette.setColor(group, QPalette::HighlightedText, Qt::white);
            palette.setColor(group, QPalette::Base, Qt::white);
        }
        if (combo_->palette() != palette)
            combo_->setPalette(palette);
        auto editorPalette = palette;
        editorPalette.setColor(QPalette::Base, Qt::transparent);
        if (editor_ && editor_->palette() != editorPalette)
            editor_->setPalette(editorPalette);
        const auto size = combo_->property("figmaComboSize").toString();
        combo_->setFixedHeight(size == "Small" ? 24 : size == "Large" ? 40 : 32);
        surface::Recipe recipe;
        recipe.radius = 4;
        recipe.borderWidth = 1;
        if (state(combo_) == "Focus")
            recipe.shadows = QJsonArray{QJsonObject{{"kind", "outer"},
                                                    {"x", 0},
                                                    {"y", 0},
                                                    {"blur", 0},
                                                    {"spread", 2},
                                                    {"color", "#f3efff"},
                                                    {"alpha", 1}}};
        shadow_->setSurfaceRect(combo_->rect());
        shadow_->setRecipe(recipe);
        combo_->update();
        refreshing_ = false;
    }

protected:
    bool eventFilter(QObject*, QEvent* e) override {
        switch (e->type()) {
        case QEvent::ChildAdded:
        case QEvent::ChildRemoved:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::EnabledChange:
        case QEvent::Resize:
        case QEvent::StyleChange:
        case QEvent::Show:
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
            break;
        default:
            break;
        }
        return false; // Qt owns every key, mouse, popup, edit and input-method event.
    }

private:
    QComboBox* combo_;
    QPointer<QLineEdit> editor_;
    surface::OuterShadowEffect* shadow_;
    bool refreshing_ = false;
};
}

void applyFigmaComboBoxAppearance(QComboBox* combo, const QString& size,
                                  const QString& snapshotState, bool sourceSnapshot) {
    if (!combo || !QStringList{"Small", "Middle", "Large"}.contains(size) ||
        !QStringList{"", "Default", "Hover", "Focus"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured ComboBox requires native QComboBox and source-backed size/state");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        return true;
    }();
    Q_UNUSED(fonts);
    combo->setProperty("figmaAppearance", "figma-light");
    combo->setProperty("designSystemStyleOwner", "figma-combo-captured");
    combo->setProperty("figmaComboSize", size);
    combo->setProperty("figmaComboSnapshotState", snapshotState);
    combo->setProperty("figmaComboSourceSnapshot", sourceSnapshot);
    auto* binding = dynamic_cast<ComboBinding*>(
        combo->findChild<QObject*>("figmaComboBinding", Qt::FindDirectChildrenOnly));
    if (!binding) {
        combo->setStyle(new ComboStyle(combo));
        binding = new ComboBinding(combo);
        combo->ensurePolished();
    }
    binding->refresh();
}
}
