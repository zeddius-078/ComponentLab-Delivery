#include "FocusIndicator.h"
#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QStyle>
#include <QWidget>
#include <cmath>
#include <stdexcept>

namespace commonWidget {
FocusIndicator& FocusIndicator::instance() {
    static QPointer<FocusIndicator> policy;
    if (!policy)
        policy = new FocusIndicator(qApp);
    return *policy;
}
FocusIndicator::FocusIndicator(QObject* parent) : QObject(parent) {
    qApp->installEventFilter(this);
    connect(qApp, &QApplication::focusChanged, this, [this] { emit changed(); });
}
FocusIndicator::Mode FocusIndicator::parseMode(const QString& value) {
    if (value == "off")
        return Mode::Off;
    if (value == "keyboard")
        return Mode::Keyboard;
    if (value == "always")
        return Mode::Always;
    throw std::invalid_argument("Focus indicator mode must be off, keyboard or always");
}
QString FocusIndicator::modeName(Mode value) {
    switch (value) {
    case Mode::Off:
        return "off";
    case Mode::Keyboard:
        return "keyboard";
    case Mode::Always:
        return "always";
    }
    throw std::invalid_argument("Unknown focus indicator mode");
}
void FocusIndicator::configure(const QString& defaultMode, qreal opacity) {
    const auto mode = parseMode(defaultMode);
    if (!std::isfinite(opacity) || opacity < 0 || opacity > 1)
        throw std::invalid_argument("Focus indicator opacity must be within 0..1");
    currentMode = mode;
    alpha = opacity;
    emit changed();
}
void FocusIndicator::setMode(Mode value) {
    modeName(value);
    if (currentMode == value)
        return;
    currentMode = value;
    emit changed();
}
QColor FocusIndicator::color(QColor source) const {
    source.setAlphaF(source.alphaF() * alpha);
    return source;
}
bool FocusIndicator::enabledFor(const QWidget* widget) const {
    for (auto* current = widget; current; current = current->parentWidget()) {
        const QVariant value = current->property("focusIndicatorEnabled");
        if (value.isValid())
            return value.toBool();
    }
    return true;
}
void FocusIndicator::setEnabledFor(QWidget* widget, bool enabled) {
    if (!widget)
        return;
    if (widget->property("focusIndicatorEnabled") == QVariant(enabled))
        return;
    widget->setProperty("focusIndicatorEnabled", enabled);
}
bool FocusIndicator::visibleFor(const QWidget* widget) const {
    auto* target = QApplication::focusWidget();
    return widget && enabledFor(widget) && widget->isEnabled() && target &&
           (target == widget || widget->isAncestorOf(target)) &&
           (currentMode == Mode::Always || (currentMode == Mode::Keyboard && keyboard));
}
void FocusIndicator::watch(QWidget* widget) {
    if (!widget || widget->property("_focusIndicatorWatched").toBool())
        return;
    widget->setProperty("_focusIndicatorWatched", true);
    auto refresh = [this, widget] {
        const bool visible = visibleFor(widget);
        if (widget->property("focusIndicatorVisible") != QVariant(visible)) {
            widget->setProperty("focusIndicatorVisible", visible);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
        widget->update();
    };
    connect(this, &FocusIndicator::changed, widget, refresh);
    refresh();
}
bool FocusIndicator::eventFilter(QObject*, QEvent* event) {
    bool next = keyboard;
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin)
        next = false;
    else if (event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Tab || key == Qt::Key_Backtab || key == Qt::Key_Left ||
            key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down ||
            key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter)
            next = true;
    } else if (event->type() == QEvent::FocusIn) {
        const auto reason = static_cast<QFocusEvent*>(event)->reason();
        if (reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
            reason == Qt::ShortcutFocusReason)
            next = true;
        else if (reason == Qt::MouseFocusReason)
            next = false;
    } else if (event->type() == QEvent::DynamicPropertyChange &&
               static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName() ==
                   "focusIndicatorEnabled") {
        // Support direct QWidget::setProperty usage in addition to setEnabledFor().
        emit changed();
    }
    if (keyboard != next) {
        keyboard = next;
        emit changed();
    }
    return false;
}
} // namespace commonWidget
