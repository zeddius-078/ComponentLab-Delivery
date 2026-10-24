"""Joined pipeline tests; synthetic contracts do not certify the target page."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
import tempfile
from unittest.mock import patch
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from generate_page import attach_dsl_names, convert_graph, stage_with_record
from component_engine import ComponentError
from test_styled_layout import element


class GenerateTests(unittest.TestCase):
    def setUp(self):
        self.root = element(**{"background-color": "#fff"})
        self.graph = {"componentTree": {"children": []}, "styledTree": self.root}
        self.mapping = {"schemaVersion": 1, "sdkRelease": "fixture",
                        "profiles": ["fixture"], "components": [],
                        "pageHost": {"class": "QWidget", "header": "QWidget",
                                     "constructor": "parent-only"}}

    def convert(self):
        return convert_graph(self.graph, self.mapping, "fixture", "fixture", "fixture",
                             Path(__file__).parent / "fixtures", "Example", "root",
                             "design_1", {"width": 600, "height": 400, "source": "fixture"})

    def add_component(self):
        source = {"file": "Item.vue", "sha256": "a" * 64, "import": "Item.vue",
                  "tag": "Item", "instancePath": "root/item"}
        self.graph["componentTree"]["children"] = [
            {"kind": "component", "source": source, "props": {"Enabled": False}, "children": []}]
        child = element(20, instance="root/item", **{
            "width": "120px", "height": "40px", "box-sizing": "border-box", "flex-shrink": "0"})
        child["componentBoundaries"] = [source]
        self.root["children"] = [child]
        self.graph["componentEvidence"] = [{"source": source, "conflicts": [],
                                           "content": {"text": [], "images": []}}]
        self.mapping["components"] = [{
            "mappingKey": "fixture.item", "status": "verified",
            "source": {"fileKey": "fixture", "componentSetId": "fixture-family",
                       "import": "Item.vue", "tag": "Item", "moduleSha256": "a" * 64},
            "qt": {"class": "FixtureWidget", "extends": "QWidget",
                   "header": "Fixture.h", "constructor": "parent-only"},
            "styleContract": {"display": "flex", "flex-direction": "column"},
            "props": {"Enabled": {"type": "bool", "method": "setEnabled"}}, "slots": {}}]

    def test_five_files_and_repeatability(self):
        files, evidence = self.convert()
        self.assertEqual(len(files), 5)
        self.assertEqual(files, self.convert()[0])
        self.assertIn("not-run", evidence["acceptance"])
        self.assertIn(b"background-color: #fff", files["view/res/qss/Example.qss"])

    def test_real_instance_values_and_placement_reach_output(self):
        self.add_component()
        files, _ = self.convert()
        self.assertIn(b"->setEnabled(false);", files["src/Example.cpp"])
        ui = ET.fromstring(files["form/Example.ui"])
        widget = ui.find(".//widget[@class='FixtureWidget']")
        self.assertEqual(widget.findtext("./property[@name='minimumSize']/size/width"), "120")
        self.assertEqual(widget.findtext("./property[@name='maximumSize']/size/height"), "40")

    def test_partial_and_unconsumed_content_fail_before_output(self):
        self.add_component()
        original = copy.deepcopy(self.mapping)
        self.mapping["components"][0]["status"] = "partial"
        with self.assertRaisesRegex(ComponentError, "NOT_VERIFIED"):
            self.convert()
        self.mapping = original
        self.graph["componentEvidence"][0]["content"]["text"] = [{"text": "must not disappear"}]
        with self.assertRaisesRegex(ComponentError, "UNCONSUMED_COMPONENT_CONTENT"):
            self.convert()

    def test_unaccounted_sdk_style_not_dropped(self):
        self.add_component()
        self.root["children"][0]["style"]["values"]["color"] = "#123456"
        with self.assertRaisesRegex(ComponentError, "STYLE_CONTRACT_REQUIRED"):
            self.convert()

    def test_export_first_records_differences_and_keeps_public_api(self):
        self.add_component()
        self.mapping["generationPolicy"] = "export-first"
        self.mapping["components"][0]["status"] = "partial"
        self.graph["componentTree"]["children"][0]["props"]["Unmapped"] = "source-value"
        self.graph["componentEvidence"][0]["conflicts"] = ["DECLARED_CONTENT_CONFLICT"]
        self.graph["componentEvidence"][0]["content"]["text"] = [{"text": "unbound"}]
        self.root["children"][0]["style"]["values"]["color"] = "#123456"
        files, evidence = self.convert()
        self.assertIn(b"->setEnabled(false);", files["src/Example.cpp"])
        self.assertNotIn(b"setUnmapped", files["src/Example.cpp"])
        self.assertEqual({item["kind"] for item in evidence["warnings"]}, {
            "COMPONENT_MAPPING_DIFFERENCE", "SOURCE_CONTENT_CONFLICT",
            "SDK_APPEARANCE_USED", "UNCONSUMED_COMPONENT_CONTENT"})
        self.assertEqual(self.mapping["components"][0]["status"], "partial")
        self.assertIn("not-run", evidence["acceptance"])

    def test_export_first_still_rejects_wrong_module_hash(self):
        self.add_component()
        self.mapping["generationPolicy"] = "export-first"
        self.mapping["components"][0]["source"]["moduleSha256"] = "b" * 64
        with self.assertRaisesRegex(ComponentError, "HASH_MISMATCH"):
            self.convert()

    def test_frame_content_wrapper_gets_owner_dsl_name(self):
        with tempfile.TemporaryDirectory() as temporary:
            dsl = Path(temporary) / "dsl.json"
            dsl.write_text(json.dumps({"pixTreeNodes": [
                {"guid": "2:23506", "name": "TopframeUI", "childNode": []}]},
                ensure_ascii=False), encoding="utf-8")
            graph = {"styledTree": {"kind": "element", "attrs": {
                "class": "frame-content-2_23506"}, "source": {}, "children": []}}
            evidence = attach_dsl_names(graph, dsl, "2_23506")
        self.assertEqual(graph["styledTree"]["designName"], "TopframeUI Content")
        self.assertEqual(evidence["attachedNodes"], 1)

    def test_explicit_text_slot_reaches_static_initialization(self):
        self.add_component()
        rule = self.mapping["components"][0]
        rule["textBindings"] = {"label": {"file": "Item.vue", "slots": ["label"]}}
        rule["slots"] = {"label": {"kind": "text", "type": "string", "method": "setText"}}
        self.graph["componentEvidence"][0]["content"]["text"] = [{
            "text": "A", "slots": [{"file": "Item.vue", "name": "label",
                                   "instancePath": "root/item"}]}]
        files, _ = self.convert()
        self.assertIn(b'->setText(QString::fromUtf8("\\x41", 1));', files["src/Example.cpp"])

    def test_record_is_outside_page_and_does_not_claim_acceptance(self):
        files, evidence = self.convert()
        work = ROOT / "output/work/pixso-native"
        work.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="record-test-", dir=str(work)) as temp:
            destination = Path(temp) / "Example"
            output, record_path = stage_with_record(destination, files, evidence, {"fixture": True})
            record = json.loads(record_path.read_text(encoding="utf-8"))
            self.assertEqual(record_path.parent, output.parent)
            self.assertEqual(record["status"], "STAGED")
            self.assertEqual(record["acceptance"], "not-run")
            self.assertEqual(len(list(output.rglob("*.*"))), 5)
            for item in record["files"]:
                self.assertEqual(item["sha256"], hashlib.sha256(files[item["path"]]).hexdigest())
            with self.assertRaisesRegex(ValueError, "OUTPUT_ALREADY_EXISTS"):
                stage_with_record(destination, files, evidence, {})

    def test_failed_stage_keeps_failure_and_refuses_batch_reuse(self):
        files, evidence = self.convert()
        work = ROOT / "output/work/pixso-native"
        work.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="record-test-", dir=str(work)) as temp:
            destination = Path(temp) / "Example"
            with patch("generate_page.write_page", side_effect=OSError("injected write failure")):
                with self.assertRaises(OSError):
                    stage_with_record(destination, files, evidence, {})
            record_path = destination.with_name("Example.generation.json")
            before = record_path.read_bytes()
            self.assertEqual(json.loads(before)["status"], "FAILED")
            self.assertFalse(destination.exists())
            with self.assertRaisesRegex(ValueError, "GENERATION_RECORD_ALREADY_EXISTS"):
                stage_with_record(destination, files, evidence, {})
            self.assertEqual(record_path.read_bytes(), before)

    def test_public_child_slot_uses_typed_accessor_not_private_lookup(self):
        self.add_component()
        rule = self.mapping["components"][0]
        rule["publicChildren"] = {"search": {"accessor": "searchInput",
                                             "class": "FixtureInput", "header": "FixtureInput.h"}}
        rule["textBindings"] = {"placeholder": {"file": "Item.vue", "slots": ["search"]}}
        rule["slots"] = {"placeholder": {"kind": "text", "type": "string",
                                         "method": "setPlaceholderText", "receiver": "search"}}
        self.graph["componentEvidence"][0]["content"]["text"] = [{
            "text": "Search", "slots": [{"file": "Item.vue", "name": "search",
                                          "instancePath": "root/item"}]}]
        files, _ = self.convert()
        source = files["src/Example.cpp"]
        self.assertIn(b"#include <FixtureInput.h>", source)
        self.assertIn(b"->searchInput();", source)
        self.assertIn(b"PUBLIC_CHILD_UNAVAILABLE", source)
        self.assertNotIn(b"findChild", source)
        for accessor in ("findChild<QWidget*>", "searchInput();evil", "search->input"):
            rule["publicChildren"]["search"]["accessor"] = accessor
            with self.assertRaisesRegex(ComponentError, "IDENTIFIER"):
                self.convert()
        rule["publicChildren"] = {}
        with self.assertRaisesRegex(ComponentError, "PUBLIC_CHILD_CONTRACT_REQUIRED"):
            self.convert()


if __name__ == "__main__":
    unittest.main()
