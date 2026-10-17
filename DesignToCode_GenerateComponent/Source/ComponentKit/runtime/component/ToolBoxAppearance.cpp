#include "ToolBoxAppearance.h"
#include "FocusIndicator.h"
#include <QAbstractButton>
#include <QEvent>
#include <QFontDatabase>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QPointer>
#include <QProxyStyle>
#include <QScrollArea>
#include <QStyleFactory>
#include <QStyleOptionToolBox>
#include <QSvgRenderer>
#include <QTimer>
#include <QToolBox>
#include <QVariant>
#include <QVBoxLayout>
#include <stdexcept>

namespace commonWidget {
namespace {
QLabel* compositionLabel(QWidget* parent, const QString& text, const QRect& rect, int size,
                         int weight, const QString& color, bool wrap = false) {
    auto* result = new QLabel(text, parent);
    result->setTextFormat(Qt::PlainText);
    result->setGeometry(rect);
    result->setWordWrap(wrap);
    result->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont labelFont("HarmonyOS Sans");
    labelFont.setPixelSize(size);
    labelFont.setWeight(weight);
    result->setFont(labelFont);
    result->setStyleSheet(
        QString("background:transparent;border:0;color:%1;font-family:'HarmonyOS Sans';font-size:%2px;")
            .arg(color)
            .arg(size));
    return result;
}

QFont labelFont() {
    QFont f("HarmonyOS Sans");
    f.setPixelSize(14);
    f.setWeight(QFont::DemiBold);
    return f;
}

void populateFigmaToolBoxImpl(QToolBox* box, bool sourceSnapshot) {
    if (!box)
        throw std::invalid_argument("ToolBox is null");
    while (box->count()) {
        auto* page = box->widget(0);
        box->removeItem(0);
        delete page;
    }
    for (const auto& name : {QStringLiteral("General"), QStringLiteral("Rules"),
                             QStringLiteral("Notifications")}) {
        auto* page = new QWidget;
        page->setObjectName(QStringLiteral("toolBoxDemo") + name);
        auto* title = compositionLabel(page, name + QStringLiteral(" page"),
                                       QRect(23, 23, 700, 26), 18, QFont::DemiBold,
                                       QStringLiteral("#191919"));
        auto* body = compositionLabel(
            page,
            QStringLiteral("Only the current item widget is shown here. Other item widgets remain "
                           "owned by the ToolBox until removed."),
            QRect(23, 61, 700, 19), 13, QFont::Normal, QStringLiteral("#191919"),
            !sourceSnapshot);
        if (!sourceSnapshot) {
            auto* layout = new QVBoxLayout(page);
            layout->setContentsMargins(23, 23, 23, 20);
            layout->setSpacing(12);
            for (auto* text : {title, body}) {
                text->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
                layout->addWidget(text);
            }
            layout->addStretch();
        }
        box->addItem(page, name == QStringLiteral("General") ? QStringLiteral("&General") : name);
    }
    box->setItemIcon(1, figmaToolBoxSettingsIcon(true));
    box->setItemEnabled(2, false);
    box->setCurrentIndex(1);
    applyFigmaToolBoxAppearance(box);
}

void svg(QPainter* p, const QString& name, const QRect& rect) {
    QSvgRenderer asset(":/figma/toolbox/" + name + ".svg");
    asset.render(p, rect);
}

class ItemStyle final : public QProxyStyle {
public:
    ItemStyle(QToolBox* box, QAbstractButton* button)
        : QProxyStyle(QStyleFactory::create("Fusion")), box_(box), button_(button) {
        setParent(button);
        setObjectName("figmaToolBoxItemStyle");
    }

