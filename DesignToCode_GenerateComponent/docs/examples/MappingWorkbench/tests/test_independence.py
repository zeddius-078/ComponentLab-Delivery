"""Keep DesignToCode components, Token inputs and resources locally owned."""
from pathlib import Path
import ast
import json
import unittest
import xml.etree.ElementTree as ET

PROJECT = Path(__file__).resolve().parents[4]
ROOT = PROJECT / "Source"
KIT = PROJECT / "output/work/generated/Source/ComponentKit"


class IndependenceTests(unittest.TestCase):
    def test_resource_files_belong_to_design_to_code(self):
        for qrc in (KIT / "runtime/component-resources.qrc",
                    PROJECT / "docs/examples/MappingWorkbench/mapping-resources.qrc"):
            for node in ET.parse(qrc).findall(".//file"):
                resolved = (qrc.parent / node.text).resolve()
                self.assertTrue(resolved.is_relative_to(PROJECT), str(resolved))
                self.assertTrue(resolved.is_file(), str(resolved))

    def test_canonical_component_token_sources(self):
        mapping = json.loads((PROJECT / "docs/examples/MappingWorkbench/mapping/qt-component-map.json").read_bytes())
        for name in ("Button", "Input", "Select"):
            component = mapping["components"][name]
            for key in ("header", "implementation", "style", "tokenContract"):
                target = (KIT / component[key]).resolve()
                self.assertTrue(target.is_relative_to(KIT), str(target))
                self.assertTrue(target.is_file(), str(target))

    def test_no_component_lab_build_or_runtime_dependency(self):
        # Inspect maintained commands, not saved design evidence or old result logs.
        for path in ROOT.rglob("*"):
            if not path.is_file() or {"Build", "__pycache__"}.intersection(path.relative_to(ROOT).parts):
                continue
            if path == Path(__file__) or path.suffix not in {".cpp", ".h", ".ps1", ".py", ".qrc"} and path.name != "CMakeLists.txt":
                continue
            content = path.read_text(encoding="utf-8-sig").replace("\\", "/")
            for forbidden in ("../Source/", "'Source/runtime", '"Source/runtime',
                              "DemoRuntime.h", "button_demo::install", "component_lab.exe", "../Build/licenses"):
                self.assertNotIn(forbidden, content, str(path))

    def test_python_is_only_a_build_time_data_tool(self):
        for path in (KIT / "design-assets").rglob("*.py"):
            tree = ast.parse(path.read_text(encoding="utf-8-sig"))
            for node in ast.walk(tree):
                modules = ([alias.name for alias in node.names] if isinstance(node, ast.Import)
                           else [node.module or ""] if isinstance(node, ast.ImportFrom) else [])
                self.assertFalse(any(name.startswith(("PySide", "PyQt", "shiboken")) for name in modules), str(path))

    def test_components_do_not_reverse_depend_on_theme(self):
        component_dir = KIT / "runtime/component"
        for path in component_dir.rglob("*"):
            if path.suffix not in {".cpp", ".h"}:
                continue
            content = path.read_text(encoding="utf-8-sig")
            self.assertNotIn("ComponentTheme.h", content, str(path))
            self.assertNotIn("component_theme::", content, str(path))

    def test_runtime_font_chain_is_harmony_only(self):
        font_root = KIT / "runtime/assets/fonts"
        self.assertEqual({path.name for path in font_root.iterdir() if path.is_dir()},
                         {"HarmonyOSSans"})
        qrc = (KIT / "runtime/component-resources.qrc").read_text(encoding="utf-8-sig")
        self.assertIn("HarmonyOS_Sans.ttf", qrc)
        self.assertIn("HarmonyOS_Sans_SC.ttf", qrc)
        for forbidden in ("Inter", "Poppins", "NotoSans", "Microsoft YaHei"):
            self.assertNotIn(forbidden, qrc)


if __name__ == "__main__":
    unittest.main()
