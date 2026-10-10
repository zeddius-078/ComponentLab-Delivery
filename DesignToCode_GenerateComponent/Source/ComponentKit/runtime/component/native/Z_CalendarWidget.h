#pragma once

#include <QCalendarWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QCalendarWidget behavior.
class Z_CalendarWidget : public QCalendarWidget {
    Q_OBJECT
  public:
    explicit Z_CalendarWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
