#include "Z_CalendarWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_CalendarWidget::Z_CalendarWidget(QWidget* parent) : QCalendarWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_CalendarWidget");
    setProperty("nativeQtClass", "QCalendarWidget");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
