#include "DateTimeAppearance.h"
#include <QDateTimeEdit>
#include <QEvent>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QSignalBlocker>
#include <QStyleFactory>
#include <QStyleOptionSpinBox>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
bool sourceFocus(const QDateTimeEdit* edit) {
    if (!edit->isEnabled() || edit->isReadOnly())
        return false;
    const auto state = edit->property("figmaDateSnapshotState").toString();
    return state.isEmpty() ? edit->hasFocus() : state == "Focus";
}

bool hasDate(const QDateTimeEdit* edit) {
    return (edit->displayedSections() & QDateTimeEdit::DateSections_Mask) != 0;
}

bool calendarAvailable(const QDateTimeEdit* edit) {
    return hasDate(edit) && edit->calendarPopup();
}

int borderWidth(const QDateTimeEdit* edit) {
    return sourceFocus(edit) ? 2 : 1;
}

QRectF dateVisualRect(const QDateTimeEdit* edit, QRectF rect) {
    if (edit->layoutDirection() == Qt::RightToLeft)
        rect.moveLeft(edit->width() - rect.right());
    return rect;
}

QRectF controlRect(const QDateTimeEdit* edit) {
    if (edit->buttonSymbols() == QAbstractSpinBox::NoButtons)
        return {};
    return dateVisualRect(edit,
                          QRectF(edit->width() - 42 + borderWidth(edit), 0, 42, edit->height()));
}

QRectF editorRect(const QDateTimeEdit* edit) {
    const int controlWidth = edit->buttonSymbols() == QAbstractSpinBox::NoButtons ? 2 : 42;
    return dateVisualRect(edit, QRectF(borderWidth(edit) + 10, 1, edit->width() - controlWidth - 18,
                                       edit->height() - 2));
}

QFont sourceFont(int size = 14, int weight = QFont::Medium) {
    QFont font;
    font.setFamilies({"Inter", "Noto Sans SC"});
    font.setPixelSize(size);
    font.setWeight(weight);
    return font;
}

bool authoredFormat(const QDateTimeEdit* edit) {
    return QStringList{"yyyy-MM-dd",  QString::fromUtf8("yyyy-MM-dd '·' HH:mm"),
                       "HH:mm:ss",    "HH:mm",
                       "hh:mm:ss AP", "hh:mm AP"}
        .contains(edit->displayFormat());
}

class DateStyle final : public QProxyStyle {
public:
    explicit DateStyle(QObject* owner) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(owner);
    }

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl sub,
                         const QWidget* widget) const override {
        const auto* edit = qobject_cast<const QDateTimeEdit*>(widget);
        if (!edit || (control != CC_SpinBox && control != CC_ComboBox))
            return QProxyStyle::subControlRect(control, option, sub, widget);
        const auto slot = controlRect(edit);
        if (control == CC_ComboBox && sub == SC_ComboBoxArrow)
            return slot.toRect();
        if (control == CC_SpinBox && (sub == SC_SpinBoxUp || sub == SC_SpinBoxDown))
            return slot.isEmpty()
                       ? QRect()
                       : QRect(qRound(slot.x()), sub == SC_SpinBoxUp ? 0 : edit->height() / 2, 42,
                               edit->height() / 2);
        if ((control == CC_ComboBox && sub == SC_ComboBoxEditField) ||
            (control == CC_SpinBox && sub == SC_SpinBoxEditField))
            return editorRect(edit).toRect();
        return edit->rect();
    }

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                                     const QPoint& point, const QWidget* widget) const override {
        if (!qobject_cast<const QDateTimeEdit*>(widget) ||
            (control != CC_SpinBox && control != CC_ComboBox))
            return QProxyStyle::hitTestComplexControl(control, option, point, widget);
        if (control == CC_ComboBox)
            return subControlRect(control, option, SC_ComboBoxArrow, widget).contains(point)
                       ? SC_ComboBoxArrow
                       : SC_ComboBoxEditField;
        for (const auto sub : {SC_SpinBoxUp, SC_SpinBoxDown})
            if (subControlRect(control, option, sub, widget).contains(point))
                return sub;
        return SC_SpinBoxEditField;
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                            QPainter* painter, const QWidget* widget) const override {
        const auto* edit = qobject_cast<const QDateTimeEdit*>(widget);
        if (!edit || (control != CC_SpinBox && control != CC_ComboBox)) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const QRectF root(edit->rect());
        QPainterPath clip;
        clip.addRoundedRect(root, 4, 4);
        painter->setClipPath(clip);
        painter->fillPath(clip,
                          QColor(!edit->isEnabled() || edit->isReadOnly() ? "#f3f3f3" : "#ffffff"));
        const auto slot = controlRect(edit);
        if (!slot.isEmpty()) {
            painter->fillRect(slot, QColor("#f3f3f3"));
            const bool available = edit->isEnabled() && !edit->isReadOnly();
            if (calendarAvailable(edit)) {
                QSvgRenderer svg(QString(":/figma/datetime/Calendar%1.svg")
                                     .arg(available ? "Primary" : "Disabled"));
                svg.render(painter, QRectF(slot.x() + 13, 8, 16, 16));
            } else {
                const auto* spinOption = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
                for (const bool up : {true, false}) {
                    const bool stepEnabled =
                        available && spinOption &&
                        (spinOption->stepEnabled & (up ? QAbstractSpinBox::StepUpEnabled
                                                       : QAbstractSpinBox::StepDownEnabled));
                    QSvgRenderer svg(
                        QString(":/figma/spinbox/%1%2.svg")
                            .arg(up ? "Up" : "Down", stepEnabled ? "Primary" : "Disabled"));
                    // The source instance is 12px, but its original SVG child remains
                    // 16px at (0,0): preserve that overflow, do not scale the glyph.
                    svg.render(painter,
                               QRectF(slot.x() + 15, (up ? 0 : (edit->height() + 1) / 2.0) + 1.75,
                                      16, 16));
                }
                painter->fillRect(QRectF(slot.x(), (edit->height() - 1) / 2.0, 42, 1),
                                  QColor("#d0d0d0"));
            }
        }
        const auto* line = edit->findChild<QLineEdit*>();
        if (sourceFocus(edit) && authoredFormat(edit) && line && !line->hasFocus() &&
            !line->hasSelectedText()) {
            const qreal width =
                hasDate(edit) ? (qobject_cast<const QDateEdit*>(edit) ? 102 : 91) : 28;
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#f3f3f3"));
            painter->drawRoundedRect(
                dateVisualRect(edit, QRectF(borderWidth(edit) + 10, 2, width, 28)), 5, 5);
        }
        const qreal border = borderWidth(edit);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(!edit->isEnabled()  ? "#d0d0d0"
                                    : sourceFocus(edit) ? "#6235f7"
                                                        : "#a7a7a7"),
                             border));
        painter->drawRoundedRect(root.adjusted(border / 2, border / 2, -border / 2, -border / 2),
                                 4 - border / 2, 4 - border / 2);
        painter->restore();
    }
};

