"""Generate a public API constructor fixture, NOT a converted design page."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from page_output import render_page, write_page


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--table-inspection", type=Path)
    parser.add_argument("--table-instance")
    parser.add_argument("--navbar-inspection", type=Path)
    parser.add_argument("--navbar-instance")
    args = parser.parse_args()
    manifest = json.loads((args.sdk / "manifest/component-sdk.json").read_text(encoding="utf-8-sig"))
    definitions = [
        ("button", "commonWidget::Z_PushButton", "QPushButton", "Z_PushButton.h",
         {"Text": {"method": "setText", "type": "string", "default": "Constructor probe"}}),
        ("input", "designer_widgets::InputField", "QWidget", "DesignerWidgets.h",
         {"Value": {"method": "setValue", "type": "string", "default": "probe value"}}),
        ("navbar", "commonWidget::Z_Navbar", "QWidget", "Z_DesignComponents.h",
         {"Name": {"method": "setProfileName", "type": "string", "default": "Probe"}}),
        ("sidebar", "commonWidget::Z_Sidebar", "QWidget", "Z_DesignComponents.h",
         {"Collapsed": {"method": "setCollapsed", "type": "bool", "default": False}}),
        ("bottom", "commonWidget::Z_DesktopBottomNavigation", "QWidget", "Z_DesignComponents.h",
         {"Placement": {"method": "setIconPlacement", "type": "string", "default": "Top"}}),
        ("table", "commonWidget::Z_TableView", "QTableView", "native/Z_TableView.h",
         {"Density": {"method": "setDensity", "type": "string", "default": "compact"}}),
    ]
    rules, children = [], []
    for name, class_name, base, header, props in definitions:
        identity = {"fileKey": "synthetic-public-api-probe", "componentSetId": name,
                    "import": "fixture/" + name, "tag": name}
        if not (args.sdk / "include/MyDesignSystem" / header).is_file():
            raise RuntimeError("PUBLIC_HEADER_MISSING: " + header)
        rules.append({"status": "verified", "source": identity,
                      "qt": {"class": class_name, "extends": base,
                             "header": "MyDesignSystem/" + header, "constructor": "parent-only"},
                      "props": props, "slots": {}})
        children.append({"kind": "component", "name": name, "source": identity, "props": {}})
    if bool(args.table_inspection) != bool(args.table_instance):
        raise ValueError("TABLE_INSPECTION_AND_INSTANCE_REQUIRED_TOGETHER")
    if args.table_inspection:
        report = json.loads(args.table_inspection.read_text(encoding="utf-8"))
        matches = [item for item in report["componentContent"]
                   if item["source"]["instancePath"] == args.table_instance]
        if len(matches) != 1 or "resolvedTableData" not in matches[0]:
            raise ValueError("TABLE_INSPECTION_NOT_UNIQUE_OR_RESOLVED")
        rules[-1]["slots"] = {"sampleData": {"kind": "table-model", "method": "setModel", "readOnly": True}}
        children[-1]["slots"] = {"sampleData": matches[0]["resolvedTableData"]}
    if bool(args.navbar_inspection) != bool(args.navbar_instance):
        raise ValueError("NAVBAR_INSPECTION_AND_INSTANCE_REQUIRED_TOGETHER")
    if args.navbar_inspection:
        report = json.loads(args.navbar_inspection.read_text(encoding="utf-8"))
        matches = [item for item in report["componentContent"]
                   if item["source"]["instancePath"] == args.navbar_instance]
        if len(matches) != 1 or "searchPlaceholder" not in matches[0].get("resolvedTextSlots", {}):
            raise ValueError("NAVBAR_INSPECTION_NOT_UNIQUE_OR_RESOLVED")
        rules[2]["publicChildren"] = {"search": {"accessor": "searchInput",
            "class": "commonWidget::Z_InputField", "header": "MyDesignSystem/Z_InputField.h"}}
        rules[2]["slots"] = {"searchPlaceholder": {"kind": "text", "type": "string",
            "method": "setPlaceholder", "receiver": "search"}}
        children[2]["slots"] = {"searchPlaceholder": matches[0]["resolvedTextSlots"]["searchPlaceholder"]}
    mapping = {"schemaVersion": 1, "sdkRelease": manifest["releaseId"],
               "profiles": ["standard-light"], "components": rules,
               "pageHost": {"class": "designer_widgets::ThemeHost",
                            "header": "MyDesignSystem/DesignerWidgets.h",
                            "constructor": "profile-parent"}}
    def container(name, direction, contents):
        return {"kind": "container", "name": name, "source": {"fixture": "public-api-constructor"},
                "css": {"display": "flex", "flex-direction": direction, "gap": 4}, "children": contents}
    root = container("root", "column", [
        container("upper", "row", [container("fields", "column", children[:2]), children[2]]),
        container("lower", "row", children[3:])])
    files = render_page("SDKConstructorProbe", root, mapping, manifest["releaseId"], "standard-light")
    output = write_page(args.output, files)
    print(json.dumps({"scope": "synthetic public API fixture, not production mapping",
                      "page": str(output), "sdkRelease": manifest["releaseId"], "classes": len(rules)}))


if __name__ == "__main__":
    main()
