#include "RadioAppearance.h"
#include "FocusIndicator.h"
#include <QEvent>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QRadioButton>
#include <QStyle>
#include <QSvgRenderer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
class RadioRenderer final : public QObject {
public:
    explicit RadioRenderer(QRadioButton* button) : QObject(button), button_(button) {
        setObjectName("figmaRadioRenderer");
        button_->installEventFilter(this);
        button_->setAttribute(Qt::WA_Hover);
        FocusIndicator::instance().watch(button_);
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave ||
            event->type() == QEvent::DynamicPropertyChange)
            button_->update();
        if (event->type() != QEvent::Paint)
            return false;
        QString state = button_->property("figmaRadioSnapshotState").toString();
        if (!button_->isEnabled())
            state = "Disabled";
        else if (state.isEmpty()) {
            if (button_->isDown())
                state = "Pressed";
            else if (FocusIndicator::instance().visibleFor(button_))
                state = "Focus-visible";
            else if (button_->underMouse())
                state = "Hover";
            else
                state = "Default";
        }
        const QString asset =
            QString(":/figma/radio/%1-%2.svg")
                .arg(button_->isChecked() ? "selected" : "unselected", state.toLower());
        const bool focus = state == "Focus-visible";
        // The original focus SVG is 22x22, including the 2px outward effect;
        // all other source assets are 18x18. Do not scale either source asset.
        const int extent = focus ? 22 : 18;
        QRect slot(focus ? 0 : 2, (button_->height() - extent) / 2, extent, extent);
        slot = QStyle::visualRect(button_->layoutDirection(), button_->rect(), slot);
        QPainter painter(button_);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.save();
        // Qt 5 SVG ignores the source clipPath. Reproduce the Figma Indicator
        // frame's clipsContent=true, radius=10 at 18px without editing the SVG.
        QPainterPath clip;
        clip.addEllipse(slot);
        painter.setClipPath(clip);
        QSvgRenderer svg(asset);
        svg.render(&painter, slot);
        painter.restore();
        if (button_->property("figmaRadioShowLabel").toBool()) {
            const QRect label = QStyle::visualRect(
                button_->layoutDirection(), button_->rect(),
                QRect(28, (button_->height() - 20) / 2, qMax(0, button_->width() - 30), 14));
            QPalette palette = button_->palette();
            palette.setColor(QPalette::WindowText,
                             state == "Disabled" ? QColor("#8a8a8a") : QColor("#191919"));
            palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#8a8a8a"));
            int flags =
                Qt::AlignVCenter | Qt::AlignLeading | Qt::TextSingleLine | Qt::TextShowMnemonic;
            if (!button_->style()->styleHint(QStyle::SH_UnderlineShortcut, nullptr, button_))
                flags |= Qt::TextHideMnemonic;
            button_->style()->drawItemText(&painter, label, flags, palette, button_->isEnabled(),
                                           button_->text(), QPalette::WindowText);
        }
        return true;
    }

private:
    QRadioButton* button_;
};
}

void applyFigmaRadioAppearance(QRadioButton* button, const QString& snapshotState, bool showLabel) {
    if (!button ||
        !QStringList{"", "Default", "Hover", "Pressed", "Focus-visible", "Disabled"}.contains(
            snapshotState))
        throw std::invalid_argument("Radio requires a current Figma state or empty live state");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
        QFontDatabase::addApplicationFont(":/figma/fonts/NotoSansSC-Regular.otf");
        return true;
    }();
    Q_UNUSED(fonts);
    QFont font;
    font.setFamilies({"Inter", "Noto Sans SC"});
    font.setPixelSize(14);
    font.setWeight(QFont::Medium);
    button->setFont(font);
    button->setProperty("figmaAppearance", "figma-light");
    button->setProperty("figmaRadioSnapshotState", snapshotState);
    button->setProperty("figmaRadioShowLabel", showLabel);
    // Native style geometry still governs sizeHint and hitButton; painting alone
    // is adapted. Keep the native text for mnemonic and accessibility contracts.
    button->setStyleSheet("QRadioButton {background:transparent;border:0;padding:2px;spacing:8px;"
                          "min-height:28px;color:#191919;font-family:'Inter','Noto Sans SC';"
                          "font-size:14px;font-weight:500;}"
                          "QRadioButton::indicator {width:18px;height:18px;}"
                          "QRadioButton:disabled {color:#8a8a8a;}");
    if (!button->findChild<QObject*>("figmaRadioRenderer", Qt::FindDirectChildrenOnly))
        new RadioRenderer(button);
    button->update();
}
}
