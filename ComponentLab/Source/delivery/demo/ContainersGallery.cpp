#include "ContainersGallery.h"
#include "FigmaGalleryView.h"
#include "FigmaLayoutEvidence.h"
#include "ComponentFactory.h"
#include "MyDesignSystem.h"
#include "Z_PushButton.h"
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStyle>
#include <QVBoxLayout>
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
    int x = 12;
    for (const auto& text : labels) {
        auto* label = new QLabel(text, key);
        label->setGeometry(x, 12, 0, 15);
        label->setStyleSheet(
            "background:transparent;border:0;color:#505868;font:600 12px 'Inter';");
        label->resize(label->fontMetrics().horizontalAdvance(text) + 1, 15);
        x += label->width() + 24;
    }
}

void passive(QWidget* widget, const QString& variant, const QString& state = QString()) {
    widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    widget->setFocusPolicy(Qt::NoFocus);
    widget->setProperty("figmaVariant", variant);
    if (!state.isEmpty())
        widget->setProperty("figmaPreviewState", state);
}

QLabel* panelContent(QWidget* parent, const QString& text) {
    auto* label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color:#191919;background:transparent;");
    return label;
}

void splitterSpecimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                      QWidget* canvas, const QRect& rect, const QString& orientation,
                      const QString& state, bool showGrip) {
    auto* splitter = qobject_cast<commonWidget::Z_Splitter*>(
        factory->create("commonWidget::Z_Splitter", canvas));
    if (!splitter)
        throw std::runtime_error("Z_Splitter unavailable");
    splitter->setGeometry(rect);
    splitter->setOrientation(orientation == "Vertical" ? Qt::Vertical : Qt::Horizontal);
    splitter->setShowGrip(showGrip);
    auto* first = panelContent(splitter, QStringLiteral("Navigation"));
    auto* second = panelContent(splitter, QStringLiteral("Workspace"));
    for (auto* pane : {first, second}) {
        pane->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        pane->setContentsMargins(14, 16, 0, 0);
        pane->setMinimumSize(0, 0);
    }
    splitter->addWidget(first);
    splitter->addWidget(second);
    const bool vertical = orientation == QStringLiteral("Vertical");
    if (state == QStringLiteral("Collapsed"))
        splitter->setSizes(vertical ? QList<int>{24, 160} : QList<int>{24, 336});
    else
        splitter->setSizes(vertical ? QList<int>{72, 112} : QList<int>{142, 218});
    splitter->setSnapshotState(state);
    passive(splitter, QStringLiteral("Orientation=%1, State=%2").arg(orientation, state),
            state == "Hover" || state == "Dragging" ? state : QString());
}

void groupSpecimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                   QWidget* canvas, const QRect& rect, const QString& frame,
                   const QString& mode, const QString& state) {
    auto* group = qobject_cast<commonWidget::Z_GroupBox*>(
        factory->create("commonWidget::Z_GroupBox", canvas));
    if (!group)
        throw std::runtime_error("Z_GroupBox unavailable");
    group->setGeometry(rect);
    group->setObjectName(QStringLiteral("figmaGroupSpecimen"));
    group->setTitle(QStringLiteral("设备联锁"));
    group->setFlat(frame == "Flat");
    if (mode != "Static") {
        group->setCheckable(true);
        group->setChecked(mode == "Checked");
    }
    if (state == "Disabled")
        group->setEnabled(false);
    group->setStyleSheet(state == QStringLiteral("Focus")
                             ? QStringLiteral("QGroupBox#figmaGroupSpecimen {border:2px solid #6d4aff;border-radius:7px;margin-top:10px;} QGroupBox#figmaGroupSpecimen::title {subcontrol-origin:margin;left:10px;padding:0 4px;}")
                             : QStringLiteral("QGroupBox#figmaGroupSpecimen {border:1px solid #d7dbe3;border-radius:7px;margin-top:10px;} QGroupBox#figmaGroupSpecimen::title {subcontrol-origin:margin;left:10px;padding:0 4px;}"));
    auto* layout = new QVBoxLayout(group);
    layout->addWidget(panelContent(group, QStringLiteral("启用联锁保护")));
    passive(group, QStringLiteral("Frame=%1, Mode=%2, State=%3").arg(frame, mode, state),
            state == "Focus" ? state : QString());
}

void scrollSpecimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                    QWidget* canvas, const QRect& rect, const QString& bars,
                    const QString& resize, const QString& state) {
    auto* area = qobject_cast<commonWidget::Z_ScrollArea*>(
        factory->create("commonWidget::Z_ScrollArea", canvas));
    if (!area)
        throw std::runtime_error("Z_ScrollArea unavailable");
    area->setGeometry(rect);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setBars(bars);
    area->setResizeMode(resize);
    auto* content = new QWidget(area);
    content->setObjectName(QStringLiteral("scrollAreaContent"));
    content->setProperty("designSystemClass",
                         QStringLiteral("commonWidget::Z_ScrollAreaContent"));
    content->setProperty("designSystemStyleOwner", QStringLiteral("tokens"));
    content->setAttribute(Qt::WA_StyledBackground, true);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(6);
    auto* title = new QLabel(QStringLiteral("Scrollable content"), content);
    title->setObjectName(QStringLiteral("scrollAreaContentTitle"));
    title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    contentLayout->addWidget(title);
    for (int index = 0; index < 5; ++index) {
        auto* row = new QFrame(content);
        row->setObjectName(QStringLiteral("scrollAreaContentRow"));
        row->setAccessibleName(QStringLiteral("Content placeholder %1").arg(index + 1));
        row->setFixedHeight(index == 0 ? 16 : 12);
        row->setMaximumWidth(index % 2 == 0 ? 292 : 238);
        row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        row->setProperty("designSystemClass",
                         QStringLiteral("commonWidget::Z_ScrollAreaContentSkeleton"));
        row->setProperty("designSystemStyleOwner", QStringLiteral("tokens"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        row->style()->unpolish(row);
        row->style()->polish(row);
        contentLayout->addWidget(row);
    }
    auto* focusTarget = qobject_cast<commonWidget::Z_PushButton*>(
        factory->create("commonWidget::Z_PushButton", content));
    if (!focusTarget)
        throw std::runtime_error("Z_PushButton unavailable for ScrollArea fixture");
    focusTarget->setObjectName(QStringLiteral("scrollAreaFocusTarget"));
    focusTarget->setText(QStringLiteral("Focus target"));
    focusTarget->setType(commonWidget::ButtonType::Default);
    focusTarget->setSize(commonWidget::ButtonSize::S);
    focusTarget->setFixedSize(112, 28);
    focusTarget->setFocusPolicy(Qt::StrongFocus);
    auto* focusTargetRow = new QWidget(content);
    focusTargetRow->setObjectName(QStringLiteral("scrollAreaFocusTargetRow"));
    focusTargetRow->setFixedHeight(28);
    auto* focusTargetLayout = new QHBoxLayout(focusTargetRow);
    focusTargetLayout->setContentsMargins(152, 0, 0, 0);
    focusTargetLayout->setSpacing(0);
    focusTargetLayout->addWidget(focusTarget, 0, Qt::AlignLeft);
    focusTargetLayout->addStretch(1);
    contentLayout->addWidget(focusTargetRow);
    contentLayout->addStretch(1);
    const bool horizontal = bars == QStringLiteral("Horizontal") || bars == QStringLiteral("Both");
    const bool vertical = bars == QStringLiteral("Vertical") || bars == QStringLiteral("Both");
    const QSize contentSize(horizontal ? rect.width() + 96 : rect.width() - 36,
                            vertical ? rect.height() + 96 : rect.height() - 36);
    content->setMinimumSize(contentSize);
    content->resize(contentSize);
    area->setWidget(content);
    area->setObjectName(QStringLiteral("figmaScrollSpecimen"));
    passive(area, QStringLiteral("Bars=%1, Resize=%2, State=%3").arg(bars, resize, state),
            state == "Focus" ? state : QString());
    area->setSnapshotState(state);
}

void widgetSpecimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                    QWidget* canvas, const QRect& rect, const QString& role,
                    const QString& state) {
    auto* host = qobject_cast<commonWidget::Z_Widget*>(
        factory->create("commonWidget::Z_Widget", canvas));
    if (!host)
        throw std::runtime_error("Z_Widget unavailable");
    host->setGeometry(rect);
    host->setObjectName(QStringLiteral("figmaWidgetSpecimen"));
    if (role == "LayoutHost") {
        auto* layout = new QVBoxLayout(host);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->addWidget(panelContent(host, QStringLiteral("子部件 A")));
        layout->addWidget(panelContent(host, QStringLiteral("子部件 B")));
    } else {
        host->setStyleSheet(
            "QWidget#figmaWidgetSpecimen { background:#fafafa; border:1px solid #d0d0d0; "
            "border-radius:8px; }");
    }
    if (state == "Disabled")
        host->setEnabled(false);
    if (state == QStringLiteral("Focus"))
        host->setStyleSheet(QStringLiteral(
            "QWidget#figmaWidgetSpecimen {background:#faf9ff;border:2px solid #6d4aff;border-radius:8px;}"));
    passive(host, QStringLiteral("Role=%1, State=%2").arg(role, state),
            state == "Focus" ? state : QString());
}

void frameSpecimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                   QWidget* canvas, const QRect& rect, const QString& shape,
                   const QString& shadow, const QString& state) {
    auto* frame = qobject_cast<commonWidget::Z_Frame*>(
        factory->create("commonWidget::Z_Frame", canvas));
    if (!frame)
        throw std::runtime_error("Z_Frame unavailable");
    frame->setGeometry(rect);
    frame->setFrameShape(shape == "NoFrame"        ? QFrame::NoFrame
                         : shape == "Box"          ? QFrame::Box
                         : shape == "Panel"        ? QFrame::Panel
                         : shape == "StyledPanel"  ? QFrame::StyledPanel
                         : shape == "HLine"        ? QFrame::HLine
                                                   : QFrame::VLine);
    frame->setFrameShadow(shadow == "Raised"   ? QFrame::Raised
                          : shadow == "Sunken" ? QFrame::Sunken
                                               : QFrame::Plain);
    if (state == "Disabled")
        frame->setEnabled(false);
    passive(frame, QStringLiteral("Shape=%1, Shadow=%2, State=%3").arg(shape, shadow, state));
}

} // namespace

