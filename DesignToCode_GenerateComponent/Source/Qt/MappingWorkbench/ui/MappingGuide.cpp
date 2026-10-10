#include "MappingGuide.h"
#include "QtMapping.h"
#include <QBoxLayout>
#include <QDesktopServices>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QUrl>

namespace design_mapping {
void populateGuide(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());
    auto* toolbar = new QHBoxLayout;
    auto* status = new QLabel(QString::fromUtf8("设计变量已核对 | 组件集等待字体确认 | Token 管道保持原路径"));
    status->setObjectName("mappingDesignSourceStatus");
    status->setWordWrap(true);
    toolbar->addWidget(status, 1);
    auto* open = new QPushButton(page->style()->standardIcon(QStyle::SP_DirLinkIcon),
                                QString::fromUtf8("打开 Figma"));
    toolbar->addWidget(open);
    layout->addLayout(toolbar);
    QObject::connect(open, &QPushButton::clicked, page, [] {
        QDesktopServices::openUrl(QUrl("https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf?node-id=127-2710"));
    });
    auto* split = new QSplitter(page);
    split->setChildrenCollapsible(false);
    auto* explanation = new QTextBrowser(split);
    explanation->setObjectName("mappingImplementationGuide");
    explanation->setFrameShape(QFrame::NoFrame);
    explanation->setOpenExternalLinks(false);
    explanation->setHtml(QString::fromUtf8(read(":/mapping/overview.html")));
    explanation->setMinimumWidth(400);
    auto* variables = new QTreeWidget(split);
    variables->setObjectName("mappingDesignVariables");
    variables->setHeaderLabels({QString::fromUtf8("设计变量 / 本地路径"), QString::fromUtf8("Figma ID")});
    variables->setMinimumWidth(250);
    variables->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    variables->header()->setStretchLastSection(false);
    const auto manifest = parse(read(":/mapping/figma-variables.json"));
    for (const auto& value : manifest["variables"].toArray()) {
        const auto variable = value.toObject();
        auto* item = new QTreeWidgetItem(variables, {variable["sourcePath"].toString(), variable["id"].toString()});
        item->setToolTip(0, variable["sourcePath"].toString());
        const auto values = variable["values"].toObject();
        for (auto it = values.begin(); it != values.end(); ++it)
            new QTreeWidgetItem(item, {it.key(), it.value().toVariant().toString()});
    }
    split->addWidget(explanation);
    split->addWidget(variables);
    split->setSizes({760, 330});
    layout->addWidget(split, 1);
}
}
