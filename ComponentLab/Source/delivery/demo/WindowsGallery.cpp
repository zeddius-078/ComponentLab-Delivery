#include "WindowsGallery.h"
#include "FigmaGalleryView.h"
#include "FigmaLayoutEvidence.h"
#include "ComponentFactory.h"
#include "MyDesignSystem.h"
#include <QColorDialog>
#include <QDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStatusBar>
#include <QVBoxLayout>
#include <stdexcept>

namespace native_gallery {
namespace {

QWidget* keyPanel(QWidget* canvas) {
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
    return key;
}

void passive(QWidget* widget, const QString& variant) {
    widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    widget->setFocusPolicy(Qt::NoFocus);
    widget->setProperty("figmaVariant", variant);
}

commonWidget::Z_StatusBar* statusSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& scenario, bool grip) {
    auto* bar = qobject_cast<commonWidget::Z_StatusBar*>(
        factory->create("commonWidget::Z_StatusBar", canvas));
    if (!bar)
        throw std::runtime_error("Z_StatusBar unavailable");
    bar->setGeometry(rect);
    bar->setFixedSize(rect.size());
    auto* permanent = new QLabel(QStringLiteral("READY · LOT-2026-0930"), bar);
    permanent->setStyleSheet("color:#444444;background:transparent;");
    if (scenario == "Temporary") {
        bar->showMessage(QStringLiteral("正在同步批次 LOT-2026-0930…"));
    } else if (scenario == "Composite") {
        bar->showMessage(QStringLiteral("同步中"));
        auto* progress = new QProgressBar(bar);
        progress->setRange(0, 100);
        progress->setValue(64);
        progress->setFixedSize(96, 6);
        progress->setTextVisible(false);
        bar->addPermanentWidget(progress);
    } else {
        auto* indicator = new QLabel(QStringLiteral("设备连接正常"), bar);
        indicator->setStyleSheet("color:#191919;background:transparent;");
        bar->addWidget(indicator);
    }
    bar->addPermanentWidget(permanent);
    bar->setSizeGripEnabled(grip);
    passive(bar, QStringLiteral("Scenario=%1, Grip=%2")
                     .arg(scenario, grip ? QStringLiteral("Shown") : QStringLiteral("Hidden")));
    return bar;
}

commonWidget::Z_Dialog* dialogSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& modality, const QString& size, bool focus) {
    auto* dialog = qobject_cast<commonWidget::Z_Dialog*>(
        factory->create("commonWidget::Z_Dialog", canvas));
    if (!dialog)
        throw std::runtime_error("Z_Dialog unavailable");
    dialog->setWindowFlags(Qt::Widget);
    dialog->setGeometry(rect);
    dialog->setFixedSize(rect.size());
    dialog->setWindowTitle(QStringLiteral("工单复核"));
    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->addWidget(new QLabel(QStringLiteral("确认提交当前工单参数？"), dialog));
    if (size == QStringLiteral("Medium")) {
        auto* detail = new QLabel(QStringLiteral("批次 LOT-2026-0930 · 设备 ETCH-01"), dialog);
        detail->setStyleSheet(QStringLiteral("color:#697180;"));
        layout->addWidget(detail);
    }
    layout->addStretch(1);
    auto* buttons = new commonWidget::Z_DialogButtonBox(dialog);
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    dialog->setWindowModality(modality == "WindowModal"   ? Qt::WindowModal
                              : modality == "ApplicationModal" ? Qt::ApplicationModal
                                                               : Qt::NonModal);
    dialog->setProperty("figmaPreviewState", focus ? "Focus" : "Default");
    dialog->setProperty("figmaSize", size);
    dialog->setStyleSheet(focus
                              ? QStringLiteral("commonWidget--Z_Dialog {border:2px solid #6d4aff;background:white;}")
                              : QStringLiteral("commonWidget--Z_Dialog {border:1px solid #d7dbe3;background:white;}"));
    passive(dialog, QStringLiteral("Modality=%1, Size=%2, State=%3")
                        .arg(modality, size,
                             focus ? QStringLiteral("Focus") : QStringLiteral("Default")));
    return dialog;
}

