#include "NativeDisplayGallery.h"
#include "FigmaGalleryView.h"
#include "FigmaLayoutEvidence.h"
#include "ComponentFactory.h"
#include "MyDesignSystem.h"
#include <QCalendarWidget>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QLabel>
#include <QLCDNumber>
#include <QOpenGLWidget>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QTextEdit>
#include <stdexcept>

namespace native_gallery {
namespace {

void keyPanel(QWidget* canvas) {
    auto* key = new QWidget(canvas);
    key->setObjectName("variantKey");
    key->setGeometry((canvas->width() - 743) / 2, 28, 743, 39);
    key->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
    const QStringList labels{QStringLiteral("Assets · 1 groups"),
                             QStringLiteral("Variants · component set / component / frame"),
                             QStringLiteral("States · default / hover / selected / disabled / custom")};
    int tx = 12;
    for (const auto& text : labels) {
        auto* label = new QLabel(text, key);
        label->setGeometry(tx, 12, 0, 15);
        label->setStyleSheet(
            "background:transparent;border:0;color:#505868;font:600 12px 'Inter';");
        label->resize(label->fontMetrics().horizontalAdvance(text) + 1, 15);
        tx += label->width() + 24;
    }
}

void passive(QWidget* widget, const QString& variant, const QString& state = QString()) {
    widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    widget->setFocusPolicy(Qt::NoFocus);
    widget->setProperty("figmaVariant", variant);
    if (!state.isEmpty())
        widget->setProperty("figmaPreviewState", state);
}

commonWidget::Z_LCDNumber* lcdSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& mode, const QString& segment, bool disabled) {
    auto* lcd = qobject_cast<commonWidget::Z_LCDNumber*>(
        factory->create("commonWidget::Z_LCDNumber", canvas));
    if (!lcd)
        throw std::runtime_error("Z_LCDNumber unavailable");
    lcd->setGeometry(rect);
    lcd->setMode(mode == "Hex"    ? QLCDNumber::Hex
                 : mode == "Bin"  ? QLCDNumber::Bin
                 : mode == "Oct"  ? QLCDNumber::Oct
                                  : QLCDNumber::Dec);
    lcd->setSegmentStyle(segment == "Filled" ? QLCDNumber::Filled
                         : segment == "Flat" ? QLCDNumber::Flat
                                             : QLCDNumber::Outline);
    lcd->display(mode == "Dec" ? QStringLiteral("42.7")
                 : mode == "Hex" ? QStringLiteral("ABCD")
                 : mode == "Bin" ? QStringLiteral("1010")
                                 : QStringLiteral("1234"));
    lcd->setEnabled(!disabled);
    passive(lcd, QStringLiteral("Mode=%1, Segment=%2, State=%3")
                     .arg(mode, segment, disabled ? QStringLiteral("Disabled")
                                                  : QStringLiteral("Default")));
    return lcd;
}

commonWidget::Z_TextBrowser* browserSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& content, bool disabled) {
    auto* browser = qobject_cast<commonWidget::Z_TextBrowser*>(
        factory->create("commonWidget::Z_TextBrowser", canvas));
    if (!browser)
        throw std::runtime_error("Z_TextBrowser unavailable");
    browser->setGeometry(rect);
    if (content == "Article")
        browser->setHtml(QStringLiteral("<h3>复核结果</h3><p>参数已通过规则校验，可以提交工单。</p>"));
    else if (content == "Links")
        browser->setHtml(QStringLiteral("<p>参考文档：<a href=\"#guide\">工艺指南</a> · "
                                        "<a href=\"#log\">批次日志</a></p>"));
    else
        browser->setHtml(QStringLiteral("<pre>alarm: none\nstate: running\nbatch: LOT-2026-0930</pre>"));
    browser->setEnabled(!disabled);
    passive(browser, QStringLiteral("Content=%1, State=%2")
                         .arg(content, disabled ? QStringLiteral("Disabled")
                                                : QStringLiteral("Default")));
    return browser;
}

commonWidget::Z_GraphicsView* graphicsSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& mode, bool disabled) {
    auto* view = qobject_cast<commonWidget::Z_GraphicsView*>(
        factory->create("commonWidget::Z_GraphicsView", canvas));
    if (!view)
        throw std::runtime_error("Z_GraphicsView unavailable");
    view->setGeometry(rect);
    auto* scene = new QGraphicsScene(view);
    scene->addRect(8, 8, 200, 90, QPen(QColor("#6d4aff")), QBrush(QColor("#f1edff")));
    scene->addText(QStringLiteral("工艺腔体 ETCH-01"))->setDefaultTextColor(QColor("#191919"));
    view->setScene(scene);
    view->setDragMode(mode == "Pan"       ? QGraphicsView::ScrollHandDrag
                      : mode == "Select"  ? QGraphicsView::RubberBandDrag
                                          : QGraphicsView::NoDrag);
    if (mode == "Passive")
        view->setInteractive(false);
    view->setEnabled(!disabled);
    passive(view, QStringLiteral("Mode=%1, State=%2")
                      .arg(mode, disabled ? QStringLiteral("Disabled") : QStringLiteral("Default")));
    return view;
}

