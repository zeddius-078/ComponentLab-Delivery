#include "Z_Splitter.h"
#include <QPainter>
#include <QSplitterHandle>
#include <QVariant>

namespace commonWidget {
namespace {

class DesignSplitterHandle final : public QSplitterHandle {
  public:
    DesignSplitterHandle(Qt::Orientation orientation, Z_Splitter* parent)
        : QSplitterHandle(orientation, parent) {
        setProperty("designSystemNative", true);
        setProperty("designSystemClass", "commonWidget::Z_SplitterHandle");
        setAccessibleName(QStringLiteral("Splitter handle"));
    }
  protected:
    void paintEvent(QPaintEvent* event) override {
        QSplitterHandle::paintEvent(event);
        const auto* owner = qobject_cast<const Z_Splitter*>(splitter());
        if (!owner || !owner->showGrip()) return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().color(QPalette::Base));
        const QRect grip = orientation() == Qt::Horizontal
                              ? QRect((width() - 4) / 2, (height() - 32) / 2, 4, 32)
                              : QRect((width() - 32) / 2, (height() - 4) / 2, 32, 4);
        painter.drawRoundedRect(grip, 2, 2);
    }
};

} // namespace

Z_Splitter::Z_Splitter(QWidget* parent) : QSplitter(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Splitter");
    setProperty("nativeQtClass", "QSplitter");
    setAccessibleName(QStringLiteral("Resizable splitter"));
    setFocusPolicy(Qt::NoFocus);
    setHandleWidth(12);
    setChildrenCollapsible(true);
    setOpaqueResize(true);
}

void Z_Splitter::setShowGrip(bool value) {
    if (showGripValue == value) return;
    showGripValue = value;
    for (int index = 1; index < count(); ++index)
        if (auto* item = handle(index)) item->update();
    emit showGripChanged(value);
}

QSize Z_Splitter::sizeHint() const { return {372, 196}; }

QSplitterHandle* Z_Splitter::createHandle() {
    return new DesignSplitterHandle(orientation(), this);
}

} // namespace commonWidget