QWidget* messageSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& icon, bool details) {
    // QMessageBox is lifecycle-owned: embedding a visible QMessageBox in a graphics proxy
    // triggers its showEvent and is unsafe. Keep a configured, hidden real Z_MessageBox as
    // runtime type evidence and render a clearly marked passive anatomy specimen.
    auto* runtime = qobject_cast<commonWidget::Z_MessageBox*>(
        factory->create("commonWidget::Z_MessageBox", canvas));
    if (!runtime)
        throw std::runtime_error("Z_MessageBox unavailable");
    runtime->setObjectName(QStringLiteral("messageBoxRuntimeContract"));
    runtime->setIcon(icon == "Information"   ? QMessageBox::Information
                     : icon == "Question"    ? QMessageBox::Question
                     : icon == "Warning"     ? QMessageBox::Warning
                     : icon == "Critical"    ? QMessageBox::Critical
                                              : QMessageBox::NoIcon);
    runtime->setText(QStringLiteral("工单复核完成，参数已通过规则校验。"));
    runtime->setDetailedText(details ? QStringLiteral("压力：1.2 Pa\n温度：21.4 °C") : QString());
    runtime->setProperty("figmaVariant", QStringLiteral("Icon=%1, Details=%2")
                                             .arg(icon, details ? QStringLiteral("Shown")
                                                                : QStringLiteral("Hidden")));
    runtime->setProperty("lifecycleBoundary", true);
    runtime->hide();

    auto* box = new QWidget(canvas);
    box->setObjectName(QStringLiteral("messageDialogAnatomy"));
    box->setGeometry(rect);
    box->setFixedSize(rect.size());
    box->setStyleSheet(QStringLiteral(
        "QWidget#messageDialogAnatomy {background:white;border:1px solid #d7dbe3;border-radius:8px;}"));
    box->setProperty("figmaComponentIdentity", QStringLiteral("ComponentLab/MessageDialog"));
    box->setProperty("runtimeClass", QStringLiteral("commonWidget::Z_MessageBox"));
    box->setProperty("nativeSurfaceBoundary", true);
    box->setProperty("specimenKind", QStringLiteral("anatomy-boundary"));
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(16, 16, 16, 16);
    auto* row = new QHBoxLayout;
    auto* glyph = new QLabel(box);
    glyph->setFixedSize(32, 32);
    glyph->setAlignment(Qt::AlignCenter);
    glyph->setStyleSheet("background:#f3efff;border-radius:6px;color:#6235f7;");
    glyph->setText(icon == "Information"   ? QStringLiteral("i")
                   : icon == "Question"    ? QStringLiteral("?")
                   : icon == "Warning"     ? QStringLiteral("!")
                   : icon == "Critical"    ? QStringLiteral("!")
                                           : QString());
    row->addWidget(glyph);
    auto* textColumn = new QVBoxLayout;
    auto* text = new QLabel(QStringLiteral("工单复核完成，参数已通过规则校验。"), box);
    text->setWordWrap(true);
    textColumn->addWidget(text);
    if (details) {
        auto* detail = new QLabel(QStringLiteral("压力：1.2 Pa\n温度：21.4 °C\n批次：LOT-2026-0930"), box);
        detail->setStyleSheet("color:#444444;");
        detail->setWordWrap(true);
        textColumn->addWidget(detail);
    }
    row->addLayout(textColumn, 1);
    layout->addLayout(row);
    layout->addStretch(1);
    auto* buttons = new commonWidget::Z_DialogButtonBox(box);
    buttons->setStandardButtons(QDialogButtonBox::Ok);
    layout->addWidget(buttons);
    passive(box, QStringLiteral("Icon=%1, Details=%2")
                     .arg(icon, details ? QStringLiteral("Shown") : QStringLiteral("Hidden")));
    return box;
}

