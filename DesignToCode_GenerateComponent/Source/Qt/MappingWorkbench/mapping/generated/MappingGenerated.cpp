#include "MappingGenerated.h"
#include "ui_MappingGenerated.h"
#include "QtMapping.h"
MappingGenerated::MappingGenerated(const QString& profile, QWidget* parent) : QWidget(parent) {
    Ui::MappingGenerated ui;
    ui.setupUi(this);
    const auto contract = design_mapping::parse(QByteArray(R"QTM0(
{
    "mappingId": "component-lab-qt-mapping-v1",
    "page": {
        "children": [
            {
                "component": "Input",
                "id": "lotInput",
                "properties": {
                    "allowClear": true,
                    "controlSize": "md",
                    "label": "批次编号",
                    "placeholder": "输入批次编号",
                    "value": "LOT-2026-0917"
                },
                "tokenRefs": [
                    "component.basic.input.size.md.height",
                    "component.basic.input.state.default.background"
                ]
            },
            {
                "component": "Select",
                "id": "chamberSelect",
                "properties": {
                    "controlSize": "md",
                    "hint": "维护中的腔体不可选择",
                    "label": "目标腔体",
                    "options": [
                        {
                            "disabled": false,
                            "label": "刻蚀腔体 ETCH-01",
                            "value": "etch-01"
                        },
                        {
                            "disabled": false,
                            "label": "刻蚀腔体 ETCH-02",
                            "value": "etch-02"
                        },
                        {
                            "disabled": true,
                            "label": "维护中 ETCH-03",
                            "value": "etch-03"
                        }
                    ],
                    "values": [
                        "etch-01"
                    ]
                },
                "tokenRefs": [
                    "component.basic.input.size.md.height",
                    "component.basic.input.radius"
                ]
            },
            {
                "component": "Checkbox",
                "id": "qualityCheck",
                "properties": {
                    "checked": true,
                    "controlSize": "sm",
                    "text": "启用质量检查"
                },
                "tokenRefs": [
                    "component.basic.checkbox.radius"
                ]
            },
            {
                "children": [
                    {
                        "component": "Button",
                        "id": "submitButton",
                        "properties": {
                            "controlSize": "md",
                            "text": "确认操作",
                            "trailingIcon": "arrow-right",
                            "type": "primary"
                        },
                        "tokenRefs": [
                            "component.basic.button.size.md.height"
                        ]
                    },
                    {
                        "component": "IconButton",
                        "id": "searchButton",
                        "properties": {
                            "accessibleName": "查询批次",
                            "controlSize": "md",
                            "leadingIcon": "search",
                            "type": "default"
                        },
                        "tokenRefs": [
                            "component.basic.button.size.md.height"
                        ]
                    }
                ],
                "component": "Row",
                "id": "actions",
                "layout": {
                    "margin": "base.stoken.layout.padding.inline.compact",
                    "spacing": "base.stoken.metric.space.vertical.standard"
                }
            }
        ],
        "component": "Column",
        "id": "MappingExample",
        "layout": {
            "margin": "base.stoken.layout.padding.inline.compact",
            "spacing": "base.stoken.metric.space.vertical.standard"
        }
    },
    "schemaVersion": 1,
    "source": {
        "documentUrl": "",
        "kind": "localFixture",
        "nodeId": "",
        "rawSha256": "",
        "reviewed": false
    }
}
)QTM0"));
    design_mapping::mount(this, contract, profile);
    setStyleSheet(design_mapping::pageStyle());
}
QWidget* MappingGenerated::mappedControl(const QString& id) const { return findChild<QWidget*>(id); }
