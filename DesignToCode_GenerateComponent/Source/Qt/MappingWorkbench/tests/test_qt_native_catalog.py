import json
import re
import runpy
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "mapping/qt-native-catalog.json"
COMPONENT_KIT = ROOT.parent.parent / "ComponentKit"
REGISTRY = COMPONENT_KIT / "component-definitions/native-components.json"
SOURCE_MAP = COMPONENT_KIT / "component-definitions/generated-component-source-map.json"
COMPONENT_MAP = ROOT / "mapping/qt-component-map.json"
QT_REFERENCE = (ROOT.parents[2] /
                "docs/mainSkill/generate-component-pipeline/references/Qt组件类与主属性参考.md")


class QtNativeCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
        cls.components = cls.catalog["components"]

    def test_every_reference_type_is_registered_once(self):
        names = [item["qtClass"] for item in self.components]
        self.assertEqual(len(names), len(set(names)))
        self.assertEqual("5.15.2", self.catalog["source"]["qtVersion"])
        builder = runpy.run_path(str(ROOT / "mapping/tools/build_qt_catalog.py"))
        reference = QT_REFERENCE.read_text(encoding="utf-8")
        rebuilt_names = {
            item["qtClass"]
            for item in builder["enrich"](builder["parse_reference"](reference))
        }
        self.assertEqual(rebuilt_names, set(names))

    def test_visual_components_have_complete_design_and_sdk_identity(self):
        visual = [item for item in self.components if item["mappingKind"] == "visual-component"]
        registry = json.loads(REGISTRY.read_text(encoding="utf-8"))["components"]
        self.assertEqual(
            {item["qtClass"] for item in registry},
            {item["qtClass"] for item in visual},
        )
        for item in visual:
            with self.subTest(qtClass=item["qtClass"]):
                self.assertTrue(item["sdkClass"].startswith("commonWidget::Z_"))
                self.assertTrue(item["designComponent"].startswith("ComponentLab/"))
                self.assertIn(item["designReference"]["equivalence"],
                              {"direct", "adapted", "project-defined"})

    def test_every_declared_property_has_one_supported_target(self):
        targets = set(self.catalog["propertyTargets"])
        for component in self.components:
            for prop in component["declaredProperties"]:
                with self.subTest(qtClass=component["qtClass"], property=prop["name"]):
                    self.assertIn(prop["target"], targets)
                    self.assertTrue(prop["name"])
                    self.assertTrue(prop["declaredType"])

    def test_progress_bar_static_states_and_read_only_text_are_distinct_from_qt_api(self):
        progress = next(item for item in self.components if item["qtClass"] == "QProgressBar")
        self.assertEqual(["determinate", "indeterminate", "disabled"], progress["states"])
        self.assertIn("not Qt writable properties", progress["stateBoundary"])
        text = next(prop for prop in progress["declaredProperties"] if prop["name"] == "text")
        self.assertEqual("QString", text["declaredType"])
        self.assertEqual("read-only", text["target"])
        self.assertEqual("qt-5.15.2-header", text["metaVerification"])
        self.assertEqual({"system": "DSM", "component": "ComponentLab/ProgressBar",
                          "equivalence": "project-defined"}, progress["designReference"])
        builder = runpy.run_path(str(ROOT / "mapping/tools/build_qt_catalog.py"))
        reference = QT_REFERENCE.read_text(encoding="utf-8")
        rebuilt = next(item for item in builder["enrich"](builder["parse_reference"](reference))
                       if item["qtClass"] == "QProgressBar")
        self.assertEqual(progress["states"], rebuilt["states"])
        self.assertEqual(progress["stateBoundary"], rebuilt["stateBoundary"])
        self.assertEqual(progress["designReference"], rebuilt["designReference"])
        self.assertEqual(text, next(prop for prop in rebuilt["declaredProperties"]
                                    if prop["name"] == "text"))

    def test_scroll_slider_and_dial_specimens_survive_catalog_rebuild(self):
        expected = {
            "QScrollBar": ["default", "hover", "dragging", "disabled"],
            "QSlider": ["default", "focus", "dragging", "disabled"],
            "QDial": ["default", "focus", "dragging", "disabled"],
        }
        builder = runpy.run_path(str(ROOT / "mapping/tools/build_qt_catalog.py"))
        reference = QT_REFERENCE.read_text(encoding="utf-8")
        rebuilt = {item["qtClass"]: item for item in builder["enrich"](builder["parse_reference"](reference))}
        cataloged = {item["qtClass"]: item for item in self.components}
        for name, states in expected.items():
            with self.subTest(qtClass=name):
                current = cataloged[name]
                self.assertEqual(states, current["states"])
                self.assertEqual(states, rebuilt[name]["states"])
                self.assertEqual(current["stateBoundary"], rebuilt[name]["stateBoundary"])
                self.assertEqual({"system": "DSM", "component": current["designComponent"],
                                  "equivalence": "project-defined"}, current["designReference"])
                self.assertEqual(current["designReference"], rebuilt[name]["designReference"])

    def test_native_visual_identity_matches_registry_source_map_and_direct_cpp_base(self):
        registry = json.loads(REGISTRY.read_text(encoding="utf-8"))["components"]
        source_map = json.loads(SOURCE_MAP.read_text(encoding="utf-8"))["components"]
        visual = [item for item in self.components if item["mappingKind"] == "visual-component"]
        for records in (registry, source_map, visual):
            self.assertEqual(len(registry), len(records))
            self.assertEqual(len(records), len({item["qtClass"] for item in records}))

        registry_by_class = {item["qtClass"]: item for item in registry}
        source_by_class = {item["qtClass"]: item for item in source_map}
        catalog_by_class = {item["qtClass"]: item for item in visual}
        self.assertEqual(set(registry_by_class), set(source_by_class))
        self.assertEqual(set(registry_by_class), set(catalog_by_class))

        for qt_class, registered in registry_by_class.items():
            with self.subTest(qtClass=qt_class):
                mapped = source_by_class[qt_class]
                cataloged = catalog_by_class[qt_class]
                sdk_class = "commonWidget::" + registered["sdkClass"]
                design_component = "ComponentLab/" + registered["designComponent"]
                self.assertEqual(sdk_class, mapped["sdkClass"])
                self.assertEqual(sdk_class, cataloged["sdkClass"])
                self.assertEqual(design_component, mapped["designComponent"])
                self.assertEqual(design_component, cataloged["designComponent"])
                self.assertEqual(registered["implementation"], mapped["implementationKind"])
                self.assertEqual(registered["category"], mapped["category"])

                header = COMPONENT_KIT / mapped["header"]
                implementation = COMPONENT_KIT / mapped["implementation"]
                self.assertTrue(header.is_file(), str(header))
                self.assertTrue(implementation.is_file(), str(implementation))
                declaration = rf"\bclass\s+{re.escape(registered['sdkClass'])}\s*:\s*public\s+{re.escape(qt_class)}\b"
                self.assertRegex(header.read_text(encoding="utf-8"), declaration)

    def test_composite_input_is_not_conflated_with_native_line_edit(self):
        source_map = json.loads(SOURCE_MAP.read_text(encoding="utf-8"))["components"]
        by_class = {item["qtClass"]: item for item in source_map}
        self.assertEqual("ComponentLab/LineEdit", by_class["QLineEdit"]["designComponent"])
        self.assertNotIn("ComponentLab/Input", {item["designComponent"] for item in source_map})
        mapping = json.loads(COMPONENT_MAP.read_text(encoding="utf-8"))["components"]["Input"]
        self.assertEqual("input", mapping["factory"])
        self.assertEqual("commonWidget::Z_InputField", mapping["class"])
        self.assertEqual("runtime/component/Z_InputField.h", mapping["header"])
        self.assertEqual("runtime/component/Z_InputField.cpp", mapping["implementation"])
        self.assertEqual("setStatus", mapping["properties"]["status"]["api"])
        self.assertEqual("setLoading", mapping["properties"]["loading"]["api"])
        input_header = COMPONENT_KIT / "runtime/component/Z_InputField.h"
        line_edit_header = COMPONENT_KIT / by_class["QLineEdit"]["header"]
        self.assertRegex(input_header.read_text(encoding="utf-8"),
                         r"\bclass\s+Z_InputField\s*:\s*public\s+QWidget\b")
        self.assertRegex(line_edit_header.read_text(encoding="utf-8"),
                         r"\bclass\s+Z_LineEdit\s*:\s*public\s+QLineEdit\b")
        assembly = (ROOT / "mapping/QtMapping.cpp").read_text(encoding="utf-8")
        self.assertIn("auto* input = new Z_InputField(fields, layout->parentWidget());", assembly)


if __name__ == "__main__":
    unittest.main()
