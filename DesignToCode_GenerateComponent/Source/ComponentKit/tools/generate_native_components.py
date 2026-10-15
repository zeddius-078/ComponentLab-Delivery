"""Generate one Z_* C++ and QSS contract for every concrete Qt visual component."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFINITIONS = ROOT / "component-definitions/native-components.json"
COMPONENT_DIR = ROOT / "runtime/component/native"
STYLE_DIR = ROOT / "design-assets/styles/native"
GENERATED_STYLE_DIR = ROOT / "design-assets/generated/native"
SOURCE_MAP = ROOT / "component-definitions/generated-component-source-map.json"
BUTTON_MANIFEST = ROOT / "design-assets/generated/button/manifest.json"
RUNTIME_TOKENS = ROOT / "design-assets/DesignSystem/generated/runtime/tokens_qt.json"
CAPTURED_STYLE_CLASSES = {"QDateTimeEdit", "QDateEdit", "QTimeEdit", "QSlider", "QScrollBar", "QDial", "QLineEdit", "QKeySequenceEdit", "QComboBox", "QFontComboBox", "QCommandLinkButton", "QToolButton", "QDialogButtonBox", "QTabWidget", "QToolBox", "QDockWidget", "QMdiArea", "QMdiSubWindow", "QCalendarWidget"}


def load_inputs():
    contract = json.loads(DEFINITIONS.read_text(encoding="utf-8"))
    components = contract["components"]
    qt_classes = {item["qtClass"] for item in components}
    sdk_classes = {item["sdkClass"] for item in components}
    assert components, "native component registry must not be empty"
    assert len(qt_classes) == len(components)
    assert len(sdk_classes) == len(components)
    for item in components:
        expected = "Z_" + item["qtClass"][1:]
        assert item["sdkClass"] == expected, (item["sdkClass"], expected)
        assert item["implementation"] in {"generated", "specialized"}
    token_manifest = json.loads(BUTTON_MANIFEST.read_text(encoding="utf-8"))
    profiles = {item["profile"]: item for item in token_manifest["profiles"]}
    runtime_tokens = json.loads(RUNTIME_TOKENS.read_text(encoding="utf-8"))
    assert set(profiles) == {
        "light-comfortable", "light-compact", "dark-comfortable", "dark-compact"
    }
    for name, profile in profiles.items():
        values = runtime_tokens["profiles"][name]
        profile["nativeMetrics"] = {
            "height": values["layout"]["semantic.dimension.control.height-md"],
            "radius": values["layout"]["semantic.radius.control-md"],
            "fontSize": values["font"]["base.stoken.typography.font-size.label.default"],
        }
    return components, profiles


def header_text(item):
    if item["qtClass"] == "QCalendarWidget":
        return '''#pragma once

#include <QCalendarWidget>

namespace commonWidget {

// Token-backed Calendar identity. WeekStart, Grid and State mirror the
// public Figma component-set axes while date selection remains native Qt API.
class Z_CalendarWidget : public QCalendarWidget {
    Q_OBJECT
    Q_PROPERTY(QString weekStart READ weekStart WRITE setWeekStart NOTIFY weekStartChanged)
    Q_PROPERTY(bool grid READ grid WRITE setGrid NOTIFY gridChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
    Q_PROPERTY(bool showNavigation READ showNavigation WRITE setShowNavigation NOTIFY showNavigationChanged)
  public:
    explicit Z_CalendarWidget(QWidget* parent = nullptr);
    QString weekStart() const { return weekStartValue; }
    bool grid() const { return gridValue; }
    QString snapshotState() const { return snapshotStateValue; }
    bool showNavigation() const { return isNavigationBarVisible(); }
    void setWeekStart(const QString& value);
    void setGrid(bool value);
    void setSnapshotState(const QString& value);
    void setShowNavigation(bool value);
    QSize sizeHint() const override;
  signals:
    void weekStartChanged(const QString& value);
    void gridChanged(bool value);
    void snapshotStateChanged(const QString& value);
    void showNavigationChanged(bool value);
  protected:
    void paintCell(QPainter* painter, const QRect& rect, const QDate& date) const override;
    void resizeEvent(QResizeEvent* event) override;
  private:
    void refreshContract();
    void refreshLayout();
    QString weekStartValue = QStringLiteral("Monday");
    bool gridValue = true;
    QString snapshotStateValue = QStringLiteral("Default");
};

} // namespace commonWidget
'''
    if item["qtClass"] == "QSplitter":
        return '''#pragma once

#include <QSplitter>

namespace commonWidget {

// Token-backed splitter. The root is deliberately not focusable: keyboard
// focus belongs to interactive children hosted in its panes.
class Z_Splitter : public QSplitter {
    Q_OBJECT
    Q_PROPERTY(bool showGrip READ showGrip WRITE setShowGrip NOTIFY showGripChanged)
  public:
    explicit Z_Splitter(QWidget* parent = nullptr);
    bool showGrip() const { return showGripValue; }
    void setShowGrip(bool value);
    QSize sizeHint() const override;
  signals:
    void showGripChanged(bool value);
  protected:
    QSplitterHandle* createHandle() override;
  private:
    bool showGripValue = true;
};

} // namespace commonWidget
'''
    if item["qtClass"] == "QScrollArea":
        return '''#pragma once

#include <QScrollArea>

namespace commonWidget {

// Token-backed ScrollArea identity. Bars and resize behavior are explicit
// Figma axes; content ownership and scrolling remain native QScrollArea APIs.
class Z_ScrollArea : public QScrollArea {
    Q_OBJECT
    Q_PROPERTY(QString bars READ bars WRITE setBars NOTIFY barsChanged)
    Q_PROPERTY(QString resizeMode READ resizeMode WRITE setResizeMode NOTIFY resizeModeChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_ScrollArea(QWidget* parent = nullptr);
    QString bars() const { return barsValue; }
    QString resizeMode() const { return resizeModeValue; }
    QString snapshotState() const { return snapshotStateValue; }
    void setBars(const QString& value);
    void setResizeMode(const QString& value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void barsChanged(const QString& value);
    void resizeModeChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
  private:
    void refreshContract();
    QString barsValue = QStringLiteral("None");
    QString resizeModeValue = QStringLiteral("Fixed");
    QString snapshotStateValue = QStringLiteral("Default");
};

} // namespace commonWidget
'''
    return f'''#pragma once

#include <{item["qtClass"]}>

namespace commonWidget {{

// Project-owned component identity for Qt's {item["qtClass"]} behavior.
class {item["sdkClass"]} : public {item["qtClass"]} {{
    Q_OBJECT
  public:
    explicit {item["sdkClass"]}(QWidget* parent = nullptr);
}};

}} // namespace commonWidget
'''


def source_text(item):
    style_owner = ('    setProperty("designSystemStyleOwner", "tokens");\n'
                   if item["qtClass"] in CAPTURED_STYLE_CLASSES else "")
    if item["qtClass"] == "QCalendarWidget":
        return '''#include "Z_CalendarWidget.h"
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
'''
    if item["qtClass"] == "QSplitter":
        return '''#include "Z_Splitter.h"
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
'''
    if item["qtClass"] == "QScrollArea":
        return '''#include "Z_ScrollArea.h"
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
    setFocusPolicy(Qt::StrongFocus);
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
'''
    return f'''#include "{item["sdkClass"]}.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {{

{item["sdkClass"]}::{item["sdkClass"]}(QWidget* parent) : {item["qtClass"]}(parent) {{
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::{item["sdkClass"]}");
    setProperty("nativeQtClass", "{item["qtClass"]}");
{style_owner}    FocusIndicator::instance().watch(this);
}}

}} // namespace commonWidget
'''


def registry_header_text():
    return '''#pragma once

#include "../ComponentFactory.h"

class QWidget;

namespace commonWidget {

// Compiled registry generated from native-components.json. This is product
// source: callers can inspect and debug it like any other C++ implementation.
QList<NativeComponentDescriptor> nativeComponentDescriptors();
QWidget* createGeneratedNativeComponent(const QString& sdkClass,
                                        QWidget* parent = nullptr);

} // namespace commonWidget
'''


def registry_source_text(components):
    descriptors = "\n".join(
        '        {QStringLiteral("%s"), QStringLiteral("commonWidget::%s"), '
        'QStringLiteral("ComponentLab/%s"), QStringLiteral("visual-component")},'
        % (item["qtClass"], item["sdkClass"], item["designComponent"])
        for item in components
    )
    factories = "\n".join(
        '    if (sdkClass == QStringLiteral("commonWidget::%s")) '
        'return new %s(parent);' % (item["sdkClass"], item["sdkClass"])
        for item in components if item["implementation"] == "generated"
    )
    return f'''#include "NativeComponentRegistry.h"
#include "NativeComponents.h"

namespace commonWidget {{

QList<NativeComponentDescriptor> nativeComponentDescriptors() {{
    return {{
{descriptors}
    }};
}}

QWidget* createGeneratedNativeComponent(const QString& sdkClass, QWidget* parent) {{
{factories}
    return nullptr;
}}

}} // namespace commonWidget
'''


def qss_template(item):
    selector = f'*[designSystemClass="commonWidget::{item["sdkClass"]}"]'
    # Captured native subcontrols own their geometry; global QSS must not
    # replace calendar/slider hit rectangles with generic Token boxes.
    if item["qtClass"] in CAPTURED_STYLE_CLASSES:
        selector += '[designSystemStyleOwner="tokens"]'
    common = f'''/* {item["sdkClass"]} <- {item["qtClass"]}; compiled from the active Token profile. */
{selector} {{
    color: @FOREGROUND@;
    background-color: @SURFACE@;
    font-size: @FONT_SIZE@px;
}}
{selector}:disabled {{
    color: @DISABLED_TEXT@;
    background-color: @DISABLED@;
    border-color: @DISABLED_BORDER@;
}}
'''
    if item["implementation"] == "specialized":
        return (f'/* {item["sdkClass"]} uses its specialized Token compiler output. '
                'This file records its native mapping without overriding that style. */\n')
    if item["category"] == "input":
        common += f'''{selector} {{
    min-height: @HEIGHT@px;
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
    padding-left: 8px;
    padding-right: 8px;
}}
{selector}:focus[focusIndicatorVisible="true"] {{ border-color: @FOCUS@; }}
'''
    elif item["category"] == "button":
        common += f'''{selector} {{
    min-height: @HEIGHT@px;
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
    padding: 0 12px;
}}
{selector}:hover {{ background-color: @PRIMARY_SOFT@; }}
{selector}:pressed {{ border-color: @PRIMARY@; }}
'''
    elif item["qtClass"] == "QProgressBar":
        common += f'''{selector} {{
    min-height: @HEIGHT@px;
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
    text-align: center;
}}
{selector}::chunk {{ background-color: @PRIMARY@; border-radius: @RADIUS@px; }}
'''
    elif item["qtClass"] == "QLabel":
        common += f'{selector} {{ background: transparent; border: none; }}\n'
    elif item["qtClass"] == "QSplitter":
        common += f'''{selector} {{
    border: 1px solid @DISABLED_BORDER@;
    border-radius: 9px;
}}
*[designSystemClass="commonWidget::Z_SplitterHandle"] {{
    background-color: @DISABLED_BORDER@;
    border: 0;
}}
*[designSystemClass="commonWidget::Z_SplitterHandle"]:hover,
*[designSystemClass="commonWidget::Z_SplitterHandle"][figmaSnapshotState="Hover"] {{
    background-color: @NEUTRAL_HOVER@;
}}
*[designSystemClass="commonWidget::Z_SplitterHandle"]:pressed,
*[designSystemClass="commonWidget::Z_SplitterHandle"][figmaSnapshotState="Dragging"] {{
    background-color: @PRIMARY@;
}}
*[designSystemClass="commonWidget::Z_SplitterHandle"]:disabled {{
    background-color: @DISABLED_BORDER@;
}}
'''
    elif item["qtClass"] == "QCalendarWidget":
        common += f'''{selector} {{
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
}}
{selector} QWidget#qt_calendar_navigationbar {{
    background-color: @NEUTRAL_HOVER@;
    border: 0;
    border-radius: @RADIUS@px;
    min-height: 44px;
    max-height: 44px;
}}
{selector} QToolButton {{
    color: @FOREGROUND@;
    background: transparent;
    border: 0;
    border-radius: @RADIUS@px;
    font-weight: 600;
}}
{selector} QToolButton::menu-indicator {{ image: none; width: 0; }}
{selector} QToolButton:hover {{ background-color: @SURFACE@; }}
{selector} QSpinBox {{
    color: @FOREGROUND@;
    background: transparent;
    border: 0;
    selection-background-color: @PRIMARY@;
}}
{selector} QAbstractItemView {{
    color: @FOREGROUND@;
    background-color: @SURFACE@;
    border: 0;
    outline: 0;
    selection-background-color: @PRIMARY@;
    selection-color: @ON_BRAND@;
}}
{selector} QAbstractItemView::item:selected {{
    background-color: @PRIMARY@;
    color: @ON_BRAND@;
    border-radius: @RADIUS@px;
}}
{selector} QAbstractItemView:disabled {{
    color: @DISABLED_TEXT@;
    background-color: @DISABLED@;
    selection-background-color: @DISABLED_BORDER@;
    selection-color: @DISABLED_TEXT@;
}}
{selector} QAbstractItemView:disabled::item:selected {{
    background-color: @DISABLED_BORDER@;
    color: @DISABLED_TEXT@;
}}
{selector}[figmaSnapshotState="Disabled"] QWidget#qt_calendar_navigationbar {{
    background-color: @DISABLED@;
}}
'''
    elif item["qtClass"] == "QScrollArea":
        common += f'''{selector} {{
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
}}
{selector}[figmaSnapshotState="Focus"],
{selector}:focus[focusIndicatorVisible="true"] {{ border: 2px solid @FOCUS@; }}
{selector} QWidget#qt_scrollarea_viewport {{ background-color: @SURFACE@; }}
'''
    elif item["category"] in {"data", "display", "container"}:
        common += f'''{selector} {{
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
}}
'''
    return common


def render_qss(template, profile):
    palette = profile["palette"]
    metrics = profile["nativeMetrics"]
    values = {
        "FOREGROUND": palette["foreground"],
        "SURFACE": palette["surface"],
        "PRIMARY": palette["primary"],
        "PRIMARY_SOFT": palette["primarySoft"],
        "NEUTRAL_HOVER": palette["neutralHover"],
        "ON_BRAND": palette["onBrand"],
        "BORDER": palette["border"],
        "DISABLED": palette["disabled"],
        "DISABLED_TEXT": palette["disabledText"],
        "DISABLED_BORDER": palette["disabledBorder"],
        "FOCUS": palette["focus"],
        "HEIGHT": str(metrics["height"]),
        "FONT_SIZE": str(metrics["fontSize"]),
        "RADIUS": str(metrics["radius"]),
    }
    for name, value in values.items():
        template = template.replace(f"@{name}@", value)
    assert "@" not in template, template
    return template


def expected_outputs(components, profiles):
    outputs = {}
    generated = [item for item in components if item["implementation"] == "generated"]
    for item in generated:
        outputs[COMPONENT_DIR / f'{item["sdkClass"]}.h'] = header_text(item)
        outputs[COMPONENT_DIR / f'{item["sdkClass"]}.cpp'] = source_text(item)

    includes = ["#pragma once", "", '#include "../Z_PushButton.h"',
                '#include "../Z_CheckBox.h"']
    includes.extend(f'#include "{item["sdkClass"]}.h"' for item in generated)
    outputs[COMPONENT_DIR / "NativeComponents.h"] = "\n".join(includes) + "\n"
    outputs[COMPONENT_DIR / "NativeComponentRegistry.h"] = registry_header_text()
    outputs[COMPONENT_DIR / "NativeComponentRegistry.cpp"] = registry_source_text(components)

    source_map = {"schemaVersion": 1, "owner": "DesignToCode_GenerateComponent",
                  "components": []}
    for item in components:
        base = ("runtime/component" if item["implementation"] == "specialized"
                else "runtime/component/native")
        style_family = ("button" if item["qtClass"] == "QPushButton" else
                        "checkbox" if item["qtClass"] == "QCheckBox" else "native")
        source_template_path = (f'design-assets/styles/{item["sdkClass"]}.qss.in'
                                if item["implementation"] == "specialized"
                                else f'design-assets/styles/native/{item["sdkClass"]}.qss.in')
        source_map["components"].append({
            "qtClass": item["qtClass"],
            "sdkClass": "commonWidget::" + item["sdkClass"],
            "designComponent": "ComponentLab/" + item["designComponent"],
            "header": f'{base}/{item["sdkClass"]}.h',
            "implementation": f'{base}/{item["sdkClass"]}.cpp',
            "qss": f'design-assets/generated/{style_family}/<profile>/{item["sdkClass"]}.qss',
            "qssTemplate": source_template_path,
            "category": item["category"],
            "implementationKind": item["implementation"],
        })
    outputs[SOURCE_MAP] = json.dumps(source_map, ensure_ascii=False, indent=2) + "\n"

    templates = {}
    for item in components:
        template = qss_template(item)
        templates[item["sdkClass"]] = template
        outputs[STYLE_DIR / f'{item["sdkClass"]}.qss.in'] = template
    for profile_name, profile in profiles.items():
        combined = []
        for item in components:
            rendered = render_qss(templates[item["sdkClass"]], profile)
            outputs[GENERATED_STYLE_DIR / profile_name / f'{item["sdkClass"]}.qss'] = rendered
            combined.append(rendered)
        outputs[GENERATED_STYLE_DIR / profile_name / "native-components.qss"] = "\n".join(combined)
    manifest = {
        "schemaVersion": 1,
        "componentCount": len(components),
        "generatedCppCount": len(generated),
        "profiles": list(profiles),
        "definitionSha256": hashlib.sha256(DEFINITIONS.read_bytes()).hexdigest(),
        "runtimeTokenSha256": hashlib.sha256(RUNTIME_TOKENS.read_bytes()).hexdigest(),
        "namingRule": "Q<Class> -> commonWidget::Z_<Class>",
    }
    outputs[GENERATED_STYLE_DIR / "manifest.json"] = json.dumps(
        manifest, ensure_ascii=False, indent=2
    ) + "\n"
    return outputs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    components, profiles = load_inputs()
    outputs = expected_outputs(components, profiles)
    drift = []
    for path, content in outputs.items():
        if args.check:
            if not path.is_file() or path.read_text(encoding="utf-8") != content:
                drift.append(str(path.relative_to(ROOT)))
            continue
        if path.is_file() and path.read_text(encoding="utf-8") == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
    if drift:
        raise SystemExit("Generated native component drift:\n" + "\n".join(drift))
    print(json.dumps({"components": len(components), "outputs": len(outputs),
                      "mode": "check" if args.check else "write"}))


if __name__ == "__main__":
    main()