bool hasContainersGallery(const QString& pageId) {
    return pageId == "520:11" || pageId == "526:11" || pageId == "531:11" || pageId == "537:2";
}

QWidget* createContainersGallery(const QString& pageId,
                                 const std::shared_ptr<commonWidget::ComponentFactory>& factory,
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
        if (item.componentSetName.endsWith(QStringLiteral("Splitter"))) {
            splitterSpecimen(factory, canvas, item.rect,
                             figmaAxisValue(item.variant, QStringLiteral("Orientation")),
                             figmaAxisValue(item.variant, QStringLiteral("State")),
                             figmaAxisValue(item.variant, QStringLiteral("Show grip")) !=
                                 QStringLiteral("False"));
        } else if (item.componentSetName.endsWith(QStringLiteral("GroupBox"))) {
            groupSpecimen(factory, canvas, item.rect,
                          figmaAxisValue(item.variant, QStringLiteral("Frame")),
                          figmaAxisValue(item.variant, QStringLiteral("Mode")),
                          figmaAxisValue(item.variant, QStringLiteral("State")));
        } else if (item.componentSetName.endsWith(QStringLiteral("ScrollArea"))) {
            scrollSpecimen(factory, canvas, item.rect,
                           figmaAxisValue(item.variant, QStringLiteral("Bars")),
                           figmaAxisValue(item.variant, QStringLiteral("Resize")),
                           figmaAxisValue(item.variant, QStringLiteral("State")));
        } else if (item.componentSetName.endsWith(QStringLiteral("Widget"))) {
            widgetSpecimen(factory, canvas, item.rect,
                           figmaAxisValue(item.variant, QStringLiteral("Role")),
                           figmaAxisValue(item.variant, QStringLiteral("State")));
        } else if (item.componentSetName.endsWith(QStringLiteral("Frame"))) {
            frameSpecimen(factory, canvas, item.rect,
                          figmaAxisValue(item.variant, QStringLiteral("Shape")),
                          figmaAxisValue(item.variant, QStringLiteral("Shadow")),
                          figmaAxisValue(item.variant, QStringLiteral("State")));
        } else {
            throw std::runtime_error("Unsupported Container component set");
        }
        QWidget* specimen = nullptr;
        for (auto* child : canvas->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (child->property("figmaVariant").toString() == item.variant &&
                !child->property("figmaNodeId").isValid()) {
                specimen = child;
                break;
            }
        }
        if (!specimen)
            throw std::runtime_error("Container specimen was not created");
        specimen->setProperty("figmaComponentSet", item.componentSetName);
        specimen->setProperty("figmaComponentSetId", item.componentSetId);
        specimen->setProperty("figmaNodeId", item.nodeId);
        specimen->setProperty("figmaDesignBounds", item.rect);
    }
    return new FigmaGalleryView(canvas, parent);
}

} // namespace native_gallery