class DateBinding final : public QObject {
public:
    explicit DateBinding(QDateTimeEdit* edit)
        : QObject(edit), edit_(edit), line_(edit->findChild<QLineEdit*>()) {
        setObjectName("figmaDateBinding");
        edit_->installEventFilter(this);
        line_->installEventFilter(this);
        QObject::connect(line_, &QLineEdit::textChanged, this, [this] {
            refresh();
        });
        QObject::connect(line_, &QLineEdit::selectionChanged, this, [this] {
            line_->update();
            edit_->update();
        });
    }

    void refresh() {
        if (edit_->minimumHeight() != 32 || edit_->maximumHeight() != 32)
            edit_->setFixedHeight(32);
        line_->setGeometry(editorRect(edit_).toRect());
        const auto slot = controlRect(edit_);
        edit_->setProperty("figmaDateCalendarAvailable", calendarAvailable(edit_));
        edit_->setProperty("figmaDateAuthoredFormat", authoredFormat(edit_));
        edit_->setProperty("figmaDateCalendarAssetBounds", QRectF(slot.x() + 13, 8, 16, 16));
        edit_->setProperty("figmaDateUpAssetBounds", QRectF(slot.x() + 15, 1.75, 16, 16));
        edit_->setProperty("figmaDateDownAssetBounds", QRectF(slot.x() + 15, 18.25, 16, 16));
        edit_->update();
        line_->update();
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut ||
            event->type() == QEvent::Resize ||
            (object == edit_ && (event->type() == QEvent::StyleChange ||
                                 event->type() == QEvent::LayoutDirectionChange ||
                                 event->type() == QEvent::EnabledChange)))
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
        if (object != line_ || event->type() != QEvent::Paint || line_->hasFocus() ||
            line_->hasSelectedText() || !authoredFormat(edit_))
            return false;
        QPainter painter(line_);
        painter.setRenderHint(QPainter::TextAntialiasing);
        const QColor primary(edit_->isEnabled() ? "#191919" : "#8a8a8a"),
            secondary(edit_->isEnabled() ? "#444444" : "#8a8a8a");
        auto draw = [&](const QString& text, qreal x, qreal width, const QFont& font,
                        const QColor& color, bool centered = false) {
            if (edit_->layoutDirection() == Qt::RightToLeft)
                x = line_->width() - x - width;
            painter.setFont(font);
            painter.setPen(color);
            painter.drawText(QRectF(x, 0, width, line_->height()),
                             Qt::AlignVCenter | Qt::TextSingleLine |
                                 (centered ? Qt::AlignHCenter : Qt::AlignLeft),
                             text);
        };
        const auto numeric = sourceFont(), separator = sourceFont(13, QFont::Normal);
        const auto section = [&](QDateTimeEdit::Section value) {
            return edit_->sectionText(value);
        };
        if (hasDate(edit_)) {
            const qreal dateWidth = qobject_cast<QDateEdit*>(edit_) ? 102 : 91;
            draw(section(QDateTimeEdit::YearSection) + "-" + section(QDateTimeEdit::MonthSection) +
                     "-" + section(QDateTimeEdit::DaySection),
                 0, dateWidth, numeric, primary, true);
            if (edit_->displayedSections() & QDateTimeEdit::HourSection) {
                draw(QString::fromUtf8("·"), 96, 4, separator, primary);
                const auto time = section(QDateTimeEdit::HourSection) + ":" +
                                  section(QDateTimeEdit::MinuteSection);
                const qreal timeWidth = qRound(QFontMetricsF(numeric).horizontalAdvance(time));
                draw(time, 105, timeWidth, numeric, primary);
                const auto annotation = edit_->property("figmaDatePreviewTimeSpec").toString();
                if (!annotation.isEmpty())
                    draw(annotation, 110 + timeWidth,
                         qRound(QFontMetricsF(sourceFont(12, QFont::Normal))
                                    .horizontalAdvance(annotation)),
                         sourceFont(12, QFont::Normal), secondary);
            }
        } else {
            draw(section(QDateTimeEdit::HourSection), 0, 28, numeric, primary, true);
            draw(":", 31, 4, separator, primary);
            const auto minutes = section(QDateTimeEdit::MinuteSection);
            qreal x = 38;
            const qreal minuteWidth = qRound(QFontMetricsF(numeric).horizontalAdvance(minutes));
            draw(minutes, x, minuteWidth, numeric, primary);
            x += minuteWidth + 3;
            if (edit_->displayedSections() & QDateTimeEdit::SecondSection) {
                draw(":", x, 4, separator, primary);
                x += 7;
                const auto seconds = section(QDateTimeEdit::SecondSection);
                const qreal width = qRound(QFontMetricsF(numeric).horizontalAdvance(seconds));
                draw(seconds, x, width, numeric, primary);
                x += width + 3;
            }
            if (edit_->displayedSections() & QDateTimeEdit::AmPmSection) {
                const auto ampm = " " + section(QDateTimeEdit::AmPmSection);
                const auto font = sourceFont(12, QFont::DemiBold);
                draw(ampm, x, qRound(QFontMetricsF(font).horizontalAdvance(ampm)), font, secondary);
            }
        }
        return true;
    }