QWidget* nativeSurfaceCard(QWidget* canvas, const QRect& rect, const QString& component,
                           const QString& variant) {
    auto* card = new QWidget(canvas);
    card->setObjectName(QStringLiteral("nativeSurfaceBoundary"));
    card->setGeometry(rect);
    card->setStyleSheet(QStringLiteral(
        "QWidget#nativeSurfaceBoundary {background:#f7f8fa;border:1px dashed #aeb5c1;border-radius:8px;}"));
    card->setProperty("figmaComponentIdentity", component);
    card->setProperty("runtimeSurface", QStringLiteral("OS-owned native dialog"));
    card->setProperty("nativeSurfaceBoundary", true);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    auto* heading = new QLabel(QStringLiteral("Native surface · OS owned"), card);
    heading->setStyleSheet(QStringLiteral("font-weight:600;color:#202124;border:0;background:transparent;"));
    auto* note = new QLabel(QStringLiteral("此 Variant 由操作系统在独立窗口中渲染；分组页不以 QtWidget 外观冒充 Native。\n请在“外观”页启动交互实例。"), card);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color:#626a78;border:0;background:transparent;"));
    layout->addStretch(1);
    layout->addWidget(heading);
    layout->addWidget(note);
    layout->addStretch(1);
    passive(card, variant);
    return card;
}

QWidget* fileSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& intent, bool native) {
    const QString variant = QStringLiteral("Intent=%1, Surface=%2")
                                .arg(intent, native ? QStringLiteral("Native")
                                                    : QStringLiteral("QtWidget"));
    if (native)
        return nativeSurfaceCard(canvas, rect, QStringLiteral("ComponentLab/FileDialog"), variant);
    auto* dialog = qobject_cast<commonWidget::Z_FileDialog*>(
        factory->create("commonWidget::Z_FileDialog", canvas));
    if (!dialog)
        throw std::runtime_error("Z_FileDialog unavailable");
    dialog->setWindowFlags(Qt::Widget);
    dialog->setGeometry(rect);
    dialog->setFixedSize(rect.size());
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);
    dialog->setNameFilter(QStringLiteral("工单文件 (*.json *.xml)"));
    if (intent == "OpenFile")
        dialog->setFileMode(QFileDialog::ExistingFile);
    else if (intent == "OpenFiles")
        dialog->setFileMode(QFileDialog::ExistingFiles);
    else if (intent == "SaveFile")
        dialog->setAcceptMode(QFileDialog::AcceptSave);
    else if (intent == "Directory") {
        dialog->setFileMode(QFileDialog::Directory);
        dialog->setOption(QFileDialog::ShowDirsOnly, true);
    }
    dialog->setProperty("nativeSurfaceBoundary", false);
    passive(dialog, variant);
    return dialog;
}

QWidget* colorSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, bool native, bool alpha, bool live) {
    const QString variant = QStringLiteral("Surface=%1, Alpha=%2, Mode=%3")
                                .arg(native ? QStringLiteral("Native") : QStringLiteral("QtWidget"),
                                     alpha ? QStringLiteral("Shown") : QStringLiteral("Hidden"),
                                     live ? QStringLiteral("Live") : QStringLiteral("Confirm"));
    if (native)
        return nativeSurfaceCard(canvas, rect, QStringLiteral("ComponentLab/ColorDialog"), variant);
    auto* dialog = qobject_cast<commonWidget::Z_ColorDialog*>(
        factory->create("commonWidget::Z_ColorDialog", canvas));
    if (!dialog)
        throw std::runtime_error("Z_ColorDialog unavailable");
    dialog->setWindowFlags(Qt::Widget);
    dialog->setGeometry(rect);
    dialog->setFixedSize(rect.size());
    dialog->setOption(QColorDialog::DontUseNativeDialog, true);
    dialog->setOption(QColorDialog::ShowAlphaChannel, alpha);
    dialog->setCurrentColor(QColor("#6d4aff"));
    dialog->setProperty("nativeSurfaceBoundary", false);
    passive(dialog, variant);
    return dialog;
}

