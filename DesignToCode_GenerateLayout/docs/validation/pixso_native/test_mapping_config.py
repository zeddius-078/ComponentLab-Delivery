"""Public-header review only; never interpreted as a C++ compile test."""
import json
import os
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from component_engine import ComponentEngine, ComponentError, read_mapping


class MappingTests(unittest.TestCase):
    def setUp(self):
        self.mapping = read_mapping(ROOT / "Source/pixso_native/component-mapping.json")

    def test_partial_rules_cannot_emit(self):
        self.mapping["generationPolicy"] = "strict"
        engine = ComponentEngine(self.mapping, self.mapping["sdkRelease"], "standard-light")
        for rule in self.mapping["components"]:
            self.assertEqual(rule["status"], "partial")
            self.assertTrue(rule["unresolved"])
            with self.assertRaisesRegex(ComponentError, "NOT_VERIFIED"):
                engine.emit({"source": rule["source"]}, "probe")

    def test_single_target_identity(self):
        targets = [rule["qt"]["class"] for rule in self.mapping["components"]]
        self.assertEqual(len(targets), len(set(targets)))
        self.assertNotEqual(self.mapping["sourceEvidence"]["attachmentSameBatchAsLiveDesign"], True)

    def test_input_default_does_not_promote_hidden_value_to_visible_content(self):
        rule = next(rule for rule in self.mapping["components"]
                    if rule["mappingKey"] == "native.input")
        self.assertEqual(rule["status"], "partial")
        self.assertGreater(list(rule["props"]).index("State"),
                           list(rule["props"]).index("text_339_90"))
        self.assertEqual(rule["props"]["State"]["cases"]["Default"][0],
                         {"method": "setValue", "type": "string", "value": ""})
        self.assertEqual(set(rule["props"]["State"]["cases"]), {"Default"})
        self.assertEqual(rule["slots"]["placeholder"]["method"], "setPlaceholderText")
        self.assertEqual(rule["textBindings"]["placeholder"]["slots"], ["slot_339_77"])

    def test_module_fingerprints_have_separate_provenance_scope(self):
        for rule in self.mapping["components"]:
            source = rule["source"]
            self.assertRegex(source["moduleSha256"], r"^[a-f0-9]{64}$")
            self.assertEqual(source["moduleEvidence"]["familyRootId"],
                             source["componentSetId"].replace(":", "_"))
            self.assertIn(source["moduleEvidence"]["comparison"],
                          ("byte-equal", "verified-resource-url-text-only"))
            self.assertEqual(rule["status"], "partial")

    def test_sidebar_does_not_ignore_unverified_source_axes(self):
        rule = next(rule for rule in self.mapping["components"]
                    if rule["mappingKey"] == "native.sidebar")
        self.assertEqual(set(rule["props"]), {"Density", "Collapsed"})
        self.assertEqual(rule["props"]["Collapsed"]["cases"], {
            "False": [{"method": "setCollapsed", "type": "bool", "value": False}]})
        self.assertEqual(rule["status"], "partial")
        self.assertNotIn("styleContract", rule)

    def test_published_header_references(self):
        sdk = ROOT.parent / "DesignToCode_GenerateComponent/build/sdk"
        manifest_path = sdk / "manifest/component-sdk.json"
        if not manifest_path.exists():
            self.skipTest("Published SDK is not present; this is not a source failure.")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
        self.assertEqual(self.mapping["sdkRelease"], manifest["releaseId"])
        for rule in self.mapping["components"]:
            header = sdk / "include" / rule["qt"]["header"]
            self.assertTrue(header.is_file(), str(header))
            # Parent InputField APIs are declared in a separate public header.
            headers = header.read_text(encoding="utf-8-sig")
            for child in rule.get("publicChildren", {}).values():
                self.assertIn(child["accessor"] + "(", headers)
                headers += (sdk / "include" / child["header"]).read_text(encoding="utf-8-sig")
            if rule["qt"]["class"] == "designer_widgets::InputField":
                headers += (sdk / "include/MyDesignSystem/Z_InputField.h").read_text(encoding="utf-8-sig")
            if rule["qt"]["class"] in ("designer_widgets::InputField", "commonWidget::Z_PushButton"):
                qt_root = os.environ.get("QT_ROOT")
                if not qt_root:
                    self.skipTest("QT_ROOT required to inspect inherited QWidget declarations; runtime probe is separate.")
                headers += (Path(qt_root) / "include/QtWidgets/qwidget.h").read_text(encoding="utf-8-sig")
            if rule["qt"]["extends"] == "QTableView":
                qt_root = os.environ.get("QT_ROOT")
                if not qt_root:
                    self.skipTest("QT_ROOT required for inherited QTableView declarations.")
                headers += (Path(qt_root) / "include/QtWidgets/qtableview.h").read_text(encoding="utf-8-sig")
            for spec in list(rule["props"].values()) + list(rule.get("slots", {}).values()):
                operations = ([operation for case in spec["cases"].values() for operation in case]
                              if "cases" in spec else [spec])
                for operation in operations:
                    self.assertIn(operation["method"] + "(", headers)


if __name__ == "__main__":
    unittest.main()