commonWidget::Z_CalendarWidget* calendarSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, bool monday, bool grid, bool disabled) {
    auto* calendar = qobject_cast<commonWidget::Z_CalendarWidget*>(
        factory->create("commonWidget::Z_CalendarWidget", canvas));
    if (!calendar)
        throw std::runtime_error("Z_CalendarWidget unavailable");
    calendar->setGeometry(rect);
    calendar->setWeekStart(monday ? QStringLiteral("Monday") : QStringLiteral("Sunday"));
    calendar->setGrid(grid);
    calendar->setSnapshotState(disabled ? QStringLiteral("Disabled")
                                        : QStringLiteral("Default"));
    passive(calendar, QStringLiteral("WeekStart=%1, Grid=%2, State=%3")
                          .arg(monday ? QStringLiteral("Monday") : QStringLiteral("Sunday"),
                               grid ? QStringLiteral("True") : QStringLiteral("False"),
                               disabled ? QStringLiteral("Disabled") : QStringLiteral("Default")));
    return calendar;
}

commonWidget::Z_TextEdit* richSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& format, const QString& state) {
    auto* edit = qobject_cast<commonWidget::Z_TextEdit*>(
        factory->create("commonWidget::Z_TextEdit", canvas));
    if (!edit)
        throw std::runtime_error("Z_TextEdit unavailable");
    edit->setGeometry(rect);
    if (format == "Rich")
        edit->setHtml(QStringLiteral("<b>工艺备注</b><br>等待设备确认。"));
    else if (format == "Markdown")
        edit->setPlainText(QStringLiteral("# 工艺备注\n\n- [ ] 等待设备确认\n- [x] 参数复核"));
    else
        edit->setPlainText(QStringLiteral("工艺备注：等待设备确认。"));
    edit->setReadOnly(state == "ReadOnly");
    edit->setEnabled(state != "Disabled");
    edit->setObjectName(QStringLiteral("figmaRichTextSpecimen"));
    edit->setStyleSheet(state == QStringLiteral("Focus")
                            ? QStringLiteral("QTextEdit#figmaRichTextSpecimen {border:2px solid #6d4aff;border-radius:6px;background:white;}")
                            : QStringLiteral("QTextEdit#figmaRichTextSpecimen {border:1px solid #d7dbe3;border-radius:6px;background:white;}"));
    passive(edit, QStringLiteral("Format=%1, State=%2").arg(format, state),
            state == "Focus" ? state : QString());
    return edit;
}

commonWidget::Z_PlainTextEdit* plainSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& wrap, const QString& state) {
    auto* edit = qobject_cast<commonWidget::Z_PlainTextEdit*>(
        factory->create("commonWidget::Z_PlainTextEdit", canvas));
    if (!edit)
        throw std::runtime_error("Z_PlainTextEdit unavailable");
    edit->setGeometry(rect);
    edit->setPlainText(QStringLiteral("ALARM: none\nSTATE: running\nBATCH: LOT-2026-0930"));
    edit->setLineWrapMode(wrap == "NoWrap" ? QPlainTextEdit::NoWrap
                                           : QPlainTextEdit::WidgetWidth);
    edit->setReadOnly(state == "ReadOnly");
    edit->setEnabled(state != "Disabled");
    edit->setObjectName(QStringLiteral("figmaPlainTextSpecimen"));
    edit->setStyleSheet(state == QStringLiteral("Focus")
                            ? QStringLiteral("QPlainTextEdit#figmaPlainTextSpecimen {border:2px solid #6d4aff;border-radius:6px;background:white;}")
                            : QStringLiteral("QPlainTextEdit#figmaPlainTextSpecimen {border:1px solid #d7dbe3;border-radius:6px;background:white;}"));
    passive(edit, QStringLiteral("Wrap=%1, State=%2").arg(wrap, state),
            state == "Focus" ? state : QString());
    return edit;
}