    void drawControl(ControlElement ce, const QStyleOption* option, QPainter* p,
                     const QWidget* widget) const override {
        const auto* opt = qstyleoption_cast<const QStyleOptionToolBox*>(option);
        if (!box_ || !button_ || !opt || ce != CE_ToolBoxTab) {
            QProxyStyle::drawControl(ce, option, p, widget);
            return;
        }
        const int index = button_->property("figmaToolBoxItemIndex").toInt();
        const auto fixed = box_->property("figmaToolBoxSnapshotState").toString();
        const int target = box_->property("figmaToolBoxPreviewIndex").toInt();
        const auto visual = !button_->isEnabled() ? QString("Disabled")
                            : !fixed.isEmpty() && (target < 0 || target == index) ? fixed
                            : opt->state.testFlag(State_Selected)  ? QString("Selected")
                            : opt->state.testFlag(State_MouseOver) ? QString("Hover")
                                                                   : QString("Default");
        const bool selected = visual == "Selected", disabled = visual == "Disabled";
        const bool overridden = !fixed.isEmpty() && (target < 0 || target == index);
        const bool focus = visual == "Focus" || (!overridden && button_->hasFocus() &&
                                                 FocusIndicator::instance().visibleFor(button_));
        const auto r = opt->rect;
        const qreal border = focus ? 2 : 1;
        p->save();
        p->setRenderHint(QPainter::Antialiasing, false);
        p->setPen(QPen(QColor(focus ? "#6235f7" : "#d0d0d0"), border));
        p->setBrush(QColor(selected || visual == "Hover" ? "#f3f3f3" : "white"));
        p->drawRect(QRectF(r).adjusted(border / 2, border / 2, -border / 2, -border / 2));
        const auto mirrored = [&](const QRect& rect) {
            return visualRect(opt->direction, r, rect);
        };
        const auto arrow = mirrored(QRect(r.x() + 19, r.y() + 16, 16, 16));
        svg(p, disabled ? "ChevronDisabled" : selected ? "ChevronDown" : "ChevronRight", arrow);
        const auto icon = mirrored(QRect(r.x() + 48, r.y() + 15, 16, 16));
        if (!opt->icon.isNull())
            opt->icon.paint(p, icon, Qt::AlignLeft | Qt::AlignTop,
                            disabled ? QIcon::Disabled : QIcon::Normal, QIcon::Off);
        QString hint;
        if (box_->property("figmaToolBoxShowMnemonic").toBool())
            hint = button_->shortcut().toString(QKeySequence::NativeText);
        const int hintWidth =
            hint.isEmpty() ? 0 : qMax(50, QFontMetrics(button_->font()).horizontalAdvance(hint));
        const auto hintRect =
            mirrored(QRect(r.right() + 1 - 16 - hintWidth, r.y() + 16, hintWidth, 18));
        const int labelWidth =
            box_->property("figmaToolBoxSourceSnapshot").toBool()
                ? 620
                : qMax(0, r.width() - 76 - 16 - (hint.isEmpty() ? 0 : hintWidth + 12));
        const auto textRect = mirrored(QRect(r.x() + 76, r.y() + 15, labelWidth, 22));
        p->setFont(labelFont());
        p->setPen(QColor(disabled ? "#8a8a8a" : selected ? "#6235f7" : "#191919"));
        int flags = Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextShowMnemonic |
                    (opt->direction == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft);
        if (box_->property("figmaToolBoxSourceSnapshot").toBool() ||
            !styleHint(SH_UnderlineShortcut, opt, widget))
            flags |= Qt::TextHideMnemonic;
        p->drawText(
            textRect, flags,
            QFontMetrics(labelFont())
                .elidedText(opt->text, Qt::ElideRight, textRect.width(), Qt::TextShowMnemonic));
        if (!hint.isEmpty()) {
            QFont caption("HarmonyOS Sans");
            caption.setPixelSize(12);
            caption.setWeight(QFont::Normal);
            p->setFont(caption);
            p->setPen(QColor(disabled ? "#8a8a8a" : "#6235f7"));
            p->drawText(hintRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, hint);
        }
        button_->setProperty("figmaToolBoxArrowBounds", arrow);
        button_->setProperty("figmaToolBoxIconBounds", opt->icon.isNull() ? QRect() : icon);
        button_->setProperty("figmaToolBoxLabelBounds", textRect);
        button_->setProperty("figmaToolBoxMnemonicBounds", hint.isEmpty() ? QRect() : hintRect);
        button_->setProperty("figmaToolBoxVisualState", visual);
        button_->setProperty("figmaToolBoxPaintFont", labelFont());
        p->restore();
    }

private:
    QPointer<QToolBox> box_;
    QPointer<QAbstractButton> button_;
};

class Binding final : public QObject {
public:
    explicit Binding(QToolBox* box) : QObject(box), box_(box) {
        setObjectName("figmaToolBoxBinding");
        box->installEventFilter(this);
    }

