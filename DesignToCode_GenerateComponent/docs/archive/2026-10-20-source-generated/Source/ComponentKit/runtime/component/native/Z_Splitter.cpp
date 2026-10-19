#include "Z_Splitter.h"
#include <QPainter>
#include <QResizeEvent>
#include <QSplitterHandle>
#include <QStyle>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {

class DesignSplitterHandle final : public QSplitterHandle {
  public:
    DesignSplitterHandle(Qt::Orientation orientation, Z_Splitter* parent)
        : QSplitterHandle(orientation, parent) {
        setProperty("designSystemNative", true);
        setProperty("designSystemClass", "commonWidget::Z_SplitterHandle");
        setAccessibleName(QStringLiteral("Splitter handle"));
        setAttribute(Qt::WA_StyledBackground, true);
    }
  protected:
    void paintEvent(QPaintEvent* event) override {
        const auto* owner = qobject_cast<const Z_Splitter*>(splitter());
        if (!owner) {
            QSplitterHandle::paintEvent(event);
            return;
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QString state = owner->snapshotState();
        const QPalette::ColorGroup group = owner->isEnabled()
                                                ? QPalette::Active
                                                : QPalette::Disabled;
        QColor handleColor = owner->palette().color(group, QPalette::Midlight);
        if (state == QStringLiteral("Hover"))
            handleColor = owner->palette().color(QPalette::Active, QPalette::AlternateBase);
        else if (state == QStringLiteral("Dragging"))
            handleColor = owner->palette().color(QPalette::Active, QPalette::Highlight);
        painter.fillRect(rect(), handleColor);
        if (!owner->showGrip()) return;
        painter.setPen(Qt::NoPen);
        painter.setBrush(owner->palette().color(group, QPalette::HighlightedText));
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
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);
    setHandleWidth(12);
    setChildrenCollapsible(true);
    setOpaqueResize(true);
    connect(this, &QSplitter::splitterMoved, this,
            [this] { refreshSnapshotState(); });
    refreshSnapshotState();
}

void Z_Splitter::setShowGrip(bool value) {
    if (showGripValue == value) return;
    showGripValue = value;
    for (int index = 1; index < count(); ++index)
        if (auto* item = handle(index)) item->update();
    emit showGripChanged(value);
}

void Z_Splitter::setSnapshotState(const QString& value) {
    const QStringList states{QStringLiteral("Default"), QStringLiteral("Hover"),
                             QStringLiteral("Dragging"), QStringLiteral("Collapsed"),
                             QStringLiteral("Disabled")};
    if (!states.contains(value))
        throw std::invalid_argument(
            "Splitter.snapshotState must be Default, Hover, Dragging, Collapsed or Disabled");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    setEnabled(value != QStringLiteral("Disabled"));
    refreshSnapshotState();
    emit snapshotStateChanged(value);
}

QSize Z_Splitter::sizeHint() const { return {372, 196}; }

QSplitterHandle* Z_Splitter::createHandle() {
    return new DesignSplitterHandle(orientation(), this);
}

void Z_Splitter::paintEvent(QPaintEvent* event) {
    QSplitter::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,
                                        QPalette::Midlight), 1));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 8.5, 8.5);
}

void Z_Splitter::resizeEvent(QResizeEvent* event) {
    QSplitter::resizeEvent(event);
    refreshSnapshotState();
}

void Z_Splitter::refreshSnapshotState() {
    setProperty("figmaSnapshotState", snapshotStateValue);
    style()->unpolish(this);
    style()->polish(this);
    for (int index = 1; index < count(); ++index) {
        if (auto* item = handle(index)) {
            item->setProperty("figmaSnapshotState", snapshotStateValue);
            item->style()->unpolish(item);
            item->style()->polish(item);
            item->update();
        }
    }
    for (int index = 0; index < count(); ++index) {
        if (auto* pane = widget(index)) {
            const auto evidence = pane->findChildren<QWidget*>(
                QStringLiteral("splitterRubberBandEvidence"), Qt::FindDirectChildrenOnly);
            for (auto* item : evidence) item->hide();
        }
    }
    if (snapshotStateValue == QStringLiteral("Dragging") && count() >= 2) {
        if (auto* pane = widget(1)) {
            auto* evidence = pane->findChild<QWidget*>(
                QStringLiteral("splitterRubberBandEvidence"), Qt::FindDirectChildrenOnly);
            if (!evidence) {
                evidence = new QWidget(pane);
                evidence->setObjectName(QStringLiteral("splitterRubberBandEvidence"));
                evidence->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                evidence->setAutoFillBackground(true);
            }
            auto evidencePalette = evidence->palette();
            evidencePalette.setColor(QPalette::Window,
                                     palette().color(QPalette::Active, QPalette::Highlight));
            evidence->setPalette(evidencePalette);
            if (orientation() == Qt::Horizontal)
                evidence->setGeometry(14, 7, 2, qMax(0, pane->height() - 14));
            else
                evidence->setGeometry(7, 14, qMax(0, pane->width() - 14), 2);
            evidence->show();
            evidence->raise();
        }
    }
    update();
}

} // namespace commonWidget
