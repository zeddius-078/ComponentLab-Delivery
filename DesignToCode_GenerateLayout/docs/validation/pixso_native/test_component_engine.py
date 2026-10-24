"""Self-contained generator contract tests; not proof of any published SDK API."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "native_component", ROOT / "Source/pixso_native/component_engine.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

# Synthetic contract ONLY: never used as a production mapping or second SDK.
MAPPING = {
    "schemaVersion": 1, "sdkRelease": "fixture", "profiles": ["fixture"],
    "components": [{
        "status": "verified",
        "source": {"fileKey": "fixture", "componentSetId": "fixture-family",
                   "import": "./Fixture.vue", "tag": "Fixture"},
        "qt": {"class": "FixtureWidget", "extends": "QWidget",
               "header": "FixtureWidget.h", "constructor": "parent-only"},
        "props": {"Enabled": {"default": False, "type": "bool", "method": "setEnabled"}},
        "slots": {"label": {"kind": "text", "type": "string", "method": "setText"}}
    }]
}


def node():
    return {"source": copy.deepcopy(MAPPING["components"][0]["source"])}


class ComponentTests(unittest.TestCase):
    def test_icon_is_checked_and_initialized_before_content_mode(self):
        mapping = copy.deepcopy(MAPPING)
        mapping["components"][0]["props"] = {"IconOnly": {
            "default": True, "type": "bool", "method": "setIconOnly"}}
        mapping["components"][0]["slots"]["leadingIcon"] = {"kind": "icon", "method": "setLeadingIcon"}
        mapping["components"][0]["slots"]["accessibleName"] = {
            "kind": "text", "type": "string", "method": "setAccessibleName", "phase": "before-props"}
        data = node()
        data["slots"] = {"leadingIcon": {"kind": "qt-standard", "name": "SP_FileIcon"}}
        data["slots"]["accessibleName"] = "Source label"
        engine = self.engine(mapping)
        engine.emit(data, "test")
        code = "\n".join(engine.initializers)
        self.assertIn("QStyle::SP_FileIcon", code)
        self.assertIn("COMPONENT_ICON_UNAVAILABLE", code)
        self.assertLess(code.index("->setLeadingIcon"), code.index("->setIconOnly"))
        self.assertLess(code.index("->setAccessibleName"), code.index("->setIconOnly"))
        data["slots"]["leadingIcon"]["name"] = "SP_FileIcon); evil()"
        with self.assertRaisesRegex(MODULE.ComponentError, "INVALID_ICON_VALUE"):
            self.engine(mapping).emit(data, "test")

    def engine(self, mapping=None):
        return MODULE.ComponentEngine(mapping or copy.deepcopy(MAPPING), "fixture", "fixture")

    def test_default_false_preserved(self):
        engine = self.engine()
        engine.emit(node(), "test")
        self.assertEqual(engine.initializers, ["ui->test->setEnabled(false);"])

    def test_explicit_override(self):
        engine = self.engine()
        data = node()
        data["props"] = {"Enabled": True}
        engine.emit(data, "test")
        self.assertEqual(engine.initializers, ["ui->test->setEnabled(true);"])

    def test_identity_not_tag_only(self):
        data = node()
        data["source"]["componentSetId"] = "another"
        with self.assertRaisesRegex(MODULE.ComponentError, "UNMAPPED_COMPONENT_IDENTITY"):
            self.engine().emit(data, "test")

    def test_unknown_props_and_slots_fail(self):
        for field in ("props", "slots"):
            data = node()
            data[field] = {"unknown": "value"}
            with self.subTest(field=field), self.assertRaises(MODULE.ComponentError):
                self.engine().emit(data, "test")

    def test_no_arbitrary_cpp_injection(self):
        mapping = copy.deepcopy(MAPPING)
        mapping["components"][0]["props"]["Enabled"]["method"] = "setEnabled(true); bad"
        with self.assertRaisesRegex(MODULE.ComponentError, "INVALID_CPP_IDENTIFIER"):
            self.engine(mapping).emit(node(), "test")

    def test_unicode_object_names_follow_uic_output(self):
        self.assertEqual(MODULE.identifier("px_实例_1_2_28522_abc"),
                         "px_实例_1_2_28522_abc")
        with self.assertRaisesRegex(MODULE.ComponentError, "INVALID_CPP_IDENTIFIER"):
            MODULE.identifier("px_实例 1")

    def test_text_is_length_bounded_utf8(self):
        self.assertEqual(MODULE.literal('中"\0', "string"),
                         'QString::fromUtf8("\\xe4\\xb8\\xad\\x22\\x00", 5)')

    def test_string_list_preserves_empty_and_literal_text(self):
        self.assertEqual(MODULE.literal([], "string-list"), "QStringList{}")
        self.assertEqual(MODULE.literal(["", "&nbsp;"], "string-list"),
                         'QStringList{QString::fromUtf8("", 0), '
                         'QString::fromUtf8("\\x26\\x6e\\x62\\x73\\x70\\x3b", 6)}')
        with self.assertRaises(MODULE.ComponentError):
            MODULE.literal(["valid", 1], "string-list")

    def test_collection_slots_emit_static_typed_calls(self):
        mapping = copy.deepcopy(MAPPING)
        mapping["components"][0]["slots"] = {
            "items": {"kind": "string-list", "type": "string-list", "method": "setItems"},
            "selected": {"kind": "index", "type": "int", "method": "setCurrentIndex"}}
        engine = self.engine(mapping)
        data = node()
        data["slots"] = {"items": ["A"], "selected": 0}
        engine.emit(data, "test")
        self.assertIn("ui->test->setCurrentIndex(0);", engine.initializers)
        self.assertIn('ui->test->setItems(QStringList{QString::fromUtf8("\\x41", 1)});',
                      engine.initializers)

    def test_release_and_profile_must_match(self):
        for release, profile in (("other", "fixture"), ("fixture", "other")):
            with self.assertRaises(MODULE.ComponentError):
                MODULE.ComponentEngine(MAPPING, release, profile)

    def test_contract_only_refused(self):
        mapping = copy.deepcopy(MAPPING)
        mapping["components"][0]["status"] = "contract-only"
        with self.assertRaisesRegex(MODULE.ComponentError, "NOT_VERIFIED"):
            self.engine(mapping).emit(node(), "test")

    def test_missing_default_not_guessed(self):
        mapping = copy.deepcopy(MAPPING)
        del mapping["components"][0]["props"]["Enabled"]["default"]
        with self.assertRaisesRegex(MODULE.ComponentError, "MISSING_COMPONENT_PROP"):
            self.engine(mapping).emit(node(), "test")

    def test_duplicate_target_refused(self):
        mapping = copy.deepcopy(MAPPING)
        second = copy.deepcopy(mapping["components"][0])
        second["source"]["componentSetId"] = "other"
        mapping["components"].append(second)
        with self.assertRaisesRegex(MODULE.ComponentError, "DUPLICATE_TARGET"):
            self.engine(mapping)

    def test_typed_state_transforms_emit_static_calls(self):
        mapping = copy.deepcopy(MAPPING)
        mapping["components"][0]["props"] = {"State": {"cases": {"loading": [
            {"method": "setEnabled", "type": "bool", "value": True},
            {"method": "setLoading", "type": "bool", "value": True}]}}}
        data = node()
        data["props"] = {"State": "loading"}
        engine = self.engine(mapping)
        engine.emit(data, "button")
        self.assertEqual(engine.initializers,
                         ["ui->button->setEnabled(true);", "ui->button->setLoading(true);"])
        data["props"]["State"] = "hover"
        with self.assertRaisesRegex(MODULE.ComponentError, "UNSUPPORTED_PROP_VALUE"):
            self.engine(mapping).emit(data, "button")

    def test_state_transform_is_typed_and_atomic(self):
        mapping = copy.deepcopy(MAPPING)
        mapping["components"][0]["props"] = {"State": {"default": "loading", "cases": {
            "loading": [{"method": "setEnabled", "type": "bool", "value": True},
                        {"method": "setLoading", "type": "bool", "value": "true"}]}}}
        engine = self.engine(mapping)
        with self.assertRaisesRegex(MODULE.ComponentError, "INVALID_TYPED_VALUE"):
            engine.emit(node(), "button")
        self.assertEqual(engine.initializers, [])


if __name__ == "__main__":
    unittest.main()
