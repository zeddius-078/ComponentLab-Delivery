"""Keep DesignToCode components, Token inputs and resources locally owned."""
from pathlib import Path
import ast
import json
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
KIT = ROOT / "ComponentKit"


class IndependenceTests(unittest.TestCase):
    def test_resource_files_belong_to_design_to_code(self):
        for qrc in (KIT / "runtime/component-resources.qrc",
                    ROOT / "Qt/MappingWorkbench/mapping-resources.qrc"):
            for node in ET.parse(qrc).findall(".//file"):
                resolved = (qrc.parent / node.text).resolve()
                self.assertTrue(resolved.is_relative_to(ROOT), str(resolved))
                self.assertTrue(resolved.is_file(), str(resolved))

    def test_private_component_contracts(self):
        mapping = json.loads((KIT / "design-assets/tokens/figma-mapping.json").read_bytes())
        for component in mapping["components"]:
            for key in ("header", "implementation", "style", "contract"):
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


if __name__ == "__main__":
    unittest.main()
