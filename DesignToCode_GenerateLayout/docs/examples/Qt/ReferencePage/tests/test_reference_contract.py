"""Verify the handoff source without importing or running any Python Qt bindings."""
from pathlib import Path
import json
import unittest
import xml.etree.ElementTree as ET

PAGE = Path(__file__).resolve().parents[1]
ROOT = PAGE.parents[1]


class ReferenceContractTests(unittest.TestCase):
    def setUp(self):
        self.trace = json.loads((PAGE / "ReferencePage.source.json").read_text(encoding="utf-8"))
        self.ui = ET.parse(PAGE / "ReferencePage.ui")

    def test_components_and_names(self):
        # 固定结构必须直接声明真实组件，避免维护者依赖占位替换协议。
        for name, class_name in self.trace["componentObjects"].items():
            widget = self.ui.find(f".//widget[@name='{name}']")
            self.assertIsNotNone(widget, name)
            self.assertEqual(widget.get("class"), class_name)
        names = [element.get("name") for element in self.ui.iter()
                 if element.tag in ("widget", "layout", "spacer")]
        self.assertEqual(len(names), len(set(names)))
        self.assertFalse(any(name.startswith(("host_", "node_", "placeholder_")) for name in names))

    def test_source_identity_is_honest(self):
        source = json.loads((ROOT / self.trace["sourceFile"]).read_text(encoding="utf-8"))
        self.assertEqual(source["source"]["kind"], self.trace["sourceKind"])
        self.assertEqual(self.trace["sourceKind"], "localFixture")
        self.assertIsNone(self.trace["designDocumentUrl"])
        self.assertEqual(self.trace["designNodeIds"], {})

    def test_layout_values_match_resolved_bindings(self):
        # GenerateComponent resolves Token values; GenerateLayout records and consumes that snapshot.
        for binding in self.trace["layoutBindings"]:
            value = self.ui.find(f".//layout[@name='{binding['objectName']}']/"
                                 f"property[@name='{binding['property']}']/number")
            self.assertIsNotNone(value)
            self.assertEqual(int(value.text), binding["resolvedValue"], binding)

    def test_runtime_has_no_design_interpreter(self):
        # Token catalog 留在组件库；页面公开实现只负责 Qt 结构和动作。
        for path in (PAGE / "ReferencePage.cpp", PAGE / "ReferencePage.h", PAGE / "main.cpp"):
            text = path.read_text(encoding="utf-8")
            for forbidden in ("QJson", "design_mapping", "WidgetRegistry", "mount(", "setStyleSheet("):
                self.assertNotIn(forbidden, text, str(path))
        self.assertFalse((PAGE / "ui_ReferencePage.h").exists())


if __name__ == "__main__":
    unittest.main()
