"""Generate the public Figma component catalog consumed by both demo applications.

The current Figma ledger is the design evidence source. Native Qt construction remains
owned by native-components.json; this catalog must not turn a Figma-only contract into a
runtime implementation claim.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parents[1]
LEDGER = PROJECT / "docs/designRules/design-system/figma-core-rebuild-state.json"
NATIVE_DEFINITIONS = ROOT / "component-definitions/native-components.json"
OUTPUT_JSON = ROOT / "component-definitions/generated-design-component-catalog.json"
OUTPUT_HEADER = ROOT / "runtime/component/DesignComponentCatalog.h"
OUTPUT_SOURCE = ROOT / "runtime/component/DesignComponentCatalog.cpp"
FIGMA_INVENTORY = ROOT / "component-definitions/figma-component-inventory.json"

FIGMA_FILE_KEY = "w92BKW1EcutKmTXkwzNHbf"
EXPECTED_PUBLIC_COMPONENTS = 85

CATEGORY_ORDER = [
    ("navigation", "导航"),
    ("display", "展示与编辑"),
    ("actions", "操作"),
    ("input", "输入"),
    ("selection", "选择"),
    ("container", "容器与布局"),
    ("data", "数据视图"),
    ("feedback", "反馈"),
    ("window", "窗口与系统"),
]

CATEGORY_COMPONENTS = {
    "navigation": {
        "Navbar", "Sidebar", "Menu", "MenuBar", "ToolBar", "TabWidget",
        "ToolBox", "DockWidget", "MdiArea", "MdiSubWindow", "StackedWidget",
        "WizardPage", "Wizard", "DesktopBottomNavigation",
    },
    "display": {
        "MetricCard", "Chart", "ImagePanel", "ProgressBar", "Label", "LCDNumber",
        "TextBrowser", "GraphicsView", "Calendar", "RichTextEdit", "PlainTextEdit",
        "OpenGLViewport", "Empty", "Statistic", "Descriptions",
    },
    "actions": {"Button", "ToolButton", "DialogButtonGroup", "CommandLinkButton"},
    "input": {
        "Input", "LineEdit", "SpinBox", "DoubleSpinBox", "DateTimePicker",
        "DatePicker", "TimePicker", "ShortcutInput", "Slider", "ScrollBar", "Dial",
        "Form",
    },
    "selection": {
        "Checkbox", "RadioButton", "Switch", "Chip", "Select", "ComboBox",
        "FontComboBox",
    },
    "container": {
        "Card", "IntegrationCard", "Splitter", "GroupBox", "ScrollArea", "Widget",
        "Frame", "Divider", "Collapse", "OverlayDrawer",
    },
    "data": {
        "DataTable", "TableWidget", "ListView", "ListWidget", "TreeView",
        "TreeWidget", "TableView", "ColumnView", "UndoView",
    },
    "feedback": {"MessageDialog", "Alert", "Message", "Notification", "Spin", "Result", "Skeleton"},
    "window": {
        "Dialog", "StatusBar", "FileDialog", "ColorDialog", "FontDialog",
        "InputDialog", "ProgressDialog",
    },
}

SINGLE_NAME_OVERRIDES = {
    "progress": "ProgressBar",
    "radio": "RadioButton",
    "tabs": "TabWidget",
    "toolbox": "ToolBox",
}

GROUP_VARIANT_OVERRIDES = {
    "MdiArea": 4,
    "MdiSubWindow": 6,
    "StackedWidget": 3,
    "WizardPage": 6,
    "Wizard": 16,
    "Widget": 6,
    "Frame": 24,
    "Dialog": 12,
    "MessageDialog": 10,
}

# Project-owned runtime classes for Figma components that do not have a
# one-to-one native Qt widget. Their public class name follows the same stable
# ComponentLab/<Name> -> commonWidget::Z_<Name> rule as the design contract.
DESIGN_RUNTIME_BASES = {
    "ComponentLab/Navbar": "QWidget composition",
    "ComponentLab/Sidebar": "QWidget composition",
    "ComponentLab/DesktopBottomNavigation": "QWidget composition",
    "ComponentLab/MetricCard": "QWidget",
    "ComponentLab/Chart": "QWidget",
    "ComponentLab/ImagePanel": "QWidget",
    "ComponentLab/Empty": "QWidget",
    "ComponentLab/Statistic": "QWidget",
    "ComponentLab/Descriptions": "QGridLayout composition",
    "ComponentLab/Form": "QFormLayout composition",
    "ComponentLab/Switch": "QAbstractButton",
    "ComponentLab/Chip": "QAbstractButton",
    "ComponentLab/Card": "QWidget",
    "ComponentLab/IntegrationCard": "QWidget",
    "ComponentLab/Divider": "QWidget",
    "ComponentLab/Collapse": "QWidget",
    "ComponentLab/OverlayDrawer": "QWidget",
    "ComponentLab/DataTable": "QTableWidget",
    "ComponentLab/Alert": "QWidget",
    "ComponentLab/Message": "QWidget",
    "ComponentLab/Notification": "QWidget",
    "ComponentLab/Spin": "QWidget",
    "ComponentLab/Result": "QWidget",
    "ComponentLab/Skeleton": "QWidget",
}


def lower_first(value: str) -> str:
    return value[:1].lower() + value[1:]


def category_for(name: str) -> str:
    matches = [category for category, names in CATEGORY_COMPONENTS.items() if name in names]
    assert len(matches) == 1, (name, matches)
    return matches[0]


def native_design_for_key(key: str, native_by_design: dict[str, dict]) -> str | None:
    override = SINGLE_NAME_OVERRIDES.get(key)
    if override and f"ComponentLab/{override}" in native_by_design:
        return f"ComponentLab/{override}"
    for design_id in native_by_design:
        if design_id.rsplit("/", 1)[-1].lower() == key.lower():
            return design_id
    return None


def record_design_id(key: str, record: dict, native_by_design: dict[str, dict]) -> str:
    for field in ("designComponent", "publicComponentSetName", "publicDesignComponent"):
        value = record.get(field)
        if isinstance(value, str) and value.startswith("ComponentLab/"):
            return value
    native = native_design_for_key(key, native_by_design)
    if record.get("mappingType") == "native" and native:
        return native
    name = SINGLE_NAME_OVERRIDES.get(key, key[:1].upper() + key[1:])
    return f"ComponentLab/{name}"


def group_variant_count(record: dict, name: str) -> int:
    if name in GROUP_VARIANT_OVERRIDES:
        return GROUP_VARIANT_OVERRIDES[name]
    field = lower_first(name) + "VariantCount"
    if field in record:
        return int(record[field])
    for value in record.get("componentSets", {}).values():
        if isinstance(value, dict) and value.get("componentSetId"):
            # Match the component-set id in the classification rather than a display label.
            return -1
    raise AssertionError((name, "missing grouped variant count"))


def runtime_identity(design_id: str, mapping_kind: str, native_by_design: dict[str, dict]):
    name = design_id.rsplit("/", 1)[-1]
    if mapping_kind == "native":
        native = native_by_design[design_id]
        return native["qtClass"], f'commonWidget::{native["sdkClass"]}', True
    if design_id == "ComponentLab/Input":
        return "QWidget composition", "commonWidget::Z_InputField", True
    if design_id == "ComponentLab/Select":
        return "QWidget composition", "commonWidget::Z_Select", True
    if design_id in DESIGN_RUNTIME_BASES:
        return DESIGN_RUNTIME_BASES[design_id], f'commonWidget::Z_{name}', True
    return "", f"commonWidget::Z_{name}", False


def grouped_components(record: dict, native_by_design: dict[str, dict]) -> list[dict]:
    output = []
    component_sets = record.get("componentSets", {})
    for asset in record.get("assetClassifications", []):
        if asset["assetClass"] == "engineering-contract":
            continue
        design_id = asset["designComponentId"]
        name = design_id.rsplit("/", 1)[-1]
        count = group_variant_count(record, name)
        if count < 0:
            lookup = lower_first(name)
            entry = component_sets.get(lookup)
            assert entry, (name, component_sets.keys())
            count = int(entry["variantCount"])
        mapping_kind = asset["assetClass"].replace("-component", "")
        qt_class, sdk_class, instantiable = runtime_identity(
            design_id, mapping_kind, native_by_design
        )
        output.append({
            "designComponent": design_id,
            "sdkClass": sdk_class,
            "qtClass": qt_class or asset.get("qtCompositionBasis", ""),
            "mappingKind": mapping_kind,
            "category": category_for(name),
            "pageId": record["pageId"],
            "componentSetId": asset["componentSetId"],
            "variantCount": count,
            "instantiable": instantiable,
            "figmaStatus": "verified",
            "approvalStatus": "not-approved",
            "runtimeStatus": "implemented" if instantiable else "contract-only",
        })
    return output


def build_catalog() -> list[dict]:
    ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
    native_contract = json.loads(NATIVE_DEFINITIONS.read_text(encoding="utf-8"))
    native_by_design = {
        f'ComponentLab/{item["designComponent"]}': item
        for item in native_contract["components"]
    }
    result = []
    for key, record in ledger["entities"]["componentPages"].items():
        if record.get("recordKind") == "component-specimen":
            continue
        if record.get("assetClassifications"):
            result.extend(grouped_components(record, native_by_design))
            continue
        design_id = record_design_id(key, record, native_by_design)
        mapping_kind = record.get("mappingType")
        assert mapping_kind in {"native", "design-system", "composite"}, (key, mapping_kind)
        qt_class, sdk_class, instantiable = runtime_identity(
            design_id, mapping_kind, native_by_design
        )
        component_set_ids = record.get("componentSetIds", [])
        component_set_id = record.get("componentSetId", "")
        if not component_set_id and component_set_ids:
            component_set_id = ",".join(component_set_ids)
        status_text = record.get("status", "")
        result.append({
            "designComponent": design_id,
            "sdkClass": sdk_class,
            "qtClass": qt_class,
            "mappingKind": mapping_kind,
            "category": category_for(design_id.rsplit("/", 1)[-1]),
            "pageId": record["pageId"],
            "componentSetId": component_set_id,
            "variantCount": int(record.get("variantCount", 0)),
            "instantiable": instantiable,
            "figmaStatus": "verified",
            "approvalStatus": "not-approved",
            "runtimeStatus": (
                "implemented" if instantiable else
                "implementation-blocked" if "BLOCKED" in status_text else "contract-only"
            ),
        })

    # Public Qt identity and approval come from the governance ledger. Current
    # source set IDs/counts come from the live Figma inventory, never an older
    # prose/ledger count (ProgressBar and both SpinBoxes have grown since then).
    inventory = load_figma_inventory()
    pages = {page["id"]: page for page in inventory["pages"]}
    sets = {item["id"]: item for page in inventory["pages"] for item in page["componentSets"]}
    for item in result:
        ids = [node_id for node_id in item["componentSetId"].split(",") if node_id]
        if not ids:
            owners = [other for other in result if other["pageId"] == item["pageId"]]
            assert len(owners) == 1, (item["designComponent"], "ambiguous source ownership")
            # Menu/MenuBar/TabWidget/ToolBox expose item anatomy sets, not new
            # public widget classes. Select owns its shell/anatomy/popup sets.
            ids = [source["id"] for source in pages[item["pageId"]]["componentSets"]]
            item["componentSetId"] = ",".join(ids)
        assert ids and all(node_id in sets for node_id in ids), item
        item["variantCount"] = sum(len(sets[node_id]["variants"]) for node_id in ids)

    order = {category: index for index, (category, _) in enumerate(CATEGORY_ORDER)}
    result.sort(key=lambda item: (order[item["category"]], item["pageId"], item["designComponent"]))
    assert len(result) == EXPECTED_PUBLIC_COMPONENTS, len(result)
    assert len({item["designComponent"] for item in result}) == len(result)
    assert len({item["sdkClass"] for item in result}) == len(result)
    assert sum(item["mappingKind"] == "native" for item in result) == 59
    assert sum(item["mappingKind"] == "design-system" for item in result) == 6
    assert sum(item["mappingKind"] == "composite" for item in result) == 20
    assert sum(item["instantiable"] for item in result) == EXPECTED_PUBLIC_COMPONENTS
    assert {item["category"] for item in result} == {item[0] for item in CATEGORY_ORDER}
    return result


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def load_figma_inventory() -> dict:
    inventory = json.loads(FIGMA_INVENTORY.read_text(encoding="utf-8"))
    pages = inventory["pages"]
    component_sets = [item for page in pages for item in page["componentSets"]]
    variants = [variant for item in component_sets for variant in item["variants"]]
    assert len(pages) == inventory["source"]["pageCount"] == 73
    assert len(component_sets) == inventory["source"]["componentSetCount"] == 123
    assert len(variants) == inventory["source"]["variantCount"] == 1471
    assert [item["id"] for item in inventory["categories"]] == [
        f"20.{index}" for index in range(1, 10)
    ]
    return inventory


def cpp_qstring(value: str) -> str:
    return f'QString::fromUtf8("{cpp_string(value)}")'


def cpp_qstring_list(values: list[str]) -> str:
    return "{" + ", ".join(cpp_qstring(value) for value in values) + "}"


def header_text() -> str:
    return '''#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace commonWidget {

// Public design inventory generated from the current Figma component ledger.
// instantiable=false means the design contract exists but no runtime class is claimed.
struct DesignComponentDescriptor {
    QString designComponent;
    QString sdkClass;
    QString qtClass;
    QString mappingKind;
    QString category;
    QString pageId;
    QString componentSetId;
    int variantCount;
    bool instantiable;
    QString figmaStatus;
    QString approvalStatus;
    QString runtimeStatus;
};

struct DesignComponentCategory {
    QString id;
    QString label;
};

struct DesignVariantAxis {
    QString name;
    QStringList values;
};

struct DesignComponentSet {
    QString id;
    QString name;
    QList<DesignVariantAxis> variantAxes;
    QStringList variants;
};

struct DesignComponentPage {
    QString id;
    QString name;
    QString categoryId;
    QList<DesignComponentSet> componentSets;
};

QList<DesignComponentCategory> designComponentCategories();
QList<DesignComponentDescriptor> designComponentDescriptors();
QList<DesignComponentCategory> figmaComponentCategories();
QList<DesignComponentPage> figmaComponentPages();

} // namespace commonWidget
'''


def source_text(components: list[dict], inventory: dict) -> str:
    categories = "\n".join(
        f'        {{QStringLiteral("{cpp_string(key)}"), QString::fromUtf8("{cpp_string(label)}")}},'
        for key, label in CATEGORY_ORDER
    )
    rows = "\n".join(
        "        {"
        + ", ".join([
            f'QStringLiteral("{cpp_string(item["designComponent"])}")',
            f'QStringLiteral("{cpp_string(item["sdkClass"])}")',
            f'QStringLiteral("{cpp_string(item["qtClass"])}")',
            f'QStringLiteral("{cpp_string(item["mappingKind"])}")',
            f'QStringLiteral("{cpp_string(item["category"])}")',
            f'QStringLiteral("{cpp_string(item["pageId"])}")',
            f'QStringLiteral("{cpp_string(item["componentSetId"])}")',
            str(item["variantCount"]),
            "true" if item["instantiable"] else "false",
            f'QStringLiteral("{cpp_string(item["figmaStatus"])}")',
            f'QStringLiteral("{cpp_string(item["approvalStatus"])}")',
            f'QStringLiteral("{cpp_string(item["runtimeStatus"])}")',
        ])
        + "},"
        for item in components
    )
    figma_categories = "\n".join(
        f"        {{{cpp_qstring(item['id'])}, {cpp_qstring(item['name'])}}},"
        for item in inventory["categories"]
    )
    figma_pages = []
    for page in inventory["pages"]:
        component_sets = []
        for component_set in page["componentSets"]:
            axes = ", ".join(
                "{" + cpp_qstring(name) + ", " + cpp_qstring_list(values) + "}"
                for name, values in component_set["variantAxes"].items()
            )
            component_sets.append(
                "{" + ", ".join([
                    cpp_qstring(component_set["id"]),
                    cpp_qstring(component_set["name"]),
                    "{" + axes + "}",
                    cpp_qstring_list(component_set["variants"]),
                ]) + "}"
            )
        figma_pages.append(
            "        {" + ", ".join([
                cpp_qstring(page["id"]),
                cpp_qstring(page["name"]),
                cpp_qstring(page["categoryId"]),
                "{" + ", ".join(component_sets) + "}",
            ]) + "},"
        )
    figma_page_rows = "\n".join(figma_pages)
    return f'''#include "DesignComponentCatalog.h"

namespace commonWidget {{

QList<DesignComponentCategory> designComponentCategories() {{
    return {{
{categories}
    }};
}}

QList<DesignComponentDescriptor> designComponentDescriptors() {{
    return {{
{rows}
    }};
}}

QList<DesignComponentCategory> figmaComponentCategories() {{
    return {{
{figma_categories}
    }};
}}

QList<DesignComponentPage> figmaComponentPages() {{
    return {{
{figma_page_rows}
    }};
}}

}} // namespace commonWidget
'''


def expected_outputs() -> dict[Path, str]:
    components = build_catalog()
    inventory = load_figma_inventory()
    payload = {
        "schemaVersion": 1,
        "owner": "DesignToCode_GenerateComponent",
        "figmaFileKey": FIGMA_FILE_KEY,
        "scope": "85 public components; excludes Contract/LayoutPrimitive and the Button inner-shadow material specimen",
        "namingRules": {
            "native": "Q<Class> -> commonWidget::Z_<Class>",
            "designSystemAndComposite": "ComponentLab/<Name> -> commonWidget::Z_<Name>",
            "compatibility": {"ComponentLab/Input": "commonWidget::Z_InputField"},
        },
        "componentCount": len(components),
        "instantiableCount": sum(item["instantiable"] for item in components),
        "contractOnlyCount": sum(not item["instantiable"] for item in components),
        "categories": [{"id": key, "label": label} for key, label in CATEGORY_ORDER],
        "components": components,
        "figmaSource": inventory["source"],
        "figmaCategories": inventory["categories"],
        "figmaPages": inventory["pages"],
    }
    return {
        OUTPUT_JSON: json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        OUTPUT_HEADER: header_text(),
        OUTPUT_SOURCE: source_text(components, inventory),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    drift = []
    outputs = expected_outputs()
    for path, content in outputs.items():
        if args.check:
            if not path.is_file() or path.read_text(encoding="utf-8") != content:
                drift.append(str(path.relative_to(ROOT)))
            continue
        if path.is_file() and path.read_text(encoding="utf-8") == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
    if drift:
        raise SystemExit("Generated Figma component catalog drift:\n" + "\n".join(drift))
    print(json.dumps({
        "components": EXPECTED_PUBLIC_COMPONENTS,
        "figmaPages": len(load_figma_inventory()["pages"]),
        "outputs": len(outputs),
        "mode": "check" if args.check else "write",
    }))


if __name__ == "__main__":
    main()