    void refresh() {
        auto* layout = box_->layout();
        if (!layout)
            return;
        const bool source = box_->property("figmaToolBoxSourceSnapshot").toBool();
        if (!source) {
            if (layout->contentsMargins() != QMargins(16, 16, 16, 16))
                layout->setContentsMargins(16, 16, 16, 16);
            if (layout->spacing() != 0)
                layout->setSpacing(0);
        }
        int index = 0;
        for (int i = 0; i < layout->count(); ++i)
            if (auto* button = qobject_cast<QAbstractButton*>(layout->itemAt(i)->widget())) {
                if (!button->inherits("QToolBoxButton"))
                    continue;
                button->setProperty("figmaToolBoxItemIndex", index++);
                if (!button->findChild<QObject*>("figmaToolBoxItemStyle",
                                                 Qt::FindDirectChildrenOnly))
                    button->setStyle(new ItemStyle(box_, button));
                button->installEventFilter(this);
                if (button->minimumHeight() != 48 || button->maximumHeight() != 48)
                    button->setFixedHeight(48);
                button->setFont(labelFont());
                button->setAttribute(Qt::WA_Hover);
                button->update();
            }
        for (int i = 0; i < box_->count(); ++i) {
            auto* page = box_->widget(i);
            auto* area = qobject_cast<QScrollArea*>(page->parentWidget()->parentWidget());
            if (area && area->parentWidget() == box_ &&
                !area->property("figmaToolBoxPage").toBool()) {
                area->setProperty("figmaToolBoxPage", true);
                area->setStyleSheet("QScrollArea[figmaToolBoxPage=\"true\"] "
                                    "{background:white;border:1px solid #d0d0d0;}");
                auto palette = area->viewport()->palette();
                palette.setColor(QPalette::Window, Qt::white);
                palette.setColor(QPalette::Base, Qt::white);
                area->viewport()->setPalette(palette);
            }
        }
    }

protected:
    bool eventFilter(QObject* object, QEvent* e) override {
        if (object != box_ && e->type() == QEvent::Paint) {
            auto* button = qobject_cast<QAbstractButton*>(object);
            if (!button)
                return false;
            // Qt5 QStyleSheetStyle hard-routes ToolBoxTab/Label to QWindowsStyle.
            // Paint the existing native button once; leave every input event native.
            QStyleOptionToolBox option;
            option.initFrom(button);
            option.text = button->text();
            option.icon = button->icon();
            if (box_->currentIndex() == button->property("figmaToolBoxItemIndex").toInt())
                option.state |= QStyle::State_Selected;
            auto* paintStyle = static_cast<ItemStyle*>(
                button->findChild<QObject*>("figmaToolBoxItemStyle", Qt::FindDirectChildrenOnly));
            QPainter painter(button);
            paintStyle->drawControl(QStyle::CE_ToolBoxTab, &option, &painter, box_);
            return true;
        }
        if ((e->type() == QEvent::ChildAdded || e->type() == QEvent::LayoutRequest ||
             e->type() == QEvent::Show) &&
            !queued_) {
            queued_ = true;
            QTimer::singleShot(0, this, [this] {
                queued_ = false;
                refresh();
            });
        }
        return false;
    }

private:
    QToolBox* box_;
    bool queued_ = false;
};
}

void populateFigmaToolBox(QToolBox* box, bool sourceSnapshot) {
    populateFigmaToolBoxImpl(box, sourceSnapshot);
}

QIcon figmaToolBoxSettingsIcon(bool composition) {
    QIcon icon;
    icon.addFile(":/figma/toolbox/" +
                     QString(composition ? "SettingsComposition" : "SettingsPrimary") + ".svg",
                 QSize(16, 16), QIcon::Normal);
    icon.addFile(":/figma/toolbox/SettingsDisabled.svg", QSize(16, 16), QIcon::Disabled);
    return icon;
}

void applyFigmaToolBoxAppearance(QToolBox* box, const QString& snapshotState, int previewIndex,
                                 bool showMnemonic) {
    if (!box || !QStringList{"", "Default", "Hover", "Selected", "Focus", "Disabled"}.contains(
                    snapshotState))
        throw std::invalid_argument("Native ToolBox and captured state required");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        return true;
    }();
    Q_UNUSED(fonts);
    box->setProperty("designSystemStyleOwner", "figma-toolbox-captured");
    box->setProperty("figmaToolBoxSnapshotState", snapshotState);
    box->setProperty("figmaToolBoxPreviewIndex", previewIndex);
    box->setProperty("figmaToolBoxShowMnemonic", showMnemonic);
    box->setStyleSheet("QToolBox[designSystemStyleOwner=\"figma-toolbox-captured\"] "
                       "{background:white;border:1px solid #d0d0d0;border-radius:10px;}");
    auto* binding = static_cast<Binding*>(
        box->findChild<QObject*>("figmaToolBoxBinding", Qt::FindDirectChildrenOnly));
    if (!binding)
        binding = new Binding(box);
    binding->refresh();
    box->update();
}
}
