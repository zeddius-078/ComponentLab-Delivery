"""Check imports and input guards without touching controlled tools or SDK."""
import ast
import importlib.util
from pathlib import Path
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "Source/pixso_native"
SPEC = importlib.util.spec_from_file_location("native_acquire", SOURCE / "acquire.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class IsolationTests(unittest.TestCase):
    def test_source_imports_only_explicit_standard_library(self):
        allowed = {"argparse", "ast", "hashlib", "json", "math", "os", "pathlib",
                   "re", "stat", "sys", "urllib", "xml", "zipfile",
                   "acquire", "component_engine", "component_binding", "layout_engine", "styled_layout",
                   "subprocess", "assets", "page_output", "generation_policy"}
        for path in SOURCE.glob("*.py"):
            tree = ast.parse(path.read_text(encoding="utf-8"))
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    names = [alias.name for alias in node.names]
                elif isinstance(node, ast.ImportFrom):
                    self.assertEqual(node.level, 0, str(path))
                    names = [node.module]
                else:
                    continue
                for name in names:
                    self.assertIn(name.split(".")[0], allowed, str(path))
        # This is an import boundary check, not a substitute for a later
        # complete generation run with first-route paths inaccessible.

    def test_no_first_route_paths(self):
        for path in SOURCE.glob("*"):
            if path.suffix in (".py", ".json"):
                text = path.read_text(encoding="utf-8")
                for forbidden in ("HtmlDsl", "v2_ui_compiler", "adapt_v2.cjs"):
                    self.assertNotIn(forbidden, text, str(path))

    def test_archive_paths(self):
        for value in ("../outside", "/absolute", "a\\b", "CON.txt",
                      "a/../b", "a./b", "a:b", "a//b"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                MODULE.relative_name(value)
        self.assertEqual(str(MODULE.relative_name("vue3/src/App.vue")), "vue3/src/App.vue")

    def test_output_traversal_rejected_before_creation(self):
        with self.assertRaisesRegex(ValueError, "OUTPUT_TRAVERSAL_REFUSED"):
            MODULE.new_output(ROOT / "output/work/pixso-native/../../escaped")

    def test_work_suffix_cannot_bypass_maintained_roots(self):
        for directory in ("Source", "docs", "build"):
            for nested in (Path(), Path("nested")):
                candidate = ROOT / directory / nested / "output/work/pixso-native/guard-probe"
                with self.subTest(path=str(candidate)):
                    with self.assertRaisesRegex(ValueError, "OUTPUT_IN_PROTECTED_TREE"):
                        MODULE.checked_work_path(candidate)

    def test_protected_output_rejected_before_mkdir(self):
        candidate = ROOT / "build/output/work/pixso-native/guard-probe"
        # A failing guard must never create files in a controlled directory,
        # including while this regression test runs against the old code.
        with mock.patch.object(Path, "mkdir") as mkdir:
            with self.assertRaisesRegex(ValueError, "OUTPUT_IN_PROTECTED_TREE"):
                MODULE.new_output(candidate)
            mkdir.assert_not_called()

    def test_work_and_relocated_project_paths_remain_valid(self):
        work = ROOT / "output/work/pixso-native/guard-probe"
        for candidate in (work, work / "Source", work / "build",
                          work / "isolated-project/output/work/pixso-native/page"):
            with self.subTest(path=str(candidate)):
                self.assertEqual(MODULE.checked_work_path(candidate), candidate.absolute())

    def test_relocated_project_maintained_trees_are_protected(self):
        project = ROOT / "output/work/pixso-native/guard-probe/isolated-project"
        directories = {project / "Source", project / "docs"}
        with mock.patch.object(Path, "is_dir", lambda value: value in directories):
            for directory in ("Source", "docs", "build"):
                candidate = project / directory / "nested/output/work/pixso-native/page"
                with self.subTest(path=str(candidate)):
                    with self.assertRaisesRegex(ValueError, "OUTPUT_IN_PROTECTED_TREE"):
                        MODULE.checked_work_path(candidate)
            candidate = project / "output/work/pixso-native/page"
            self.assertEqual(MODULE.checked_work_path(candidate), candidate.absolute())

    def test_windows_alias_segments_cannot_hide_protected_tree(self):
        for directory in ("Source.", "docs ", "build.", "build:stream"):
            candidate = ROOT / directory / "output/work/pixso-native/page"
            with self.subTest(path=str(candidate)):
                with self.assertRaisesRegex(ValueError, "OUTPUT_AMBIGUOUS_PATH"):
                    MODULE.checked_work_path(candidate)


if __name__ == "__main__":
    unittest.main()
