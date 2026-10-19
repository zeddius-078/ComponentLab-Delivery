#include "DisplayGallery.h"
#include "LabelAppearance.h"
#include "ProgressAppearance.h"
#include "SpinBoxAppearance.h"
#include "DateTimeAppearance.h"
#include "SliderAppearance.h"
#include "ScrollBarAppearance.h"
#include "DialAppearance.h"
#include "LineEditAppearance.h"
#include "ShortcutAppearance.h"
#include "ComboBoxAppearance.h"
#include <QFontComboBox>
#include <QKeySequenceEdit>
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProgressBar>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QTimeEdit>
#include <QSlider>
#include <QScrollBar>
#include <QDial>
#include <QLineEdit>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& node) {
    return QRect(qRound(node[3].toDouble()), qRound(node[4].toDouble()), qRound(node[5].toDouble()),
                 qRound(node[6].toDouble()));
}
} // namespace
QWidget* createDisplayGallery(const QString& pageId,
                              const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                              QWidget* parent) {
    const bool numeric = pageId == "458:2" || pageId == "460:2";
    const bool dateTime = pageId == "463:2" || pageId == "464:2" || pageId == "465:2";
    const bool sliderPage = pageId == "467:2";
    const bool scrollPage = pageId == "471:334";
    const bool dialPage = pageId == "475:334";
    const bool linePage = pageId == "1116:2";
    const bool shortcutPage = pageId == "478:337";
    const bool comboPage = pageId == "1120:2" || pageId == "1121:2";
    if (pageId != "414:7" && pageId != "410:13" && !numeric && !dateTime && !sliderPage &&
        !scrollPage && !dialPage && !linePage && !shortcutPage && !comboPage)
        throw std::invalid_argument("Unsupported authored display gallery");
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Figma gallery evidence missing");
    QJsonArray gallery;
    for (const auto& value : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray()) {
        const auto page = value.toObject();
        if (page["id"] == pageId)
            gallery = page["galleries"].toArray()[0].toArray();
    }
    if (gallery.isEmpty())
        throw std::runtime_error("Display gallery evidence missing");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", gallery[0].toString());
    canvas->setProperty("variantSpecimenCount", dateTime || sliderPage || scrollPage || dialPage ? 8
                                                : numeric           ? 16
                                                : pageId == "414:7" ? 12
                                                                    : 20);
    if (linePage)
        canvas->setProperty("variantSpecimenCount", 18);
    if (shortcutPage)
        canvas->setProperty("variantSpecimenCount", 6);
    if (comboPage)
        canvas->setProperty("variantSpecimenCount", 12);
    canvas->setProperty("componentSetCount", 1);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    for (const auto& child : gallery[7].toArray()) {
        const auto node = child.toArray();
        auto* group = new QWidget(canvas);
        group->setGeometry(bounds(node));
        if (node[1] == "Variant key") {
            group->setObjectName("variantKey");
            group->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            for (const auto& entry : node[7].toArray()) {
                const auto key = entry.toArray();
                auto* label = new QLabel(key[7].toString(), group);
                label->setGeometry(bounds(key));
                label->setStyleSheet("background:transparent;border:0;color:#505868;"
                                     "font-family:'HarmonyOS Sans';font-size:12px;");
                QFont keyFont("HarmonyOS Sans");
                keyFont.setPixelSize(12);
                keyFont.setWeight(QFont::DemiBold);
                label->setFont(keyFont);
            }
            continue;
        }
        group->setProperty("figmaComponentSetId", node[0].toString());
        if (numeric || dateTime || sliderPage) {
            group->setObjectName("numericComponentSet");
            group->setStyleSheet(
                "QWidget#numericComponentSet {background:#f7f9fc;border-radius:16px;}");
        }
        for (const auto& entry : node[7].toArray()) {
            const auto variant = entry.toArray();
            const auto name = variant[1].toString();
            auto* widget = factory->create(pageId == "414:7"    ? "commonWidget::Z_Label"
                                           : pageId == "458:2"  ? "commonWidget::Z_SpinBox"
                                           : pageId == "460:2"  ? "commonWidget::Z_DoubleSpinBox"
                                           : pageId == "463:2"  ? "commonWidget::Z_DateTimeEdit"
                                           : pageId == "464:2"  ? "commonWidget::Z_DateEdit"
                                           : pageId == "465:2"  ? "commonWidget::Z_TimeEdit"
                                           : sliderPage         ? "commonWidget::Z_Slider"
                                           : scrollPage         ? "commonWidget::Z_ScrollBar"
                                           : dialPage           ? "commonWidget::Z_Dial"
                                           : linePage           ? "commonWidget::Z_LineEdit"
                                           : shortcutPage       ? "commonWidget::Z_KeySequenceEdit"
                                           : pageId == "1120:2" ? "commonWidget::Z_ComboBox"
                                           : pageId == "1121:2" ? "commonWidget::Z_FontComboBox"
                                                                : "commonWidget::Z_ProgressBar",
                                           group);
            if (!widget)
                throw std::runtime_error("Display component unavailable");
            if (auto* label = qobject_cast<QLabel*>(widget)) {
                commonWidget::configureFigmaLabel(
                    label,
                    {{QStringLiteral("Alignment"), name.section(',', 0, 0).section('=', 1)},
                     {QStringLiteral("Wrap"), name.contains("Wrap=True") ? QStringLiteral("True")
                                                                         : QStringLiteral("False")},
                     {QStringLiteral("State"), name.section(", ", 2, 2).section('=', 1)}});
                label->setText(QString::fromUtf8("设备状态：运行中"));
                label->setTextFormat(Qt::PlainText);
            } else if (auto* progress = qobject_cast<QProgressBar*>(widget)) {
                const auto state = name.section(", ", 1, 1).section('=', 1);
                progress->setRange(0, 100);
                progress->setValue(68);
                progress->setFormat(state == "In-progress" ? "In progress"
                                    : QStringList{"Paused", "Error", "Completed"}.contains(state)
                                        ? state
                                        : "%p%");
                commonWidget::configureFigmaProgressBar(
                    progress,
                    {{QStringLiteral("Orientation"),
                      name.contains("Orientation=Vertical") ? QStringLiteral("Vertical")
                                                              : QStringLiteral("Horizontal")},
                     {QStringLiteral("State"), state},
                     {QStringLiteral("Inverted"),
                      name.contains("Inverted=True") ? QStringLiteral("True")
                                                       : QStringLiteral("False")},
                     {QStringLiteral("Show label"), QStringLiteral("True")}},
                    true);
            } else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
                if (auto* fonts = qobject_cast<QFontComboBox*>(combo)) {
                    fonts->setCurrentFont(QFont("Arial"));
                    fonts->setEditable(false); // Inert closed specimen; live stays native editable.
                } else {
                    combo->setPlaceholderText("Please select");
                    combo->setCurrentIndex(-1);
                }
                const auto sourceState = name.section(", ", 1, 1).section('=', 1);
                commonWidget::applyFigmaComboBoxAppearance(
                    combo, name.section(',', 0, 0).section('=', 1),
                    sourceState == "Disabled" ? "Default" : sourceState, true);
                combo->setFocusPolicy(Qt::NoFocus);
                combo->setAttribute(Qt::WA_TransparentForMouseEvents);
            } else if (auto* shortcut = qobject_cast<QKeySequenceEdit*>(widget)) {
                const auto sourceState = name.section('=', 1);
                shortcut->setKeySequence(sourceState == "Empty" ? QKeySequence()
                                                                : QKeySequence("Ctrl+Shift+P"));
                commonWidget::applyFigmaShortcutAppearance(
                    shortcut, sourceState == "Disabled" ? "Default" : sourceState, true, true,
                    true);
                shortcut->setFocusPolicy(Qt::NoFocus);
                shortcut->setAttribute(Qt::WA_TransparentForMouseEvents);
                for (auto* shortcutChild : shortcut->findChildren<QWidget*>())
                    shortcutChild->setFocusPolicy(Qt::NoFocus);
            } else if (auto* line = qobject_cast<QLineEdit*>(widget)) {
                const auto sourceState = name.section(", ", 1, 1).section('=', 1);
                line->setPlaceholderText("Input text");
                line->setText(sourceState == "Filled" ? "Entered value" : "");
                commonWidget::configureFigmaLineEdit(
                    line, {{"Size", name.section(',', 0, 0).section('=', 1)},
                           {"State", sourceState},
                           {"Allow clear", "False"},
                           {"Show prefix", "False"},
                           {"Show suffix", "False"}});
                line->setFocusPolicy(Qt::NoFocus);
                line->setAttribute(Qt::WA_TransparentForMouseEvents);
            } else if (auto* dial = qobject_cast<QDial*>(widget)) {
                dial->setRange(0, 100);
                dial->setValue(40);
                dial->setWrapping(name.contains("Wrapping=On"));
                dial->setNotchesVisible(true);
                commonWidget::applyFigmaDialAppearance(dial,
                                                       name.contains("State=Focus") ? "Focus"
                                                       : name.contains("State=Dragging")
                                                           ? "Dragging"
                                                           : "Default",
                                                       true, true, "40");
                dial->setProperty(
                    "figmaNativeValueAuthority",
                    "explicit demo fixture; source label and geometry are independent");
                dial->setFocusPolicy(Qt::NoFocus);
                dial->setAttribute(Qt::WA_TransparentForMouseEvents);
                dial->setToolTip(QString::fromUtf8("源稿角度快照；外观页保留 Qt 原生角度与交互。"));
            } else if (auto* bar = qobject_cast<QScrollBar*>(widget)) {
                bar->setRange(0, 100);
                bar->setValue(40);
                bar->setPageStep(20);
                commonWidget::configureFigmaScrollBar(
                    bar, {{"Orientation", name.contains("Orientation=Vertical") ? "Vertical"
                                                                                  : "Horizontal"},
                          {"State", name.section(", ", 1, 1).section('=', 1)},
                          {"Show line buttons", "True"}},
                    true);
                bar->setProperty("figmaNativeValueAuthority",
                                 "explicit demo fixture; not inferred from source thumb geometry");
                bar->setFocusPolicy(Qt::NoFocus);
                bar->setAttribute(Qt::WA_TransparentForMouseEvents);
                bar->setToolTip(QString::fromUtf8("源稿几何快照；请在外观页交互调参。"));
            } else if (auto* slider = qobject_cast<QSlider*>(widget)) {
                slider->setRange(0, 100);
                slider->setValue(40);
                slider->setTickInterval(10);
                commonWidget::configureFigmaSlider(
                    slider, {{"Orientation", name.contains("Orientation=Vertical") ? "Vertical"
                                                                                     : "Horizontal"},
                             {"State", name.section(", ", 1, 1).section('=', 1)},
                             {"Show ticks", "True"},
                             {"Show value", "True"},
                             {"Value label", "40"}},
                    true);
                slider->setProperty(
                    "figmaNativeValueAuthority",
                    "explicit demo fixture; not inferred from Figma label or pixels");
                slider->setFocusPolicy(Qt::NoFocus);
                slider->setAttribute(Qt::WA_TransparentForMouseEvents);
                slider->setToolTip(QString::fromUtf8("源稿几何快照；请在外观页交互调参。"));
            } else if (auto* edit = qobject_cast<QDateTimeEdit*>(widget)) {
                edit->setLocale(QLocale::c());
                const bool timeOnly = pageId == "465:2";
                const bool twelveHour = name.contains("Format=12h");
                if(!timeOnly) edit->setDisplayFormat(pageId == "464:2"
                                           ? "yyyy-MM-dd"
                                           : QString::fromUtf8("yyyy-MM-dd '·' HH:mm"));
                if (timeOnly)
                    edit->setTime(QTime(twelveHour ? 14 : 2, 30, 45));
                else
                    edit->setDateTime(QDateTime(QDate(2026, 10, 7), QTime(14, 30)));
                edit->setCalendarPopup(name.contains("Calendar=Available"));
                if(timeOnly)
                    commonWidget::configureFigmaTimePicker(
                        qobject_cast<QTimeEdit*>(edit),
                        {{"Format", twelveHour ? "12h" : "24h"},
                         {"State", name.section(", ", 1, 1).section('=', 1)},
                         {"Show seconds", "True"}});
                else {
                    edit->setReadOnly(name.contains("State=ReadOnly"));
                    commonWidget::applyFigmaDateTimeAppearance(
                        edit, name.contains("State=Focus") ? "Focus" : "Default",
                        pageId == "463:2" ? " CST" : "");
                }
            } else if (auto* spin = qobject_cast<QAbstractSpinBox*>(widget)) {
                spin->setLocale(QLocale::c());
                if (auto* integer = qobject_cast<QSpinBox*>(spin)) {
                    integer->setRange(0, 1000);
                    integer->setValue(128);
                    integer->setPrefix(QString::fromUtf8("¥"));
                    integer->setSuffix(" units");
                } else if (auto* decimal = qobject_cast<QDoubleSpinBox*>(spin)) {
                    decimal->setRange(0, 100);
                    decimal->setDecimals(2);
                    decimal->setValue(72.5);
                    decimal->setPrefix("$");
                    decimal->setSuffix(" %");
                }
                spin->setReadOnly(name.contains("State=ReadOnly"));
                commonWidget::applyFigmaSpinBoxAppearance(
                    spin, name.section(',', 0, 0).section('=', 1),
                    name.contains("State=Focus") ? "Focus" : "Default");
            }
            widget->setProperty("figmaNodeId", variant[0].toString());
            widget->setProperty("figmaVariant", name);
            widget->setProperty("figmaDesignBounds", bounds(variant));
            widget->setFixedSize(bounds(variant).size());
            widget->move(bounds(variant).topLeft());
            widget->setEnabled(!name.contains("State=Disabled"));
        }
    }
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
