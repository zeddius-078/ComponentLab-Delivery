"""Maintained mapping inputs are allowed without admitting generated JSON."""
import importlib.util
from pathlib import Path
import unittest

PROJECT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "layout_project_check", PROJECT / "Source/CheckProject.py"
)
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class SourceInputTests(unittest.TestCase):
    def test_named_mappings_are_maintained_inputs(self):
        for name in CHECK.MAINTAINED_CONFIGS:
            path = CHECK.SOURCE / name
            self.assertTrue(path.is_file())
            self.assertTrue(CHECK.is_source_input(path))

    def test_generated_json_is_not_admitted(self):
        for name in ("capture.json", "pixso_native/report.json",
                     "other/component-mapping.json"):
            self.assertFalse(CHECK.is_source_input(CHECK.SOURCE / name), name)


if __name__ == "__main__":
    unittest.main()
