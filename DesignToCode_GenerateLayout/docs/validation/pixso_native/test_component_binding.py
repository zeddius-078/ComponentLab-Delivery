"""Synthetic binding contracts, never proof of production mapping readiness."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "Source/pixso_native"))
from component_binding import ComponentBinding, bind_text_slots, bind_text_lists
from component_engine import ComponentEngine, ComponentError


class BindingTests(unittest.TestCase):
    def setUp(self):
        self.source = {"file": "components/Test.vue", "sha256": "a" * 64,
                       "import": "components/Test.vue", "tag": "Test",
                       "instancePath": "root/first"}
        self.tree = {"kind": "component", "source": self.source,
                     "props": {"Enabled": False}, "children": []}
        self.rule = {
            "mappingKey": "fixture", "status": "verified",
            "source": {"fileKey": "fixture", "componentSetId": "1:2",
                       "import": "components/Test.vue", "tag": "Test",
                       "moduleSha256": "a" * 64},
            "props": {"Enabled": {"type": "bool", "method": "setEnabled"}},
            "slots": {}, "qt": {"class": "FixtureWidget", "extends": "QWidget",
                                "header": "Fixture.h", "constructor": "parent-only"}}
        self.mapping = {"schemaVersion": 1, "sdkRelease": "fixture",
                        "profiles": ["fixture"], "components": [self.rule]}

    def test_real_values_reach_typed_engine(self):
        binding = ComponentBinding(self.mapping, self.tree, "fixture")
        bound = binding.bind(self.source, {})
        engine = ComponentEngine(self.mapping, "fixture", "fixture")
        engine.emit(bound, "first")
        self.assertEqual(engine.initializers, ["ui->first->setEnabled(false);"])
        self.assertNotIn("fileKey", self.tree["source"])

    def test_missing_file_or_hash_never_inferred_from_tag(self):
        self.assertIn("SOURCE_FILE_IDENTITY_REQUIRED",
                      ComponentBinding(self.mapping, self.tree).inspect(self.source)["issues"])
        del self.rule["source"]["moduleSha256"]
        with self.assertRaisesRegex(ComponentError, "SOURCE_MODULE_BINDING_UNVERIFIED"):
            ComponentBinding(self.mapping, self.tree, "fixture").bind(self.source, {})

    def test_stale_module_and_partial_contract_rejected(self):
        self.rule["source"]["moduleSha256"] = "b" * 64
        self.rule["status"] = "partial"
        issues = ComponentBinding(self.mapping, self.tree, "fixture").inspect(self.source)["issues"]
        self.assertIn("SOURCE_MODULE_HASH_MISMATCH", issues)
        self.assertIn("COMPONENT_CONTRACT_NOT_VERIFIED", issues)

    def test_props_and_enum_failures_are_visible_together(self):
        self.tree["props"] = {"Unknown": 0, "Mode": "bad"}
        self.rule["props"]["Mode"] = {"values": {"ok": "fixture"}, "type": "string",
                                     "method": "setMode"}
        report = ComponentBinding(self.mapping, self.tree, "fixture").inspect(self.source)
        self.assertEqual(report["missingProps"], ["Enabled"])
        self.assertEqual(report["unmappedProps"], ["Unknown"])
        self.assertEqual(report["unsupportedPropValues"], ["Mode"])

    def test_boundary_and_instance_must_agree(self):
        boundary = dict(self.source, sha256="b" * 64)
        with self.assertRaisesRegex(ComponentError, "INSTANCE_BOUNDARY_MISMATCH"):
            ComponentBinding(self.mapping, self.tree).inspect(boundary)

    def test_duplicate_candidate_never_picks_first(self):
        self.mapping["components"].append(copy.deepcopy(self.rule))
        report = ComponentBinding(self.mapping, self.tree, "fixture").inspect(self.source)
        self.assertIn("AMBIGUOUS_COMPONENT_MODULE", report["issues"])

    def test_slots_require_explicit_resolution_and_props_remain_strict(self):
        binding = ComponentBinding(self.mapping, self.tree, "fixture")
        with self.assertRaisesRegex(ComponentError, "RESOLVED_SLOTS_REQUIRED"):
            binding.bind(self.source, None)
        bound = binding.bind(self.source, {"unmapped": "hello"})
        with self.assertRaisesRegex(ComponentError, "UNMAPPED_COMPONENT_SLOTS"):
            ComponentEngine(self.mapping, "fixture", "fixture").emit(bound, "first")

    def test_text_slot_is_scoped_to_owner_and_preserves_override(self):
        trace = {"file": "Item.vue", "name": "profile", "instancePath": "root/item"}
        text = {"text": "Override", "slots": [trace]}
        record = {"source": {"instancePath": "root/item"}, "content": {"text": [
            {"text": "Wrong instance", "slots": [dict(trace, instancePath="root/other")]},
            text]}}
        definitions = {"profileName": {"file": "Item.vue", "slots": ["profile"]}}
        values, consumed = bind_text_slots(record, definitions)
        self.assertEqual(values, {"profileName": "Override"})
        self.assertEqual(consumed, [text])
        record["content"]["text"].append(copy.deepcopy(text))
        with self.assertRaisesRegex(ComponentError, "TEXT_SLOT_NOT_UNIQUE"):
            bind_text_slots(record, definitions)

    def test_ordered_text_list_uses_explicit_slot_order_not_text_order(self):
        def text(value, slot):
            return {"text": value, "slots": [{"file": "Nav.vue", "name": slot,
                                               "instancePath": "root/nav"}]}
        record = {"source": {"instancePath": "root/nav"}, "content": {
            "text": [text("Second", "b"), text("First", "a"), text("Brand", "brand")]}}
        spec = {"labels": {"file": "Nav.vue", "slots": ["a", "b"]}}
        values, consumed = bind_text_lists(record, spec)
        self.assertEqual(values, {"labels": ["First", "Second"]})
        self.assertEqual(len(consumed), 2)
        spec["labels"]["slots"] = ["a", "a"]
        with self.assertRaisesRegex(ComponentError, "CONSUMED_TWICE"):
            bind_text_lists(record, spec)


if __name__ == "__main__":
    unittest.main()
