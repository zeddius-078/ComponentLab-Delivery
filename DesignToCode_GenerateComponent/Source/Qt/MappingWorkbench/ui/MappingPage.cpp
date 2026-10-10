#include "MappingPage.h"
#include "QtMapping.h"
#include "Z_PushButton.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStyle>
#include <QTreeWidget>
#include <functional>

namespace design_mapping {
MappingPage::MappingPage(const QString& activeProfile, QWidget* flowPage, QWidget* filesPage,
                         QObject* parent)
    : QObject(parent), profile(activeProfile) {
    auto* flow = qobject_cast<QVBoxLayout*>(flowPage->layout());
    auto* toolbar = new QHBoxLayout;
    auto* load = new QPushButton(flowPage->style()->standardIcon(QStyle::SP_DialogOpenButton),
                                 QString::fromUtf8("载入契约"));
    auto* apply = new QPushButton(flowPage->style()->standardIcon(QStyle::SP_DialogApplyButton),
                                  QString::fromUtf8("校验并预览"));
    auto* reset = new QPushButton(flowPage->style()->standardIcon(QStyle::SP_BrowserReload),
                                  QString::fromUtf8("示例"));
    exportButton = new QPushButton(flowPage->style()->standardIcon(QStyle::SP_DialogSaveButton),
                                   QString::fromUtf8("导出 Qt 文件"));
    apply->setObjectName("mappingApply");
    exportButton->setObjectName("mappingExport");
    load->setToolTip(QString::fromUtf8("载入规范化的 UI Contract JSON"));
    exportButton->setToolTip(QString::fromUtf8("生成 .ui、.h、.cpp、.qss 与来源清单"));
    for (auto* button : {load, apply, reset, exportButton})
        toolbar->addWidget(button);
    toolbar->addStretch();
    flow->addLayout(toolbar);
    auto* stages = new QHBoxLayout;
    for (const auto& value : {QString::fromUtf8("01  设计实例\nFigma / Pixso"),
                              QString::fromUtf8("02  规则读取\nMCP / DSM"),
                              QString::fromUtf8("03  UI Contract\n本地 JSON"),
                              QString::fromUtf8("04  Qt Adapter\n已有组件 API"),
                              QString::fromUtf8("05  QWidget\n当前 Token Profile")}) {
        auto* label = new QLabel(value);
        label->setWordWrap(true);
        stages->addWidget(label, 1);
    }
    flow->addLayout(stages);
    status = new QLabel;
    status->setTextFormat(Qt::PlainText);
    status->setObjectName("mappingStatus");
    status->setWordWrap(true);
    status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    flow->addWidget(status);
    auto* splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);
    tree = new QTreeWidget;
    tree->setObjectName("mappingTree");
    tree->setHeaderLabels({QString::fromUtf8("设计节点"), "Alias"});
    tree->setColumnWidth(0, 145);
    tree->setMinimumWidth(180);
    previewArea = new QScrollArea;
    previewArea->setObjectName("mappingPreview");
    previewArea->setWidgetResizable(true);
    previewArea->setFrameShape(QFrame::NoFrame);
    previewArea->setMinimumWidth(250);
    details = new QTreeWidget;
    details->setObjectName("mappingDetails");
    details->setHeaderLabels(
        {QString::fromUtf8("映射项"), QString::fromUtf8("Qt API / 文件 / Token")});
    details->setColumnWidth(0, 105);
    details->setMinimumWidth(200);
    details->setWordWrap(true);
    splitter->addWidget(tree);
    splitter->addWidget(previewArea);
    splitter->addWidget(details);
    splitter->setSizes({230, 450, 360});
    flow->addWidget(splitter, 1);
    interaction = new QLabel(QString::fromUtf8("交互结果：等待操作"));
    interaction->setTextFormat(Qt::PlainText);
    interaction->setWordWrap(true);
    flow->addWidget(interaction);