QWidget* fontSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, bool native, bool monospaced, bool live) {
    const QString variant = QStringLiteral("Surface=%1, Filter=%2, Mode=%3")
                                .arg(native ? QStringLiteral("Native") : QStringLiteral("QtWidget"),
                                     monospaced ? QStringLiteral("Monospaced") : QStringLiteral("All"),
                                     live ? QStringLiteral("Live") : QStringLiteral("Confirm"));
    if (native)
        return nativeSurfaceCard(canvas, rect, QStringLiteral("ComponentLab/FontDialog"), variant);
    auto* dialog = qobject_cast<commonWidget::Z_FontDialog*>(
        factory->create("commonWidget::Z_FontDialog", canvas));
    if (!dialog)
        throw std::runtime_error("Z_FontDialog unavailable");
    dialog->setWindowFlags(Qt::Widget);
    dialog->setGeometry(rect);
    dialog->setFixedSize(rect.size());
    dialog->setOption(QFontDialog::DontUseNativeDialog, true);
    dialog->setOption(QFontDialog::MonospacedFonts, monospaced);
    dialog->setProperty("nativeSurfaceBoundary", false);
    passive(dialog, variant);
    return dialog;
}

commonWidget::Z_InputDialog* inputSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, const QString& kind, bool live) {
    auto* dialog = qobject_cast<commonWidget::Z_InputDialog*>(
        factory->create("commonWidget::Z_InputDialog", canvas));
    if (!dialog)
        throw std::runtime_error("Z_InputDialog unavailable");
    dialog->setWindowFlags(Qt::Widget);
    dialog->setGeometry(rect);
    dialog->setFixedSize(rect.size());
    dialog->setLabelText(QStringLiteral("输入工单号"));
    dialog->setOkButtonText(QStringLiteral("确定"));
    dialog->setCancelButtonText(QStringLiteral("取消"));
    if (kind == "Text")
        dialog->setInputMode(QInputDialog::TextInput);
    else if (kind == "Multiline") {
        dialog->setInputMode(QInputDialog::TextInput);
        dialog->setOption(QInputDialog::UsePlainTextEditForTextInput, true);
    } else if (kind == "Item") {
        dialog->setInputMode(QInputDialog::TextInput);
        dialog->setComboBoxItems({QStringLiteral("工单 A"), QStringLiteral("工单 B"),
                                  QStringLiteral("工单 C")});
        dialog->setComboBoxEditable(false);
    } else if (kind == "Integer") {
        dialog->setInputMode(QInputDialog::IntInput);
        dialog->setIntRange(0, 9999);
        dialog->setIntValue(128);
    } else if (kind == "Double") {
        dialog->setInputMode(QInputDialog::DoubleInput);
        dialog->setDoubleRange(0.0, 100.0);
        dialog->setDoubleValue(72.5);
    }
    passive(dialog, QStringLiteral("Input=%1, Mode=%2")
                        .arg(kind, live ? QStringLiteral("Live") : QStringLiteral("Confirm")));
    return dialog;
}

commonWidget::Z_ProgressDialog* progressSpecimen(
    const std::shared_ptr<commonWidget::ComponentFactory>& factory, QWidget* canvas,
    const QRect& rect, bool determinate, bool modal) {
    auto* dialog = qobject_cast<commonWidget::Z_ProgressDialog*>(
        factory->create("commonWidget::Z_ProgressDialog", canvas));
    if (!dialog)
        throw std::runtime_error("Z_ProgressDialog unavailable");
    dialog->setWindowFlags(Qt::Widget);
    dialog->setGeometry(rect);
    dialog->setFixedSize(rect.size());
    dialog->setLabelText(QStringLiteral("正在校验参数"));
    if (determinate) {
        dialog->setRange(0, 100);
        dialog->setValue(64);
    } else {
        dialog->setRange(0, 0);
    }
    dialog->setModal(modal);
    passive(dialog, QStringLiteral("Progress=%1, Mode=%2")
                        .arg(determinate ? QStringLiteral("Determinate")
                                         : QStringLiteral("Indeterminate"),
                             modal ? QStringLiteral("Modal") : QStringLiteral("Modeless")));
    return dialog;
}

} // namespace

