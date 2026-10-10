#include "Z_CalendarWidget.h"
#include <QVariant>

namespace commonWidget {

Z_CalendarWidget::Z_CalendarWidget(QWidget* parent) : QCalendarWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_CalendarWidget");
    setProperty("nativeQtClass", "QCalendarWidget");
}

} // namespace commonWidget