    auto* fileLayout = qobject_cast<QVBoxLayout*>(filesPage->layout());
    auto* fileToolbar = new QHBoxLayout;
    auto* checkEditor = new QPushButton(apply->icon(), QString::fromUtf8("应用契约"));
    fileToolbar->addWidget(checkEditor);
    fileToolbar->addStretch();
    fileChoice = new QComboBox;
    fileChoice->setObjectName("mappingFileChoice");
    fileToolbar->addWidget(fileChoice);
    fileLayout->addLayout(fileToolbar);
    auto* fileSplit = new QSplitter;
    editor = new QPlainTextEdit;
    editor->setObjectName("mappingContractEditor");
    editor->setAccessibleName("UI Contract JSON");
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    output = new QPlainTextEdit;
    output->setObjectName("mappingGeneratedText");
    output->setReadOnly(true);
    output->setLineWrapMode(QPlainTextEdit::NoWrap);
    auto codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    codeFont.setPixelSize(editor->font().pixelSize() > 0 ? editor->font().pixelSize() : 14);
    editor->setFont(codeFont);
    output->setFont(codeFont);
    fileSplit->addWidget(editor);
    fileSplit->addWidget(output);
    fileSplit->setSizes({500, 500});
    fileLayout->addWidget(fileSplit, 1);
    connect(fileChoice, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { selectFile(); });
    connect(editor, &QPlainTextEdit::textChanged, this, [this] {
        exportButton->setEnabled(false);
        status->setText(QString::fromUtf8("契约已编辑，预览保留上次有效内容；请重新校验。"));
    });
    connect(apply, &QPushButton::clicked, this, [this] { applyContract(); });
    connect(checkEditor, &QPushButton::clicked, this, [this] { applyContract(); });
    connect(reset, &QPushButton::clicked, this, [this] {
        editor->setPlainText(QString::fromUtf8(QJsonDocument(example()).toJson()));
        applyContract();
    });
    connect(load, &QPushButton::clicked, this, [this, flowPage] {
        auto path = QFileDialog::getOpenFileName(flowPage, QString::fromUtf8("UI Contract"), {},
                                                 "JSON (*.json)");
        if (path.isEmpty())
            return;
        try {
            editor->setPlainText(QString::fromUtf8(read(path)));
            applyContract();
        } catch (const std::exception& error) {
            status->setText(QString::fromUtf8(error.what()));
        }
    });
    connect(exportButton, &QPushButton::clicked, this, [this, flowPage] {
        auto path = QFileDialog::getExistingDirectory(
            flowPage, QString::fromUtf8("选择导出父目录"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (path.isEmpty())
            return;
        try {
            status->setText(QString::fromUtf8("已导出：") + exportFiles(current, profile, path));
        } catch (const std::exception& error) {
            status->setText(QString::fromUtf8(error.what()));
        }
    });
    connect(tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        details->clear();
        if (!item)
            return;
        const auto node = item->data(0, Qt::UserRole).toJsonObject();
        const auto identity = node["design"].toObject();
        for (auto it = identity.begin(); it != identity.end(); ++it)
            new QTreeWidgetItem(details, {it.key(), it.value().toString()});
        auto map = componentMap()["components"].toObject();
        auto definition = map[node["component"].toString()].toObject();
        if (definition.contains("extends"))
            definition = map[definition["extends"].toString()].toObject();
        for (const auto& key : {"class", "header", "implementation", "tokenContract", "style"})
            if (definition.contains(key))
                new QTreeWidgetItem(details, {key, definition[key].toString()});
        auto specs = definition["properties"].toObject();
        auto p = node["properties"].toObject();
        for (auto it = p.begin(); it != p.end(); ++it)
            new QTreeWidgetItem(details, {it.key(), specs[it.key()].toObject()["api"].toString()});
        for (const auto& ref : node["tokenRefs"].toArray())
            new QTreeWidgetItem(details, {"Token", ref.toString()});
        auto layout = node["layout"].toObject();
        for (auto it = layout.begin(); it != layout.end(); ++it)
            new QTreeWidgetItem(details, {it.key(), it.value().toString()});
        for (int i = 0; i < details->topLevelItemCount(); ++i)
            details->topLevelItem(i)->setToolTip(1, details->topLevelItem(i)->text(1));
    });
    editor->setPlainText(QString::fromUtf8(QJsonDocument(example()).toJson()));
    applyContract();
}
bool MappingPage::applyContract() {
    try {
        const auto candidate = parse(editor->toPlainText().toUtf8());
        const auto generated = generate(candidate, profile);
        auto* page = preview(candidate, profile);
        auto* old = previewArea->takeWidget();
        previewArea->setWidget(page);
        delete old;
        current = candidate;
        files = generated;
        files.insert("qt-component-map.json", read(":/mapping/qt-component-map.json"));
        files.insert("component-parsers.json", read(":/mapping/component-parsers.json"));
        files.insert("code-options.json", read(":/mapping/code-options.json"));
        const auto selected = fileChoice->currentText();
        fileChoice->clear();
        fileChoice->addItems(files.keys());
        fileChoice->setCurrentText(files.contains(selected) ? selected : "MappingGenerated.ui");
        selectFile();
        populateTree();
        status->setText(QString::fromUtf8("校验通过 | %1 | %2 | Token 只读 | 来源真实性与视觉对照需另行确认")
                            .arg(current["source"].toObject()["kind"].toString(), profile));
        exportButton->setEnabled(true);
        interaction->setText(QString::fromUtf8("交互结果：等待操作"));
        for (auto* b : page->findChildren<commonWidget::Z_PushButton*>())
            connect(b, &QPushButton::clicked, this,
                    [this, b] { interaction->setText(b->objectName() + ": clicked"); });
        for (auto* c : page->findChildren<commonWidget::Z_CheckBox*>())
            connect(c, &QCheckBox::toggled, this, [this, c](bool value) {
                interaction->setText(c->objectName() + ": checked=" + (value ? "true" : "false"));
            });
        for (auto* input : page->findChildren<commonWidget::Z_InputField*>())
            connect(input, &commonWidget::Z_InputField::valueChanged, this,
                    [this, input](const QString& value) {
                        interaction->setText(input->objectName() + ": " + value);
                    });
        for (auto* select : page->findChildren<commonWidget::Z_Select*>())
            connect(select, &commonWidget::Z_Select::valuesChanged, this,
                    [this, select](const QStringList& values) {
                        interaction->setText(select->objectName() + ": [" + values.join(", ") +
                                             "]");
                    });
        return true;
    } catch (const std::exception& error) {
        status->setText(QString::fromUtf8("校验失败：") + QString::fromUtf8(error.what()));
        exportButton->setEnabled(false);
        output->setPlainText(status->text());
        return false;
    }
}
void MappingPage::selectFile() {
    output->setPlainText(QString::fromUtf8(files.value(fileChoice->currentText())));
}
void MappingPage::populateTree() {
    tree->clear();
    std::function<void(const QJsonObject&, QTreeWidgetItem*)> add = [&](const QJsonObject& node,
                                                                        QTreeWidgetItem* parent) {
        auto* item =
            new QTreeWidgetItem(parent, {node["id"].toString(), node["component"].toString()});
        item->setData(0, Qt::UserRole, node);
        item->setToolTip(0, node["id"].toString());
        for (const auto& child : node["children"].toArray())
            add(child.toObject(), item);
    };
    add(current["page"].toObject(), tree->invisibleRootItem());
    tree->expandAll();
    auto* root = tree->topLevelItem(0);
    tree->setCurrentItem(root && root->childCount() ? root->child(0) : root);
}
} // namespace design_mapping