bool hasWindowsGallery(const QString& pageId) {
    return pageId == "573:2" || pageId == "588:12" || pageId == "591:14" || pageId == "598:2";
}

QWidget* createWindowsGallery(const QString& pageId,
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
        QWidget* specimen = nullptr;
        if (item.componentSetName.endsWith(QStringLiteral("StatusBar"))) {
            specimen = statusSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Scenario")),
                figmaAxisValue(item.variant, QStringLiteral("Grip")) == QStringLiteral("Shown"));
        } else if (item.componentSetName.endsWith(QStringLiteral("MessageDialog"))) {
            specimen = messageSpecimen(
                factory, canvas, item.rect, figmaAxisValue(item.variant, QStringLiteral("Icon")),
                figmaAxisValue(item.variant, QStringLiteral("Details")) == QStringLiteral("Shown"));
        } else if (item.componentSetName.endsWith(QStringLiteral("Dialog")) &&
                   !item.componentSetName.endsWith(QStringLiteral("FileDialog")) &&
                   !item.componentSetName.endsWith(QStringLiteral("ColorDialog")) &&
                   !item.componentSetName.endsWith(QStringLiteral("FontDialog")) &&
                   !item.componentSetName.endsWith(QStringLiteral("InputDialog")) &&
                   !item.componentSetName.endsWith(QStringLiteral("ProgressDialog"))) {
            specimen = dialogSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Modality")),
                figmaAxisValue(item.variant, QStringLiteral("Size")),
                figmaAxisValue(item.variant, QStringLiteral("State")) == QStringLiteral("Focus"));
        } else if (item.componentSetName.endsWith(QStringLiteral("FileDialog"))) {
            specimen = fileSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Intent")),
                figmaAxisValue(item.variant, QStringLiteral("Surface")) == QStringLiteral("Native"));
        } else if (item.componentSetName.endsWith(QStringLiteral("ColorDialog"))) {
            specimen = colorSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Surface")) == QStringLiteral("Native"),
                figmaAxisValue(item.variant, QStringLiteral("Alpha")) == QStringLiteral("Shown"),
                figmaAxisValue(item.variant, QStringLiteral("Mode")) == QStringLiteral("Live"));
        } else if (item.componentSetName.endsWith(QStringLiteral("FontDialog"))) {
            specimen = fontSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Surface")) == QStringLiteral("Native"),
                figmaAxisValue(item.variant, QStringLiteral("Filter")) == QStringLiteral("Monospaced"),
                figmaAxisValue(item.variant, QStringLiteral("Mode")) == QStringLiteral("Live"));
        } else if (item.componentSetName.endsWith(QStringLiteral("InputDialog"))) {
            specimen = inputSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Input")),
                figmaAxisValue(item.variant, QStringLiteral("Mode")) == QStringLiteral("Live"));
        } else if (item.componentSetName.endsWith(QStringLiteral("ProgressDialog"))) {
            specimen = progressSpecimen(
                factory, canvas, item.rect,
                figmaAxisValue(item.variant, QStringLiteral("Progress")) ==
                    QStringLiteral("Determinate"),
                figmaAxisValue(item.variant, QStringLiteral("Mode")) == QStringLiteral("Modal"));
        }
        if (!specimen)
            throw std::runtime_error("Unsupported Window & System component set");
        specimen->setProperty("figmaVariant", item.variant);
        specimen->setProperty("figmaComponentSet", item.componentSetName);
        specimen->setProperty("figmaComponentSetId", item.componentSetId);
        specimen->setProperty("figmaNodeId", item.nodeId);
        specimen->setProperty("figmaDesignBounds", item.rect);
    }
    return new FigmaGalleryView(canvas, parent);
}

} // namespace native_gallery
