#include "DemoWindow.h"
#include "NativeGalleryPages.h"
#include "V2ComponentMapPage.h"
#include "ui_ComponentLab.h"
#include "Z_CheckBox.h"
#include "Z_PushButton.h"
#include "CheckBoxGroupController.h"
#include "TokenResources.h"
#include "FocusIndicator.h"
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QEventLoop>
#include <QFormLayout>
#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMetaProperty>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSet>
#include <QSplitter>
#include <QTimer>
#include <QTabWidget>
#include <QTableWidget>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <stdexcept>
using commonWidget::Z_CheckBox;
using commonWidget::Z_PushButton;
namespace {
QComboBox* appearanceCombo(const surface::Catalog& catalog, QWidget* parent) {
    auto* c = new QComboBox(parent);
    for (auto id : catalog.presetIds())
        c->addItem(catalog.label(id), id);
    return c;
}
QTreeWidget* tree(QWidget* parent) {
    auto* t = new QTreeWidget(parent);
    t->setHeaderLabels({QString::fromUtf8("节点 / 属性"), QString::fromUtf8("类型 / 值")});
    t->setColumnWidth(0, 180);
    t->setMinimumWidth(280);
    return t;
}
void addTree(QTreeWidgetItem* parent, QWidget* widget) {
    auto* item =
        new QTreeWidgetItem(parent, {widget->objectName(), widget->metaObject()->className()});
    auto g = widget->geometry();
    new QTreeWidgetItem(
        item, {"geometry",
               QString("%1,%2 / %3x%4").arg(g.x()).arg(g.y()).arg(g.width()).arg(g.height())});
    const auto* meta = widget->metaObject();
    for (const auto& name : {"enabled", "visible", "text", "checked", "value",
                             "currentIndex", "placeholderText", "readOnly", "title"}) {
        const int index = meta->indexOfProperty(name);
        if (index >= 0)
            new QTreeWidgetItem(item, {QString::fromLatin1(name),
                                       meta->property(index).read(widget).toString()});
    }
    for (auto property : widget->dynamicPropertyNames())
        if (!property.startsWith('_'))
            new QTreeWidgetItem(item, {property, widget->property(property).toString()});
    for (auto* child : widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        addTree(item, child);
}
QWidget* previewHost(QLayout* into) {
    auto* w = new QWidget;
    w->setObjectName("previewHost");
    w->setMinimumHeight(160);
    into->addWidget(w);
    return w;
}
} // namespace
DemoWindow::DemoWindow(const QString& current)
    : ui(new Ui::ComponentLab), catalog(std::make_shared<surface::Catalog>(current)),
      profile(current) {
    ui->setupUi(this);
    // This gallery now contains component, layout-helper and asset sections.
    // Keep the runtime label explicit so stale generated UI headers cannot retain
    // the former component-only wording in incremental builds.
    ui->nativeSections->setTabText(ui->nativeSections->indexOf(ui->nativeGalleryTab),
                                   QString::fromUtf8("设计系统图库"));
    using commonWidget::FocusIndicator;
    auto& focus = FocusIndicator::instance();
    ui->focusIndicatorCombo->addItem(QString::fromUtf8("关闭"), "off");
    ui->focusIndicatorCombo->addItem(QString::fromUtf8("仅键盘"), "keyboard");
    ui->focusIndicatorCombo->addItem(QString::fromUtf8("始终显示"), "always");
    ui->focusIndicatorCombo->setCurrentIndex(
        ui->focusIndicatorCombo->findData(FocusIndicator::modeName(focus.mode())));
    connect(ui->focusIndicatorCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { FocusIndicator::instance().setMode(FocusIndicator::parseMode(
                         ui->focusIndicatorCombo->currentData().toString())); });
    connect(&focus, &FocusIndicator::changed, this, [this] {
        QSignalBlocker blocker(ui->focusIndicatorCombo);
        ui->focusIndicatorCombo->setCurrentIndex(ui->focusIndicatorCombo->findData(
            FocusIndicator::modeName(FocusIndicator::instance().mode())));
    });
    ui->profileCombo->addItem(QString::fromUtf8("亮色 / 舒适"), "standard-light");
    ui->profileCombo->addItem(QString::fromUtf8("亮色 / 紧凑"), "wide-light");
    ui->profileCombo->addItem(QString::fromUtf8("暗色 / 舒适"), "standard-dark");
    ui->profileCombo->addItem(QString::fromUtf8("暗色 / 紧凑"), "wide-dark");
    ui->profileCombo->setCurrentIndex(ui->profileCombo->findData(current));
    connect(ui->profileCombo, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        QStringList arguments{"--profile", ui->profileCombo->itemData(index).toString()};
        arguments << "--focus-indicator"
                  << commonWidget::FocusIndicator::modeName(commonWidget::FocusIndicator::instance().mode());
        if (!token_resources::generatedDirectory().isEmpty())
            arguments << "--token-dir" << token_resources::generatedDirectory();
        if (QProcess::startDetached(QCoreApplication::applicationFilePath(), arguments))
            close();
    });
    inspectPage();
    matrixPage();
    groupPage();
    buttonPage();
    buttonStatesPage();
    formPages();
    nativePages();
    // V2 组件地图：每个组件的外观绑定与状态表由 V2 规范产物数据驱动，
    // 与 Figma 变量组映射全量对齐，不再需要逐组件手写图库页面。
    ui->nativeSections->addTab(new component_lab_v2::V2ComponentMapPage(ui->nativeRegistryTab),
                               QString::fromUtf8("V2 Token 地图（当前）"));
    // The first registry tab is the historical SDK catalog and is retained for
    // compatibility.  Make the authoritative V2 map the default view so a
    // fresh launch cannot be mistaken for the legacy token mapping.
    ui->nativeSections->setTabText(1, QString::fromUtf8("组件映射（Legacy）"));
    ui->nativeSections->setCurrentIndex(2);
    setWindowTitle("Component Lab | Component SDK Gallery");
    ui->statusLabel->setText("COMPONENT-LAB V2 Token Architecture  |  " + current + "  |  " +
                             catalog->sourceHash().left(16));
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DemoWindow::updateTrees);
    timer->start(400);
    updateTrees();
}
DemoWindow::~DemoWindow() {
    disconnect(&commonWidget::FocusIndicator::instance(), nullptr, this, nullptr);
}
void DemoWindow::nativePages() {
    const QMap<QString, QWidget*> detailedPages{
        {"commonWidget::Z_PushButton", ui->buttonTab},
        {"commonWidget::Z_CheckBox", ui->checkboxTab},
        {"commonWidget::Z_InputField", ui->inputTab},
        {"commonWidget::Z_Select", ui->selectTab},
    };
    for (int i = 0; i < 4; ++i)
        ui->tabs->removeTab(0);
    ui->nativeGalleryLayout->addWidget(
        native_gallery::createGallery(profile, detailedPages, ui->nativeGalleryTab));
    ui->nativeRegistryLayout->addWidget(native_gallery::createRegistry(ui->nativeRegistryTab));
}
void DemoWindow::inspectPage() {
    auto* splitter = new QSplitter;
    ui->inspectLayout->addWidget(splitter);
    auto* settings = new QWidget;
    auto* controls = new QFormLayout(settings);
    controls->setVerticalSpacing(14);
    settings->setMinimumWidth(220);
    auto* label = new QLineEdit(QString::fromUtf8("启用质量检查"));
    auto* preset = appearanceCombo(*catalog, settings);
    preset->setObjectName("checkboxAppearance");
    preset->insertItem(0, QString::fromUtf8("Figma 当前画廊 / Light"), "figma-light");
    preset->setCurrentIndex(0);
    auto* variantValue = new QComboBox;
    variantValue->setObjectName("checkboxFigmaValue");
    variantValue->addItems({"Unchecked", "Checked", "Indeterminate", QString::fromUtf8("SDK checked + indeterminate（非 Figma 变体）")});
    variantValue->setCurrentIndex(1);
    auto* variantState = new QComboBox;
    variantState->setObjectName("checkboxFigmaState");
    variantState->addItems({QString::fromUtf8("交互 / Auto"), "Default", "Hover", "Pressed", "Focus-visible", "Disabled"});
    auto* size = new QComboBox;
    size->addItem("sm / 20px", "sm");
    size->addItem("lg / 24px", "lg");
    auto *checked = new Z_CheckBox(catalog, "checked"),
         *mixed = new Z_CheckBox(catalog, "indeterminate"),
         *disabled = new Z_CheckBox(catalog, "disabled"),
         *controlled = new Z_CheckBox(catalog, "controlled"),
         *accept = new Z_CheckBox(catalog, QString::fromUtf8("提交请求"));
    accept->setChecked(true);
    controls->addRow(QString::fromUtf8("文本"), label);
    controls->addRow(QString::fromUtf8("外观"), preset);
    controls->addRow(QString::fromUtf8("尺寸"), size);
    controls->addRow(QString::fromUtf8("取值"), variantValue);
    controls->addRow(QString::fromUtf8("状态"), variantState);
    variantValue->setToolTip("Figma Value / SDK checked + indeterminate");
    variantState->setToolTip(QString::fromUtf8("Figma State 预览；Auto 使用真实鼠标与键盘状态"));
    for (auto* c : {checked, mixed, disabled, controlled, accept})
        controls->addRow(c);
    auto* area = new QWidget;
    auto* layout = new QVBoxLayout(area);
    auto* host = previewHost(layout);
    auto* box = new QVBoxLayout(host);
    box->setContentsMargins(36, 36, 36, 36);
    preview = new Z_CheckBox(catalog, label->text(), host);
    preview->setAppearance("figma-light");
    preview->setChecked(true);
    checked->setChecked(true);
    box->addWidget(preview, 0, Qt::AlignCenter);
    auto* value = new QLabel;
    value->setWordWrap(true);
    layout->addWidget(value);
    layout->addStretch();
    auto describe = [this, value] {
        value->setText(QString("checked=%1  indeterminate=%2\nrequest mode=%3")
                           .arg(preview->isChecked())
                           .arg(preview->isIndeterminate())
                           .arg(preview->isControlled() ? "controlled" : "uncontrolled"));
    };
    connect(label, &QLineEdit::textChanged, preview, &Z_CheckBox::setText);
    connect(preset, qOverload<int>(&QComboBox::currentIndexChanged), preview,
            [this, preset] { preview->setAppearance(preset->currentData().toString()); });
    connect(size, qOverload<int>(&QComboBox::currentIndexChanged), preview,
            [this, size] { preview->setControlSize(size->currentData().toString()); });
    connect(checked, &QCheckBox::toggled, preview, &Z_CheckBox::setChecked);
    connect(mixed, &QCheckBox::toggled, preview, &Z_CheckBox::setIndeterminate);
    connect(variantValue, qOverload<int>(&QComboBox::currentIndexChanged), preview,
            [this, checked, mixed](int index) {
        preview->setChecked(index == 1 || index == 3);
        preview->setIndeterminate(index == 2 || index == 3);
        const QSignalBlocker checkedBlock(checked), mixedBlock(mixed);
        checked->setChecked(preview->isChecked());mixed->setChecked(preview->isIndeterminate());
    });
    auto syncValue = [this, variantValue] {
        const QSignalBlocker block(variantValue);
        variantValue->setCurrentIndex(preview->isIndeterminate() ? (preview->isChecked() ? 3 : 2) : (preview->isChecked() ? 1 : 0));
    };
    connect(preview, &QAbstractButton::toggled, preview, syncValue);
    connect(preview, &Z_CheckBox::indeterminateChanged, preview, syncValue);
    connect(variantState, qOverload<int>(&QComboBox::currentIndexChanged), preview,
            [this, variantState, disabled](int index) {
        preview->setProperty("figmaPreviewState", index ? variantState->currentText() : QString());
        disabled->setChecked(index == 5);
    });
    connect(disabled, &QCheckBox::toggled, preview, [this](bool on) { preview->setEnabled(!on); });
    connect(controlled, &QCheckBox::toggled, preview, [this, describe](bool on) {
        preview->setControlled(on);
        describe();
    });
    connect(preview, &QAbstractButton::toggled, this, [checked, describe](bool on) {
        QSignalBlocker block(checked);
        checked->setChecked(on);
        describe();
    });
    connect(preview, &Z_CheckBox::indeterminateChanged, this, [describe] { describe(); });
    connect(preview, &Z_CheckBox::checkedChangeRequested, this, [this, accept, describe](bool on) {
        if (preview->isControlled() && accept->isChecked())
            preview->setChecked(on);
        describe();
    });
    checkboxTree = tree(splitter);
    splitter->addWidget(settings);
    splitter->addWidget(area);
    splitter->addWidget(checkboxTree);
    splitter->setSizes({230, 450, 370});
    describe();
}
void DemoWindow::matrixPage() {
    auto* body = new QWidget;
    ui->matrixLayout->addWidget(body);
    auto* grid = new QGridLayout(body);
    grid->setContentsMargins(18, 18, 18, 18);
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(18);
    QStringList headings{QString::fromUtf8("外观"),
                         QString::fromUtf8("未选"),
                         QString::fromUtf8("选中"),
                         QString::fromUtf8("半选"),
                         "Hover",
                         "Pressed",
                         QString::fromUtf8("禁用")};
    for (int i = 0; i < headings.size(); ++i)
        grid->addWidget(new QLabel(headings[i]), 0, i);
    int row = 1;
    for (auto id : catalog->presetIds()) {
        grid->addWidget(new QLabel(catalog->label(id)), row, 0);
        for (int col = 1; col <= 6; ++col) {
            auto* c = new Z_CheckBox(catalog, {}, body);
            c->setObjectName(id + "_" + QString::number(col));
            c->setAppearance(id);
            c->setControlSize("lg");
            c->setChecked(col != 1 && col != 3);
            c->setIndeterminate(col == 3);
            c->setEnabled(col != 6);
            grid->addWidget(c, row, col, Qt::AlignCenter);
            if (col == 4) {
                QEvent enter(QEvent::Enter);
                QApplication::sendEvent(c, &enter);
            }
            if (col == 5) {
                c->setDown(true);
                QEvent enter(QEvent::Enter);
                QApplication::sendEvent(c, &enter);
            }
        }
        ++row;
    }
    grid->setRowStretch(row, 1);
}
void DemoWindow::groupPage() {
    auto* body = new QWidget;
    ui->groupLayout->addWidget(body);
    auto* columns = new QHBoxLayout(body);
    columns->setContentsMargins(32, 28, 32, 28);
    columns->setSpacing(72);
    auto* left = new QVBoxLayout;
    left->setSpacing(12);
    columns->addLayout(left);
    left->addWidget(new QLabel(QString::fromUtf8("检查项目")));
    auto* summary = new Z_CheckBox(catalog, QString::fromUtf8("全选可操作项目"));
    summary->setObjectName("selectAll");
    left->addWidget(summary);
    QVector<Z_CheckBox*> items;
    QStringList labels{QString::fromUtf8("设备状态"), QString::fromUtf8("批次信息"),
                       QString::fromUtf8("量测结果"), QString::fromUtf8("归档记录（锁定）")};
    for (int i = 0; i < labels.size(); ++i) {
        auto* c = new Z_CheckBox(catalog, labels[i]);
        c->setObjectName("item" + QString::number(i));
        c->setChecked(i == 0 || i == 3);
        c->setEnabled(i != 3);
        left->addWidget(c);
        items << c;
    }
    auto* values = new QLabel;
    values->setWordWrap(true);
    left->addWidget(values);
    left->addStretch();
    auto* group = new CheckBoxGroupController(summary, items, this);
    connect(group, &CheckBoxGroupController::valuesChanged, values,
            [values](const QStringList& v) { values->setText("value = [" + v.join(", ") + "]"); });
    group->synchronize();
    auto* right = new QVBoxLayout;
    right->setSpacing(12);
    columns->addLayout(right);
    right->addWidget(new QLabel(QString::fromUtf8("受控提交")));
    auto* gate = new Z_CheckBox(catalog, QString::fromUtf8("允许提交"));
    gate->setChecked(true);
    right->addWidget(gate);
    auto* controlled = new Z_CheckBox(catalog, QString::fromUtf8("启用设备报警"));
    controlled->setControlled(true);
    controlled->setAppearance("gradient-inset");
    right->addWidget(controlled);
    auto* log = new QLabel("checked=false");
    log->setWordWrap(true);
    right->addWidget(log);
    right->addStretch();
    connect(controlled, &Z_CheckBox::checkedChangeRequested, this,
            [controlled, gate, log](bool on) {
                if (gate->isChecked())
                    controlled->setChecked(on);
                log->setText(QString("requested=%1\ncommitted=%2\naccepted=%3")
                                 .arg(on)
                                 .arg(controlled->isChecked())
                                 .arg(gate->isChecked()));
            });
}
void DemoWindow::updateTrees() {
    QVector<QPair<QTreeWidget*, QWidget*>> inspections{
        qMakePair(checkboxTree, static_cast<QWidget*>(preview)),
        qMakePair(buttonTree, static_cast<QWidget*>(buttonPreview)),
        qMakePair(inputTree, inputPreview), qMakePair(selectTree, selectPreview)};
    inspections += extraInspections;
    for (auto pair : inspections) {
        if (!pair.first || !pair.second)
            continue;
        // Detailed editors are retained for lazy reuse inside the Figma page stack.
        // Most are hidden, so rebuilding all of their QObject trees every 400 ms
        // blocks the UI thread without producing any visible change.
        if (!pair.first->isVisibleTo(this) || !pair.second->isVisibleTo(this))
            continue;
        const int scroll = pair.first->verticalScrollBar()->value();
        const bool initial = pair.first->topLevelItemCount() == 0;
        QSet<QString> expanded;
        QString selected;
        std::function<void(QTreeWidgetItem*, QString, bool)> visit =
            [&](QTreeWidgetItem* parent, QString path, bool restore) {
                for (int i = 0; i < parent->childCount(); ++i) {
                    auto* item = parent->child(i);
                    auto key = path + '/' + item->text(0);
                    if (restore) {
                        item->setExpanded(expanded.contains(key));
                        if (key == selected)
                            pair.first->setCurrentItem(item);
                    } else {
                        if (item->isExpanded())
                            expanded.insert(key);
                        if (item->isSelected())
                            selected = key;
                    }
                    visit(item, key, restore);
                }
            };
        visit(pair.first->invisibleRootItem(), {}, false);
        pair.first->setUpdatesEnabled(false);
        pair.first->clear();
        addTree(pair.first->invisibleRootItem(), pair.second);
        auto* root = pair.first->topLevelItem(0);
        QStringList names =
            pair.second == preview ? QStringList{"checked",    "indeterminate", "controlled",
                                                 "appearance", "controlSize",   "interactionState"}
            : pair.second == buttonPreview
                ? QStringList{"iconOnly", "visualState", "leadingIconSource", "trailingIconSource"}
            : pair.second == inputPreview || pair.second == selectPreview
                ? QStringList{"value", "values", "controlled", "open"}
                : QStringList{};
        for (auto name : names)
            new QTreeWidgetItem(root, {name, pair.second->property(name.toLatin1()).toString()});
        if (initial)
            pair.first->expandToDepth(1);
        else
            visit(pair.first->invisibleRootItem(), {}, true);
        pair.first->verticalScrollBar()->setValue(scroll);
        pair.first->setUpdatesEnabled(true);
    }
}
void DemoWindow::capture(const QString& path, const QString& pageId) {
    if (!QDir().mkpath(path))
        throw std::runtime_error("Cannot create capture directory");
    QJsonArray navigation;
    QJsonArray layoutDiagnostics;
    int captured = 0;
    auto* componentTree = findChild<QTreeWidget*>("componentTree");
    const QStringList sampledPages = pageId.isEmpty()
        ? QStringList{"172:2710", "234:2717", "339:2", "242:2720", "443:2", "363:2"}
        : QStringList{pageId};
    ui->nativeSections->setCurrentIndex(0);
    for (int i = 0; i < sampledPages.size(); ++i) {
        QTreeWidgetItem* selected = nullptr;
        for (QTreeWidgetItemIterator it(componentTree); *it; ++it) {
            if ((*it)->data(0, Qt::UserRole).toString() == sampledPages[i]) {
                selected = *it;
                break;
            }
        }
        if (!selected)
            throw std::runtime_error(
                QString("Component tree entry missing during capture: %1")
                    .arg(sampledPages[i])
                    .toStdString());
        componentTree->setCurrentItem(selected);
        auto* pageStack = findChild<QStackedWidget*>("componentPages");
        auto* page = pageStack ? pageStack->currentWidget() : nullptr;
        auto* sub = page ? page->findChild<QTabWidget*>("componentSections") : nullptr;
        QJsonArray children;
        const int sections = sub ? sub->count() : 1;
        for (int j = 0; j < sections; ++j) {
            if (sub) sub->setCurrentIndex(j);
            QApplication::processEvents();
            // Newly selected, lazily built pages can post another layout request
            // after scrollbars are polished. Capture after that frame has settled.
            QEventLoop settled;
            QTimer::singleShot(80,&settled,&QEventLoop::quit);
            settled.exec(QEventLoop::ExcludeUserInputEvents);
            if (auto* gallery = page ? page->findChild<QGraphicsView*>("figmaVariantGallery")
                                     : nullptr;
                gallery && gallery->property("variantBuildComplete").isValid()) {
                QElapsedTimer buildDeadline;
                buildDeadline.start();
                while (!gallery->property("variantBuildComplete").toBool() &&
                       buildDeadline.elapsed() < 30000) {
                    QEventLoop buildStep;
                    QTimer::singleShot(10,&buildStep,&QEventLoop::quit);
                    buildStep.exec(QEventLoop::ExcludeUserInputEvents);
                }
                if (!gallery->property("variantBuildComplete").toBool())
                    throw std::runtime_error(
                        QString("Input gallery build timed out during capture (%1 / %2 specimens)")
                            .arg(gallery->property("variantBuiltCount").toInt())
                            .arg(gallery->property("variantSpecimenCount").toInt())
                            .toStdString());
                if (!gallery->property("variantBuildError").toString().isEmpty())
                    throw std::runtime_error(
                        gallery->property("variantBuildError").toString().toStdString());
            }
            updateTrees();
            QApplication::processEvents();
            if(j==0 && page) {
                const auto inspectors=page->findChildren<QScrollArea*>("componentSettingsScroll");
                for(auto* inspector:inspectors)if(inspector->isVisible())
                    layoutDiagnostics.append(QJsonObject{{"pageId",sampledPages[i]},
                        {"settingsViewportWidth",inspector->viewport()->width()},
                        {"settingsContentWidth",inspector->widget()->width()},
                        {"settingsHorizontalOverflow",inspector->horizontalScrollBar()->maximum()}});
            }
            if (!grab().save(path + QString("/component-%1-%2.png").arg(i).arg(j)))
                throw std::runtime_error("Screenshot failed");
            if (j == 1) {
                auto* gallery = page->findChild<QGraphicsView*>("figmaVariantGallery");
                if (gallery) {
                    for (auto* item : gallery->scene()->items()) {
                        auto* proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);
                        if (proxy && proxy->widget()->objectName() == "figmaGalleryCanvas"
                            && !proxy->widget()->grab().save(path + "/figma-gallery.png"))
                            throw std::runtime_error("Gallery canvas capture failed");
                    }
                }
            }
            children.append(sub ? sub->tabText(j) : QString::fromUtf8("预览"));
            ++captured;
        }
        navigation.append(QJsonObject{{"pageId", sampledPages[i]},
                                      {"page", selected->text(0)},
                                      {"sections", children}});
        if (sub) sub->setCurrentIndex(0);
    }
    ui->nativeSections->setCurrentIndex(1);
    QApplication::processEvents();
    if (!grab().save(path + "/component-mapping.png"))
        throw std::runtime_error("Mapping screenshot failed");
    ++captured;
    ui->nativeSections->setCurrentIndex(0);
    QJsonObject result{
        {"profile", profile},
        {"focusIndicator", commonWidget::FocusIndicator::modeName(commonWidget::FocusIndicator::instance().mode())},
        {"focusOpacity", commonWidget::FocusIndicator::instance().opacity()},
        {"appearance", preview->appearance()},
        {"sourceHash", catalog->sourceHash()},
        {"screens", ui->tabs->count()},
        {"capturedPages", captured},
        {"designSystemComponents", findChild<QWidget*>("nativeGalleryPage")->property("componentCount").toInt()},
        {"figmaComponentSets", findChild<QWidget*>("nativeGalleryPage")->property("componentSetCount").toInt()},
        {"figmaVariants", findChild<QWidget*>("nativeGalleryPage")->property("variantCount").toInt()},
        {"nativeDescriptors", findChild<QWidget*>("nativeGalleryPage")->property("descriptorCount").toInt()},
        {"nativeRegistryRows", findChild<QTableWidget*>("nativeRegistryTable")->rowCount()},
        {"navigation", navigation},
        {"layoutDiagnostics",layoutDiagnostics},
        {"presets", QJsonArray::fromStringList(catalog->presetIds())},
        {"buttonAppearances", QJsonArray{"filled", "outlined", "dashed", "text", "link"}},
        {"buttonAppearance", buttonPreview->property("buttonAppearance").toString()},
        {"checkboxSize", QString("%1x%2").arg(preview->width()).arg(preview->height())},
        {"indicatorSize", preview->indicatorRect().width()},
        {"buttonIconSource", buttonPreview->leadingIconSource()},
        {"buttonTrailingIconSource", buttonPreview->trailingIconSource()}};
    QSaveFile file(path + "/runtime.json");
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(result).toJson()) < 0 ||
        !file.commit())
        throw std::runtime_error("Capture report failed");
    ui->tabs->setCurrentIndex(0);
}
void DemoWindow::setAppearancePreview(const QString& id) {
    catalog->label(id);
    for (const char* name :
         {"checkboxAppearance", "buttonAppearance", "buttonStateAppearance", "inputAppearance",
          "inputStateAppearance", "selectAppearance", "selectStateAppearance"}) {
        auto* combo = findChild<QComboBox*>(name);
        const auto index = combo->findData(id);
        if (index >= 0)
            combo->setCurrentIndex(index);
    }
}
