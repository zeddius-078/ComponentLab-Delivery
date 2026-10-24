"""Cross-check the provisional DSM name/import contract, not the Pixso runtime."""
import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "Source/pixso_native"


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("DUPLICATE_JSON_KEY: " + key)
        result[key] = value
    return result


class DsmConfigTests(unittest.TestCase):
    def setUp(self):
        self.config = json.loads(
            (SOURCE / "dsm/component_parsers.json").read_text(encoding="utf-8"),
            object_pairs_hook=unique_object)
        self.mapping = json.loads(
            (SOURCE / "component-mapping.json").read_text(encoding="utf-8"))
        self.rules = {key: value for key, value in self.config.items()
                      if key != "__imports__"}

    def test_exact_one_to_one_source_tags(self):
        sources = [item["source"] for item in self.mapping["components"]]
        tags = [source["tag"] for source in sources]
        self.assertEqual(len(tags), len(set(tags)))
        self.assertEqual(set(self.rules), set(tags))
        self.assertEqual(set(self.config["__imports__"]), set(tags))

    def test_imports_are_existing_export_module_contracts(self):
        for item in self.mapping["components"]:
            source = item["source"]
            self.assertEqual(self.config["__imports__"][source["tag"]],
                             {"from": "@/" + source["import"], "default": True})
            self.assertTrue(source["import"].startswith("components/"))
            self.assertTrue(source["import"].endswith(".vue"))
            self.assertNotIn("..", Path(source["import"]).parts)

    def test_no_invented_filters_slots_styles_or_qt_metadata(self):
        for tag, rule in self.rules.items():
            self.assertEqual(rule, {"name": tag, "props": {"showTrueValue": True}})
        # codeOptions has no verified loading location in the current tool.
        self.assertNotIn("codeOptions", self.config)

    def test_documented_name_normalization_has_no_collision(self):
        normalized = ["".join(key.split()).lower() for key in self.rules]
        self.assertEqual(len(normalized), len(set(normalized)))


if __name__ == "__main__":
    unittest.main()
