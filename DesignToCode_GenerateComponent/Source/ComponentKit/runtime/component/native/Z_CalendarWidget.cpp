#include "Z_CalendarWidget.h"
#include "FocusIndicator.h"
#include <QAbstractItemView>
#include <QDate>
#include <QIcon>
#include <QStyle>
#include <QTextCharFormat>
#include <QToolButton>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {

Z_CalendarWidget::Z_CalendarWidget(QWidget* parent) : QCalendarWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_CalendarWidget");
    setProperty("nativeQtClass", "QCalendarWidget");
    setProperty("designSystemStyleOwner", "tokens");
    setAccessibleName(QStringLiteral("Calendar"));
    setNavigationBarVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    setDateEditEnabled(false);
    setSelectedDate(QDate(2026, 9, 23));
    setCurrentPage(2026, 9);
    if (auto* previous = findChild<QToolButton*>(QStringLiteral("qt_calendar_prevmonth")))
        previous->setIcon(QIcon(QStringLiteral(":/figma/composite/chevron-left.svg")));
    if (auto* next = findChild<QToolButton*>(QStringLiteral("qt_calendar_nextmonth")))
        next->setIcon(QIcon(QStringLiteral(":/figma/composite/chevron-right.svg")));
    const auto updateNavigationText = [this] {
        if (auto* month = findChild<QToolButton*>(QStringLiteral("qt_calendar_monthbutton")))
            month->setText(QStringLiteral("%1 年 %2 月").arg(yearShown()).arg(monthShown()));
        if (auto* year = findChild<QToolButton*>(QStringLiteral("qt_calendar_yearbutton")))
            year->hide();
    };
    connect(this, &QCalendarWidget::currentPageChanged, this,
            [updateNavigationText](int, int) { updateNavigationText(); });
    if (auto* navigation = findChild<QWidget*>(QStringLiteral("qt_calendar_navigationbar")))
        navigation->setAttribute(Qt::WA_StyledBackground, true);
    updateNavigationText();
    FocusIndicator::instance().watch(this);
    refreshContract();
}

void Z_CalendarWidget::setWeekStart(const QString& value) {
    if (value != QStringLiteral("Monday") && value != QStringLiteral("Sunday"))
        throw std::invalid_argument("Calendar.weekStart must be Monday or Sunday");
    if (weekStartValue == value) return;
    weekStartValue = value;
    refreshContract();
    emit weekStartChanged(value);
}

void Z_CalendarWidget::setGrid(bool value) {
    if (gridValue == value) return;
    gridValue = value;
    refreshContract();
    emit gridChanged(value);
}

void Z_CalendarWidget::setSnapshotState(const QString& value) {
    if (value != QStringLiteral("Default") && value != QStringLiteral("Disabled"))
        throw std::invalid_argument("Calendar.snapshotState must be Default or Disabled");
    if (snapshotStateValue == value) return;
    snapshotStateValue = value;
    refreshContract();
    emit snapshotStateChanged(value);
}

QSize Z_CalendarWidget::sizeHint() const { return {440, 360}; }

void Z_CalendarWidget::refreshContract() {
    setFirstDayOfWeek(weekStartValue == QStringLiteral("Monday") ? Qt::Monday : Qt::Sunday);
    setGridVisible(gridValue);
    setEnabled(snapshotStateValue != QStringLiteral("Disabled"));
    setProperty("figmaWeekStart", weekStartValue);
    setProperty("figmaGrid", gridValue ? QStringLiteral("True") : QStringLiteral("False"));
    setProperty("figmaSnapshotState", snapshotStateValue);
    QTextCharFormat weekday;
    weekday.setForeground(isEnabled() ? palette().color(QPalette::Text)
                                      : palette().color(QPalette::Disabled, QPalette::Text));
    for (const auto day : {Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday,
                           Qt::Friday, Qt::Saturday, Qt::Sunday})
        setWeekdayTextFormat(day, weekday);

    // QCalendarWidget owns private child widgets whose native palette can win
    // over an ancestor dynamic-property selector. Keep the generated QSS as
    // the Token source, then bridge those two private surfaces with semantic
    // palette roles so the Figma header and disabled selection remain stable
    // across Qt platform styles.
    if (auto* navigation = findChild<QWidget*>(QStringLiteral("qt_calendar_navigationbar"))) {
        QPalette navigationPalette = navigation->palette();
        const QColor header = palette().color(QPalette::Active, QPalette::AlternateBase);
        navigationPalette.setColor(QPalette::Window, header);
        navigationPalette.setColor(QPalette::Button, header);
        navigation->setPalette(navigationPalette);
        navigation->setAutoFillBackground(true);
        navigation->setStyleSheet(QStringLiteral("background-color: %1;").arg(header.name(QColor::HexArgb)));
    }
    if (auto* view = findChild<QAbstractItemView*>(QStringLiteral("qt_calendar_calendarview"))) {
        QPalette viewPalette = view->palette();
        const bool disabled = snapshotStateValue == QStringLiteral("Disabled");
        const QColor foreground = palette().color(disabled ? QPalette::Disabled : QPalette::Active,
                                                   QPalette::Text);
        const QColor surface = palette().color(QPalette::Active,
                                                disabled ? QPalette::AlternateBase : QPalette::Base);
        const QColor highlight = disabled
                                     ? palette().color(QPalette::Disabled, QPalette::Button)
                                     : palette().color(QPalette::Active, QPalette::Highlight);
        const QColor highlightedText = palette().color(disabled ? QPalette::Disabled : QPalette::Active,
                                                        disabled ? QPalette::Text : QPalette::HighlightedText);
        if (snapshotStateValue == QStringLiteral("Disabled")) {
            viewPalette.setColor(QPalette::Disabled, QPalette::Base, surface);
            viewPalette.setColor(QPalette::Disabled, QPalette::Text, foreground);
            viewPalette.setColor(QPalette::Disabled, QPalette::WindowText, foreground);
            viewPalette.setColor(QPalette::Disabled, QPalette::Highlight,
                                 highlight);
            viewPalette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                                 highlightedText);
        } else {
            viewPalette.setColor(QPalette::Active, QPalette::Base, surface);
            viewPalette.setColor(QPalette::Active, QPalette::Text, foreground);
            viewPalette.setColor(QPalette::Active, QPalette::Highlight,
                                 highlight);
            viewPalette.setColor(QPalette::Active, QPalette::HighlightedText,
                                 highlightedText);
        }
        view->setPalette(viewPalette);
        view->setStyleSheet(QStringLiteral(
            "QAbstractItemView { color: %1; background-color: %2; "
            "selection-background-color: %3; selection-color: %4; }")
                .arg(foreground.name(QColor::HexArgb), surface.name(QColor::HexArgb),
                     highlight.name(QColor::HexArgb), highlightedText.name(QColor::HexArgb)));
    }
    style()->unpolish(this);
    style()->polish(this);
    if (auto* navigation = findChild<QWidget*>(QStringLiteral("qt_calendar_navigationbar"))) {
        navigation->style()->unpolish(navigation);
        navigation->style()->polish(navigation);
        navigation->update();
    }
    updateGeometry();
    update();
}

} // namespace commonWidget