private:
    QDateTimeEdit* edit_;
    QLineEdit* line_;
};
}

void applyFigmaDateTimeAppearance(QDateTimeEdit* edit, const QString& snapshotState,
                                  const QString& previewTimeSpec) {
    if (!edit || !QStringList{"", "Default", "Focus"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured date/time requires a native QDateTimeEdit and source-backed snapshot state");
    static const bool fonts = [] {
        for (const auto* name : {"Inter-Regular.otf", "Inter-Medium.otf", "Inter-SemiBold.otf",
                                 "NotoSansSC-Regular.otf"})
            QFontDatabase::addApplicationFont(QString(":/figma/fonts/") + name);
        return true;
    }();
    Q_UNUSED(fonts);
    edit->setProperty("figmaAppearance", "figma-light");
    edit->setProperty("designSystemStyleOwner", "figma-date-captured");
    edit->setProperty("figmaDateSnapshotState", snapshotState);
    edit->setProperty("figmaDatePreviewTimeSpec", previewTimeSpec);
    auto* binding = dynamic_cast<DateBinding*>(
        edit->findChild<QObject*>("figmaDateBinding", Qt::FindDirectChildrenOnly));
    if (!binding) {
        edit->setStyle(new DateStyle(edit));
        edit->setStyleSheet(
            "QDateTimeEdit {font-family:'Inter','Noto Sans SC';font-size:14px;min-height:0;}"
            "QLineEdit "
            "{background:transparent;border:0;padding:0;min-height:0;font-family:'Inter','Noto "
            "Sans SC';font-size:14px;color:#191919;"
            "selection-background-color:#f3f3f3;selection-color:#191919;} QLineEdit:disabled "
            "{color:#8a8a8a;}");
        edit->ensurePolished();
        edit->findChild<QLineEdit*>()->setFont(sourceFont());
        binding = new DateBinding(edit);
    }
    edit->setFixedHeight(32);
    binding->refresh();
}

void setDateTimeDisplayFormatPreservingValue(QDateTimeEdit* edit, const QString& format) {
    if (!edit)
        throw std::invalid_argument("Display format requires a native date/time editor");
    const auto value = edit->dateTime();
    const auto minimum = edit->minimumDateTime(), maximum = edit->maximumDateTime();
    const auto section = edit->currentSection();
    const QSignalBlocker blocker(edit);
    edit->setDisplayFormat(format);
    edit->setDateTimeRange(minimum, maximum);
    edit->setDateTime(value);
    if (edit->displayedSections() & section)
        edit->setCurrentSection(section);
}
}