QWidget* glSpecimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                    QWidget* canvas, const QRect& rect, const QString& update,
                    const QString& state) {
    if (state == "Unsupported") {
        auto* note = new QWidget(canvas);
        note->setGeometry(rect);
        note->setStyleSheet(
            "background:#f3f3f3;border:1px solid #d0d0d0;border-radius:8px;");
        auto* label = new QLabel(QStringLiteral("OpenGL 不可用（软件回退）"), note);
        label->setAlignment(Qt::AlignCenter);
        label->setGeometry(0, rect.height() / 2 - 12, rect.width(), 24);
        label->setStyleSheet("color:#8a8a8a;border:0;background:transparent;");
        passive(note, QStringLiteral("Update=%1, State=Unsupported").arg(update));
        return note;
    }
    auto* gl = qobject_cast<commonWidget::Z_OpenGLWidget*>(
        factory->create("commonWidget::Z_OpenGLWidget", canvas));
    if (!gl)
        throw std::runtime_error("Z_OpenGLWidget unavailable");
    gl->setGeometry(rect);
    gl->setUpdateBehavior(update == QStringLiteral("Partial") ? QOpenGLWidget::PartialUpdate
                                                               : QOpenGLWidget::NoPartialUpdate);
    gl->setEnabled(state != "Disabled");
    passive(gl, QStringLiteral("Update=%1, State=%2").arg(update, state));
    return gl;
}

} // namespace

bool hasNativeDisplayGallery(const QString& pageId) {
    return pageId == "418:11" || pageId == "420:13" || pageId == "422:16" ||
           pageId == "423:18" || pageId == "426:15" || pageId == "430:20" || pageId == "431:18";
}

QWidget* createNativeDisplayGallery(
    const QString& pageId, const std::shared_ptr<commonWidget::ComponentFactory>& factory,
    QWidget* parent) {
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    const auto evidence = figmaGalleryLayout(pageId);
    canvas->setFixedSize(evidence.size);
    canvas->setProperty("galleryNodeId", evidence.galleryId);
    canvas->setProperty("variantSpecimenCount", evidence.variants.size());
    canvas->setProperty("componentSetCount", evidence.componentSetCount);
    canvas->setProperty("evidenceSource", "/figma/gallery-layouts.json");
    keyPanel(canvas);
    for (const auto& item : evidence.variants) {
        QWidget* specimen = nullptr;
        const QString state = figmaAxisValue(item.variant, QStringLiteral("State"));
        if (item.componentSetName.endsWith(QStringLiteral("LCDNumber"))) {
            specimen = lcdSpecimen(factory, canvas, item.rect,
                                   figmaAxisValue(item.variant, QStringLiteral("Mode")),
                                   figmaAxisValue(item.variant, QStringLiteral("Segment")),
                                   state == QStringLiteral("Disabled"));
        } else if (item.componentSetName.endsWith(QStringLiteral("TextBrowser"))) {
            specimen = browserSpecimen(factory, canvas, item.rect,
                                       figmaAxisValue(item.variant, QStringLiteral("Content")),
                                       state == QStringLiteral("Disabled"));
        } else if (item.componentSetName.endsWith(QStringLiteral("GraphicsView"))) {
            specimen = graphicsSpecimen(factory, canvas, item.rect,
                                        figmaAxisValue(item.variant, QStringLiteral("Mode")),
                                        state == QStringLiteral("Disabled"));
        } else if (item.componentSetName.endsWith(QStringLiteral("Calendar"))) {
            specimen = calendarSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("WeekStart")) ==
                    QStringLiteral("Monday"),
                figmaAxisValue(item.variant, QStringLiteral("Grid")) == QStringLiteral("True"),
                state == QStringLiteral("Disabled"));
        } else if (item.componentSetName.endsWith(QStringLiteral("RichTextEdit"))) {
            specimen = richSpecimen(factory, canvas, item.rect,
                                    figmaAxisValue(item.variant, QStringLiteral("Format")), state);
        } else if (item.componentSetName.endsWith(QStringLiteral("PlainTextEdit"))) {
            specimen = plainSpecimen(factory, canvas, item.rect,
                                     figmaAxisValue(item.variant, QStringLiteral("Wrap")), state);
        } else if (item.componentSetName.endsWith(QStringLiteral("OpenGLViewport"))) {
            specimen = glSpecimen(factory, canvas, item.rect,
                                  figmaAxisValue(item.variant, QStringLiteral("Update")), state);
        }
        if (!specimen)
            throw std::runtime_error("Unsupported Native Display component set");
        specimen->setProperty("figmaVariant", item.variant);
        specimen->setProperty("figmaComponentSet", item.componentSetName);
        specimen->setProperty("figmaComponentSetId", item.componentSetId);
        specimen->setProperty("figmaNodeId", item.nodeId);
        specimen->setProperty("figmaDesignBounds", item.rect);
    }
    return new FigmaGalleryView(canvas, parent);
}

} // namespace native_gallery
