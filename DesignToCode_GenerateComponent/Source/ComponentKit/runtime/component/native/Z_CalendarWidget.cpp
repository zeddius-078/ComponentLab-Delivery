#include "Z_CalendarWidget.h"
#include "FocusIndicator.h"
#include <QAbstractItemView>
#include <QDate>
#include <QFontDatabase>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLocale>
#include <QPainter>
#include <QStyle>
#include <QTableView>
#include <QTextCharFormat>
#include <QToolButton>
#include <QVBoxLayout>
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
    setLocale(QLocale(QLocale::Chinese, QLocale::China));
    setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
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
            [this, updateNavigationText](int, int) {
                updateNavigationText();
                refreshLayout();
            });
    if (auto* navigation = findChild<QWidget*>(QStringLiteral("qt_calendar_navigationbar")))
        navigation->setAttribute(Qt::WA_StyledBackground, true);
    updateNavigationText();
    setFocusPolicy(Qt::StrongFocus);
    FocusIndicator::instance().watch(this);
    refreshContract();
    refreshLayout();
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

void Z_CalendarWidget::setShowNavigation(bool value) {
    if (isNavigationBarVisible() == value) return;
    setNavigationBarVisible(value);
    refreshLayout();
    emit showNavigationChanged(value);
}

QSize Z_CalendarWidget::sizeHint() const { return {440, 360}; }

void Z_CalendarWidget::paintCell(QPainter* painter, const QRect& rect, const QDate& date) const {
    painter->save();
    const bool disabled = !isEnabled();
    const QPalette::ColorGroup group = disabled ? QPalette::Disabled : QPalette::Active;
    const QColor surface = disabled ? palette().color(QPalette::Active, QPalette::AlternateBase)
                                    : palette().color(QPalette::Active, QPalette::Base);
    painter->fillRect(rect, surface);

    const bool selected = date == selectedDate();
    const QDate reference = property("figmaReferenceDate").toDate().isValid()
                                ? property("figmaReferenceDate").toDate()
                                : QDate::currentDate();
    const QRect item = rect.adjusted(1, 1, -1, -1);
    if (selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(disabled ? palette().color(QPalette::Active, QPalette::Base)
                                   : palette().color(QPalette::Active, QPalette::Highlight));
        painter->drawRoundedRect(item, 4, 4);
    } else if (date == reference) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(disabled ? palette().color(QPalette::Active, QPalette::Text)
                                     : palette().color(QPalette::Active, QPalette::Highlight), 1));
        painter->drawRect(item);
    }

    QColor foreground = date.month() == monthShown()
                            ? palette().color(group, QPalette::Text)
                            : palette().color(QPalette::Disabled, QPalette::Text);
    if (selected && !disabled)
        foreground = palette().color(QPalette::Active, QPalette::HighlightedText);
    QFont cellFont = font();
    cellFont.setPixelSize(14);
    cellFont.setWeight(QFont::Medium);
    painter->setFont(cellFont);
    painter->setPen(foreground);
    painter->drawText(rect, Qt::AlignCenter, QString::number(date.day()));
    painter->restore();
}

void Z_CalendarWidget::resizeEvent(QResizeEvent* event) {
    QCalendarWidget::resizeEvent(event);
    refreshLayout();
}

void Z_CalendarWidget::refreshLayout() {
    if (auto* outer = qobject_cast<QVBoxLayout*>(layout())) {
        outer->setContentsMargins(16, 16, 16, 15);
        outer->setSpacing(12);
    }
    auto* navigation = findChild<QWidget*>(QStringLiteral("qt_calendar_navigationbar"));
    auto* month = findChild<QToolButton*>(QStringLiteral("qt_calendar_monthbutton"));
    auto* year = findChild<QToolButton*>(QStringLiteral("qt_calendar_yearbutton"));
    auto* previous = findChild<QToolButton*>(QStringLiteral("qt_calendar_prevmonth"));
    auto* next = findChild<QToolButton*>(QStringLiteral("qt_calendar_nextmonth"));
    if (navigation) navigation->setFixedHeight(44);
    if (year) year->hide();
    if (auto* row = navigation ? qobject_cast<QHBoxLayout*>(navigation->layout()) : nullptr;
        row && !navigation->property("figmaCalendarHeaderLayout").toBool()) {
        while (auto* item = row->takeAt(0)) delete item;
        row->setContentsMargins(12, 10, 12, 10);
        row->setSpacing(8);
        if (month) row->addWidget(month, 1);
        if (previous) row->addWidget(previous);
        if (next) row->addWidget(next);
        navigation->setProperty("figmaCalendarHeaderLayout", true);
    }
    if (month) {
        month->setMenu(nullptr);
        month->setPopupMode(QToolButton::DelayedPopup);
        month->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        month->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        month->setFixedHeight(22);
    }
    for (auto* button : {previous, next})
        if (button) button->setFixedSize(16, 16);
    if (auto* view = findChild<QAbstractItemView*>(QStringLiteral("qt_calendar_calendarview"))) {
        static const QString cjkFamily = [] {
            const int id = QFontDatabase::addApplicationFont(
                QStringLiteral(":/figma/fonts/NotoSansSC-Regular.otf"));
            const QStringList families = QFontDatabase::applicationFontFamilies(id);
            return families.isEmpty() ? QStringLiteral("Noto Sans SC") : families.first();
        }();
        QFont viewFont = view->font();
        // QCalendarWidget's private weekday row does not reliably traverse a
        // fallback family list on the Windows Qt 5 raster path. Use the
        // bundled CJK family directly so 一..日 never degrade to tofu glyphs.
        viewFont.setFamily(cjkFamily);
        view->setFont(viewFont);
        if (auto* table = qobject_cast<QTableView*>(view)) {
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            for (int row = 0; row < table->model()->rowCount(); ++row)
                table->setRowHeight(row, row == 0 ? 32 : 40);
        }
    }
}

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
    QTextCharFormat header;
    header.setFontFamily(QStringLiteral("Noto Sans SC"));
    header.setFontPointSize(9.0);
    header.setFontWeight(QFont::Medium);
    header.setForeground(isEnabled() ? palette().color(QPalette::Text)
                                     : palette().color(QPalette::Disabled, QPalette::Text));
    setHeaderTextFormat(header);

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
