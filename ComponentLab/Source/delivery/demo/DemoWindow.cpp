#include "DemoWindow.h"
#include "NativeGalleryPages.h"
#include "ui_ComponentLab.h"
#include "Z_CheckBox.h"
#include "ButtonAppearanceAdapter.h"
#include "CheckBoxGroupController.h"
#include "TokenResources.h"
#include "FocusIndicator.h"
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFormLayout>
#include <QGridLayout>
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
#include <QTreeWidget>
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
    ui->profileCombo->addItem(QString::fromUtf8("亮色 / 舒适"), "light-comfortable");
    ui->profileCombo->addItem(QString::fromUtf8("亮色 / 紧凑"), "light-compact");
    ui->profileCombo->addItem(QString::fromUtf8("暗色 / 舒适"), "dark-comfortable");
    ui->profileCombo->addItem(QString::fromUtf8("暗色 / 紧凑"), "dark-compact");
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
    setWindowTitle("Component Lab | Component SDK Gallery");
    ui->statusLabel->setText("COMPONENT-LAB R4-sToken  |  " + current + "  |  " +
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
    for (auto* c : {checked, mixed, disabled, controlled, accept})
        controls->addRow(c);
    auto* area = new QWidget;
    auto* layout = new QVBoxLayout(area);
    auto* host = previewHost(layout);
    auto* box = new QVBoxLayout(host);
    box->setContentsMargins(36, 36, 36, 36);
    preview = new Z_CheckBox(catalog, label->text(), host);
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
void DemoWindow::capture(const QString& path) {
    if (!QDir().mkpath(path))
        throw std::runtime_error("Cannot create capture directory");
    QJsonArray navigation;
    int captured = 0;
    auto* componentTree = findChild<QTreeWidget*>("componentTree");
    const QStringList sampledComponents{
        "commonWidget::Z_PushButton", "commonWidget::Z_CheckBox",
        "commonWidget::Z_InputField", "commonWidget::Z_Select",
        "commonWidget::Z_ToolButton", "commonWidget::Z_TableView"};
    ui->nativeSections->setCurrentIndex(0);
    for (int i = 0; i < sampledComponents.size(); ++i) {
        QTreeWidgetItem* selected = nullptr;
        for (auto* item : componentTree->findItems("*", Qt::MatchWildcard | Qt::MatchRecursive)) {
            if (item->data(0, Qt::UserRole).toString() == sampledComponents[i]) {
                selected = item;
                break;
            }
        }
        if (!selected)
            throw std::runtime_error("Component tree entry missing during capture");
        componentTree->setCurrentItem(selected);
        auto* page = findChild<QWidget*>("componentPages")->findChild<QWidget*>(
            "componentPage_" + sampledComponents[i].section("::", -1), Qt::FindDirectChildrenOnly);
        auto* sub = page ? page->findChild<QTabWidget*>("componentSections") : findChild<QTabWidget*>(
            sampledComponents[i] == "commonWidget::Z_PushButton" ? "buttonSections" :
            sampledComponents[i] == "commonWidget::Z_CheckBox" ? "checkboxSections" :
            sampledComponents[i] == "commonWidget::Z_InputField" ? "inputSections" :
            sampledComponents[i] == "commonWidget::Z_Select" ? "selectSections" : "");
        QJsonArray children;
        const int sections = sub ? sub->count() : 1;
        for (int j = 0; j < sections; ++j) {
            if (sub) sub->setCurrentIndex(j);
            QApplication::processEvents();
            updateTrees();
            QApplication::processEvents();
            if (!grab().save(path + QString("/component-%1-%2.png").arg(i).arg(j)))
                throw std::runtime_error("Screenshot failed");
            children.append(sub ? sub->tabText(j) : QString::fromUtf8("预览"));
            ++captured;
        }
        navigation.append(QJsonObject{{"component", sampledComponents[i]}, {"sections", children}});
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
        {"nativeDescriptors", findChild<QWidget*>("nativeGalleryPage")->property("descriptorCount").toInt()},
        {"nativeRegistryRows", findChild<QTableWidget*>("nativeRegistryTable")->rowCount()},
        {"navigation", navigation},
        {"presets", QJsonArray::fromStringList(catalog->presetIds())},
        {"buttonPresets", QJsonArray::fromStringList(catalog->buttonPresetIds())},
        {"buttonAppearance", buttonPreview->findChild<ButtonAppearanceAdapter*>()->appearance()},
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
