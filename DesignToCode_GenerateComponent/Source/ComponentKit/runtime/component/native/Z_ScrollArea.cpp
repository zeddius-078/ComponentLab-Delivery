#include "Z_ScrollArea.h"
#include "FocusIndicator.h"
#include <QScrollBar>
#include <QStyle>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {

Z_ScrollArea::Z_ScrollArea(QWidget* parent) : QScrollArea(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ScrollArea");
    setProperty("nativeQtClass", "QScrollArea");
    setAccessibleName(QStringLiteral("Scrollable content"));
    FocusIndicator::instance().watch(this);
    refreshContract();
}

void Z_ScrollArea::setBars(const QString& value) {
    if (!QStringList{"None", "Vertical", "Horizontal", "Both"}.contains(value))
        throw std::invalid_argument("ScrollArea.bars must match the Figma axis");
    if (barsValue == value) return;
    barsValue = value;
    refreshContract();
    emit barsChanged(value);
}

void Z_ScrollArea::setResizeMode(const QString& value) {
    if (!QStringList{"Fixed", "Resizable"}.contains(value))
        throw std::invalid_argument("ScrollArea.resizeMode must match the Figma axis");
    if (resizeModeValue == value) return;
    resizeModeValue = value;
    refreshContract();
    emit resizeModeChanged(value);
}

void Z_ScrollArea::setSnapshotState(const QString& value) {
    if (!QStringList{"Default", "Focus", "Disabled"}.contains(value))
        throw std::invalid_argument("ScrollArea.snapshotState must match the Figma axis");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    refreshContract();
    emit snapshotStateChanged(value);
}

QSize Z_ScrollArea::sizeHint() const { return {372, 210}; }

void Z_ScrollArea::refreshContract() {
    const bool vertical = barsValue == QStringLiteral("Vertical") ||
                          barsValue == QStringLiteral("Both");
    const bool horizontal = barsValue == QStringLiteral("Horizontal") ||
                            barsValue == QStringLiteral("Both");
    setVerticalScrollBarPolicy(vertical ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(horizontal ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
    setWidgetResizable(resizeModeValue == QStringLiteral("Resizable"));
    setEnabled(snapshotStateValue != QStringLiteral("Disabled"));
    setProperty("figmaSnapshotState", snapshotStateValue);
    for (auto* bar : {verticalScrollBar(), horizontalScrollBar()}) {
        bar->setProperty("designSystemNative", true);
        bar->setProperty("designSystemClass", "commonWidget::Z_ScrollBar");
        bar->setProperty("designSystemStyleOwner", "tokens");
    }
    style()->unpolish(this);
    style()->polish(this);
    viewport()->setAutoFillBackground(true);
    updateGeometry();
    update();
}

} // namespace commonWidget
