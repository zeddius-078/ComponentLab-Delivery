"""Synthetic bridge tests; fixture IDs do not represent Figma evidence."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("design_normalize", ROOT / "mapping/design/normalize.py")
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)


class DesignNormalizationTests(unittest.TestCase):
    def setUp(self):
        self.mapping = json.loads((ROOT / "mapping/qt-component-map.json").read_text(encoding="utf-8"))
        self.variables = json.loads((ROOT / "mapping/design/figma-variables.json").read_text(encoding="utf-8"))
        self.registry = {
            "schemaVersion": 1, "fileKey": self.variables["fileKey"], "mappingId": self.mapping["id"],
            "libraryVersion": "synthetic-test-only", "status": "READY", "assets": {}, "datasets": {},
            "components": {"synthetic-key": {"nodeId": "synthetic-component", "family": "Button", "bindings": {
                "Label#test": {"kind": "text", "figmaType": "TEXT", "target": "text"},
                "Size": {"kind": "enum", "figmaType": "VARIANT", "target": "controlSize", "values": {"M": "md", "XS": "xs"}},
                "State": {"kind": "state", "figmaType": "VARIANT", "values": {
                    "Default": {"disabled": False, "loading": False},
                    "Disabled": {"disabled": True, "loading": False}}}}}}}
        by_path = {v["sourcePath"]: v for v in self.variables["variables"]}
        gap = by_path["base.stoken.metric.space.vertical.standard"]
        padding = by_path["base.stoken.layout.padding.inline.compact"]
        self.node = {"id": "synthetic-instance", "name": "confirmButton", "type": "INSTANCE",
                     "mainComponent": {"id": "synthetic-component", "key": "synthetic-key"},
                     "visualOverrides": [], "componentProperties": {
                         "Label#test": {"type": "TEXT", "value": "Confirm"},
                         "Size": {"type": "VARIANT", "value": "M"},
                         "State": {"type": "VARIANT", "value": "Default"}}}
        root = {"id": "synthetic-frame", "name": "Example", "type": "FRAME", "layoutMode": "VERTICAL",
                "absoluteChildren": False, "boundVariables": {}, "children": [self.node]}
        for key, variable in {"itemSpacing": gap, **{k: padding for k in
                            ("paddingLeft", "paddingTop", "paddingRight", "paddingBottom")}}.items():
            root[key] = variable["values"]["light-comfortable"]
            root["boundVariables"][key] = {"type": "VARIABLE_ALIAS", "id": variable["id"]}
        self.snapshot = {"schemaVersion": 1, "provider": "figma", "fileKey": self.variables["fileKey"],
                         "profile": "light-comfortable", "root": root}

    def compile(self):
        return bridge.normalize(json.dumps(self.snapshot).encode(), self.registry, self.variables, self.mapping)

    def test_deterministic_typed_assembly(self):
        result = self.compile()
        self.assertEqual(result, self.compile())
        self.assertEqual(result["source"]["kind"], "figmaNormalized")
        self.assertEqual(len(result["source"]["rawSha256"]), 64)
        node = result["page"]["children"][0]
        self.assertEqual(node["properties"], {"text": "Confirm", "controlSize": "md", "disabled": False, "loading": False})
        self.assertEqual(node["design"]["componentKey"], "synthetic-key")
        self.assertFalse(result["source"]["reviewed"])

    def test_neutral_tree_keeps_frame_and_variable_bindings(self):
        tree = bridge.design_tree(json.dumps(self.snapshot).encode())
        root, child = tree["nodes"]
        self.assertEqual(root["nodeId"], "synthetic-frame")
        self.assertEqual(child["parentId"], root["nodeId"])
        self.assertEqual(root["tokenBindings"], self.snapshot["root"]["boundVariables"])
        self.assertEqual(child["componentRef"]["nodeId"], self.node["mainComponent"]["id"])
        self.assertEqual(child["componentRef"]["key"], self.node["mainComponent"]["key"])
        self.assertTrue(all(v is None for v in root["bounds"].values()))

    def test_false_and_xs_are_preserved(self):
        self.node["componentProperties"]["Size"]["value"] = "XS"
        node = self.compile()["page"]["children"][0]
        self.assertFalse(node["properties"]["disabled"])
        self.assertIn("semantic.dimension.control.height-xs", node["tokenRefs"])

    def test_input_select_clear_is_opt_in_for_design_mapping(self):
        # Figma DSM/Input Allow clear and DSM/Select Show clear both default to false.
        for family in ("Input", "Select"):
            with self.subTest(family=family):
                spec = self.mapping["components"][family]["properties"]["allowClear"]
                self.assertIs(spec["default"], False)
                self.assertEqual(spec["api"], "setAllowClear")

    def test_checkbox_design_default_size_is_large(self):
        # DSM/Checkbox 235:2710 defaults to LG; Z_CheckBox's constructor default is separate.
        spec = self.mapping["components"]["Checkbox"]["properties"]["controlSize"]
        self.assertEqual(spec["default"], "lg")
        self.assertEqual(spec["api"], "setControlSize")

    def test_text_button_design_default_size_is_small(self):
        # The six DSM/Button text sets default to small; IconOnly remains a separate open case.
        spec = self.mapping["components"]["Button"]["properties"]["controlSize"]
        self.assertEqual(spec["default"], "sm")
        self.assertEqual(spec["api"], "setSize")

    def test_input_select_design_default_size_is_small(self):
        # DSM/Input and the twelve public DSM/Select appearance sets default to Small.
        for family in ("Input", "Select"):
            with self.subTest(family=family):
                spec = self.mapping["components"][family]["properties"]["controlSize"]
                self.assertEqual(spec["default"], "sm")
                self.assertEqual(spec["api"], "setControlSize")

    def test_state_is_decomposed(self):
        self.node["componentProperties"]["State"]["value"] = "Disabled"
        self.assertTrue(self.compile()["page"]["children"][0]["properties"]["disabled"])

    def test_pending_registry_fails(self):
        self.registry["status"] = "PENDING_DESIGN_COMPONENTS"
        with self.assertRaisesRegex(ValueError, "not READY"): self.compile()

    def test_wrong_file_fails(self):
        self.snapshot["fileKey"] = "different-file"
        with self.assertRaisesRegex(ValueError, "identity mismatch"): self.compile()

    def test_name_cannot_substitute_identity(self):
        self.node["mainComponent"]["key"] = "unknown"
        with self.assertRaisesRegex(ValueError, "unregistered"): self.compile()

    def test_property_drift_fails(self):
        self.node["componentProperties"]["NewProperty"] = {"type": "TEXT", "value": "unknown"}
        with self.assertRaisesRegex(ValueError, "property set changed"): self.compile()

    def test_unknown_state_fails(self):
        self.node["componentProperties"]["State"]["value"] = "Hover"
        with self.assertRaisesRegex(ValueError, "unsupported design state"): self.compile()

    def test_visual_override_fails(self):
        self.node["visualOverrides"] = [{"id": "synthetic-instance", "field": "fills"}]
        with self.assertRaisesRegex(ValueError, "visual override"): self.compile()

    def test_missing_override_check_fails(self):
        del self.node["visualOverrides"]
        with self.assertRaisesRegex(ValueError, "override check"): self.compile()

    def test_layout_binding_is_required(self):
        self.snapshot["root"]["boundVariables"].clear()
        with self.assertRaisesRegex(ValueError, "expected Token"): self.compile()

    def test_layout_value_must_match_binding(self):
        self.snapshot["root"]["itemSpacing"] = 777
        with self.assertRaisesRegex(ValueError, "resolved value mismatch"): self.compile()

    def test_duplicate_names_fail(self):
        self.snapshot["root"]["children"].append(copy.deepcopy(self.node))
        with self.assertRaisesRegex(ValueError, "Duplicate object name"): self.compile()

    def test_detached_frame_is_rejected(self):
        self.node["type"] = "RECTANGLE"
        with self.assertRaisesRegex(ValueError, "component instance"): self.compile()

    def test_boolean_is_not_string(self):
        del self.registry["components"]["synthetic-key"]["bindings"]["State"]
        del self.node["componentProperties"]["State"]
        self.registry["components"]["synthetic-key"]["bindings"]["Disabled#test"] = {
            "kind": "boolean", "figmaType": "BOOLEAN", "target": "disabled"}
        self.node["componentProperties"]["Disabled#test"] = {"type": "BOOLEAN", "value": "false"}
        with self.assertRaisesRegex(ValueError, "typed value"): self.compile()

    def test_unexposed_api_is_rejected(self):
        self.registry["components"]["synthetic-key"]["bindings"]["Label#test"]["target"] = "arbitraryStyleSheet"
        with self.assertRaisesRegex(ValueError, "not exposed"): self.compile()


if __name__ == "__main__":
    unittest.main()
