#include "LineEditAppearance.h"
#include "../surface/Surface.h"
#include <QAction>
#include <QEvent>
#include <QFontDatabase>
#include <QLineEdit>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionFrame>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QString state(const QLineEdit* edit) {
    if (!edit->isEnabled())
        return "Disabled";
    const auto fixed = edit->property("figmaLineEditSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (edit->hasFocus())
        return "Focus";
    if (edit->underMouse())
        return "Hover";
    return edit->text().isEmpty() ? "Default" : "Filled";
}

int padding(const QLineEdit* edit) {
    const auto size = edit->property("figmaLineEditSize").toString();
    return size == "Small" ? 8 : size == "Large" ? 16 : 12;
}

QIcon glyph(const QString& name) {
    QIcon icon;
    for (const auto mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected})
        icon.addFile(":/figma/lineedit/" + name + ".svg", QSize(16, 16), mode);
    return icon;
}

class LineEditStyle final : public QProxyStyle {
public:
    explicit LineEditStyle(QLineEdit* edit) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(edit);
    }

    QRect subElementRect(SubElement element, const QStyleOption* opt,
                         const QWidget* widget) const override {
        if (element == SE_LineEditContents)
            if (const auto* edit = qobject_cast<const QLineEdit*>(widget)) {
                // QLineEdit adds its native 2px horizontal text margin after this box.
                // Preserve caller-owned textMargins and native QAction reservations.
                const int inset = padding(edit) - 1;
                return opt->rect.adjusted(inset, 1, -inset, -1);
            }
        return QProxyStyle::subElementRect(element, opt, widget);
    }

    int pixelMetric(PixelMetric metric, const QStyleOption* opt,
                    const QWidget* widget) const override {
        if (metric == PM_SmallIconSize && qobject_cast<const QLineEdit*>(widget))
            return 16;
        return QProxyStyle::pixelMetric(metric, opt, widget);
    }

    QIcon standardIcon(StandardPixmap standard, const QStyleOption* opt,
                       const QWidget* widget) const override {
        if (standard == SP_LineEditClearButton && qobject_cast<const QLineEdit*>(widget))
            return glyph("Clear");
        return QProxyStyle::standardIcon(standard, opt, widget);
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption* opt, QPainter* painter,
                       const QWidget* widget) const override {
        const auto* edit = qobject_cast<const QLineEdit*>(widget);
        if (element != PE_PanelLineEdit || !edit) {
            QProxyStyle::drawPrimitive(element, opt, painter, widget);
            return;
        }
        const auto visual = state(edit);
        const QColor border(visual == "Disabled" ? "#d0d0d0"
                            : visual == "Error"  ? "#e02128"
                            : visual == "Focus"  ? "#6235f7"
                            : visual == "Hover"  ? "#191919"
                                                 : "#a7a7a7");
        const QColor background(visual == "Disabled" ? "#f3f3f3"
                                : visual == "Error"  ? "#fbe4e5"
                                                     : "#ffffff");
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(background);
        painter->setPen(QPen(border, 1));
        painter->drawRoundedRect(QRectF(opt->rect).adjusted(.5, .5, -.5, -.5), 3.5, 3.5);
        painter->restore();
        const_cast<QLineEdit*>(edit)->setProperty("figmaLineEditVisualState", visual);
    }
};

class LineEditBinding final : public QObject {
public:
    explicit LineEditBinding(QLineEdit* edit) : QObject(edit), edit_(edit) {
        setObjectName("figmaLineEditBinding");
        edit_->installEventFilter(this);
        shadow_ = new surface::OuterShadowEffect(edit_);
        edit_->setGraphicsEffect(shadow_);
        QObject::connect(edit_, &QLineEdit::textChanged, this, [this] {
            refresh();
        });
    }

    void refresh() {
        if (refreshing_)
            return;
        refreshing_ = true;
        const auto visual = state(edit_);
        QFont font("Inter");
        font.setPixelSize(14);
        font.setWeight(edit_->text().isEmpty() ? QFont::Normal : QFont::Medium);
        if (edit_->font() != font)
            edit_->setFont(font);
        QPalette palette = edit_->palette();
        for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            palette.setColor(group, QPalette::Text,
                             QColor(group == QPalette::Disabled ? "#8a8a8a" : "#191919"));
            palette.setColor(group, QPalette::PlaceholderText,
                             QColor(group == QPalette::Disabled ? "#8a8a8a" : "#676767"));
            palette.setColor(group, QPalette::Highlight, QColor("#6235f7"));
            palette.setColor(group, QPalette::HighlightedText, Qt::white);
        }
        if (edit_->palette() != palette)
            edit_->setPalette(palette);
        surface::Recipe recipe;
        recipe.radius = 4;
        recipe.borderWidth = 1;
        if (visual == "Focus")
            recipe.shadows = QJsonArray{QJsonObject{{"kind", "outer"},
                                                    {"x", 0},
                                                    {"y", 0},
                                                    {"blur", 0},
                                                    {"spread", 2},
                                                    {"color", "#f3efff"},
                                                    {"alpha", 1}}};
        shadow_->setSurfaceRect(edit_->rect());
        shadow_->setRecipe(recipe);
        edit_->setFixedHeight(edit_->property("figmaLineEditSize").toString() == "Large" ? 40 : 32);
        edit_->update();
        refreshing_ = false;
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        switch (event->type()) {
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::EnabledChange:
        case QEvent::Resize:
        case QEvent::StyleChange:
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
            break;
        default:
            break;
        }
        return false; // Native paint, selection, IME and every input event remain intact.
    }

private:
    QLineEdit* edit_;
    surface::OuterShadowEffect* shadow_;
    bool refreshing_ = false;
};
}

void applyFigmaLineEditAppearance(QLineEdit* edit, const QString& size,
                                  const QString& snapshotState) {
    if (!edit || !QStringList{"Small", "Medium", "Large"}.contains(size) ||
        !QStringList{"", "Default", "Hover", "Focus", "Filled", "Error"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured LineEdit requires native QLineEdit and source-backed size/state");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
        return true;
    }();
    Q_UNUSED(fonts);
    edit->setProperty("figmaAppearance", "figma-light");
    edit->setProperty("designSystemStyleOwner", "figma-lineedit-captured");
    edit->setProperty("figmaLineEditSize", size);
    edit->setProperty("figmaLineEditSnapshotState", snapshotState);
    auto* binding = dynamic_cast<LineEditBinding*>(
        edit->findChild<QObject*>("figmaLineEditBinding", Qt::FindDirectChildrenOnly));
    if (!binding) {
        edit->setStyle(new LineEditStyle(edit));
        binding = new LineEditBinding(edit);
        edit->ensurePolished();
    }
    binding->refresh();
}

void applyFigmaLineEditActions(QLineEdit* edit, bool showPrefix, bool showSuffix) {
    if (!edit)
        throw std::invalid_argument("LineEdit actions require a native QLineEdit");
    for (const auto& spec :
         QList<QPair<QString, bool>>{{"Prefix", showPrefix}, {"Suffix", showSuffix}}) {
        const auto name = "figmaLineEdit" + spec.first + "Action";
        auto* action = edit->findChild<QAction*>(name, Qt::FindDirectChildrenOnly);
        if (!action && spec.second) {
            action = edit->addAction(glyph(spec.first), spec.first == "Prefix"
                                                            ? QLineEdit::LeadingPosition
                                                            : QLineEdit::TrailingPosition);
            action->setObjectName(name);
            action->setText(spec.first);
            action->setToolTip(spec.first);
        }
        if (action)
            action->setVisible(spec.second);
    }
}
}
