"""Real compiler regression; synthetic designs deliberately unrelated to Commerly."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

PROJECT = Path(__file__).resolve().parents[3]
COMPILER = PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe"
FONT = PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"
STYLE = PROJECT / "docs/examples/styles/initial-light.json"


class StructurePreviewTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if sys.version_info[:2] != (3, 9):
            raise RuntimeError("PYTHON_39_REQUIRED")
        reports = PROJECT / "output/work/structure-preview"
        reports.mkdir(parents=True, exist_ok=True)
        cls.work = Path(tempfile.mkdtemp(prefix="regression-", dir=str(reports)))
        cls.env = dict(os.environ)
        qt = Path(os.environ["QT_ROOT"])
        cls.env.update(QT_QPA_PLATFORM="offscreen", QT_PLUGIN_PATH=str(qt / "plugins"),
                       QT_QPA_PLATFORM_PLUGIN_PATH=str(qt / "plugins/platforms"))
        cls.env["PATH"] = str(qt / "bin") + os.pathsep + cls.env.get("PATH", "")
        cls.env["D2C_RUNTIME_ROOT"] = str(PROJECT / "build/tools/layout")
        cls.env["D2C_WORK_ROOT"] = str(PROJECT)
        cls.uic = qt / "bin/uic.exe"
        cls.source = {
            "schema": "design-structure/v1", "fileKey": "synthetic-independent",
            "provenance": "synthetic-regression", "root": {
                "id": "root", "name": "UnrelatedMachinePanel", "type": "FRAME",
                "x": 0, "y": 0, "width": 480, "height": 240, "children": [
                    {"id": "a", "type": "INSTANCE", "name": "UnknownControl",
                     "mainComponent": {"id": "master", "key": "independent-key"},
                     "x": 12, "y": 24, "width": 180, "height": 100, "children": [
                         {"id": "t", "type": "TEXT", "name": "caption", "characters": "<Test>&",
                          "x": 8, "y": 8, "width": 120, "height": 30, "fontSize": 16}]},
                    {"id": "hidden", "type": "TEXT", "characters": "not visible",
                     "visible": False, "x": 20, "y": 140, "width": 100, "height": 20}
                ]}}

    def compile(self, label, source=None, style=None, extra=()):
        raw = self.work / (label + ".input.json")
        raw.write_bytes(json.dumps(source or self.source).encode("utf-8"))
        style_path = STYLE
        if style is not None:
            style_path = self.work / (label + ".style.json")
            style_path.write_bytes(json.dumps(style).encode("utf-8"))
        output = self.work / label
        result = subprocess.run([str(COMPILER), "--structure-preview", "--raw", str(raw),
            "--initial-style", str(style_path), "--font", str(FONT), "--output", str(output),
            *extra], env=self.env, capture_output=True, text=True)
        return result, output

    def test_structure_identity_text_and_ui(self):
        result, output = self.compile("structure")
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(Path(str(output) + ".structure.json").read_text(encoding="utf-8"))
        self.assertEqual(len(report["nodes"]), 4)
        instance = next(n for n in report["nodes"] if n["id"] == "a")
        self.assertEqual(instance["mainComponent"]["key"], "independent-key")
        self.assertEqual(instance["geometry"], [12, 24, 180, 100])
        self.assertTrue(instance["objectName"].startswith("node_UnknownControl_"))
        hidden = next(n for n in report["nodes"] if n["id"] == "hidden")
        self.assertFalse(hidden["visible"])
        tree = ET.parse(str(output) + ".ui")
        self.assertIn("<Test>&", [p.text for p in tree.findall(".//property[@name='text']/string")])
        self.assertEqual(set(w.attrib["class"] for w in tree.findall(".//widget")), {"QWidget", "QLabel"})
        unknown = next(w for w in tree.findall(".//widget")
                       if w.attrib["name"] == instance["objectName"])
        self.assertEqual(unknown.attrib["class"], "QWidget")
        self.assertEqual(unknown.find("property[@name='designNodeName']/string").text, "UnknownControl")
        subprocess.run([str(self.uic), str(output) + ".ui"], env=self.env, check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def test_style_independent_of_structure(self):
        a, first = self.compile("style-first")
        changed = json.loads(STYLE.read_text(encoding="utf-8"))
        changed.update(name="alternate", canvas="#20242a", text="#eeeeee", surface="#30363d")
        b, second = self.compile("style-second", style=changed)
        self.assertEqual((a.returncode, b.returncode), (0, 0), a.stderr + b.stderr)
        one = json.loads(Path(str(first) + ".structure.json").read_text(encoding="utf-8"))
        two = json.loads(Path(str(second) + ".structure.json").read_text(encoding="utf-8"))
        self.assertEqual(one["nodes"], two["nodes"])
        self.assertNotEqual(Path(str(first) + ".qss").read_bytes(), Path(str(second) + ".qss").read_bytes())

    def test_same_design_names_keep_distinct_objects(self):
        source = copy.deepcopy(self.source)
        other = copy.deepcopy(source["root"]["children"][0])
        other["id"] = "another-instance"
        other["children"][0]["id"] = "another-text"
        source["root"]["children"].append(other)
        result, output = self.compile("same-names", source=source)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(Path(str(output) + ".structure.json").read_text(encoding="utf-8"))
        names = [n["objectName"] for n in report["nodes"] if n.get("sourceName") == "UnknownControl"]
        self.assertEqual(len(names), 2)
        self.assertEqual(len(set(names)), 2)

    def test_duplicate_identity_rejected(self):
        source = copy.deepcopy(self.source)
        source["root"]["children"][1]["id"] = "a"
        result, output = self.compile("duplicate", source=source)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("STRUCTURE_ID_INVALID_OR_DUPLICATE", result.stderr)
        self.assertFalse(Path(str(output) + ".ui").exists())

    def test_instance_identity_not_guessed(self):
        source = copy.deepcopy(self.source)
        del source["root"]["children"][0]["mainComponent"]
        result, _ = self.compile("missing-identity", source=source)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("STRUCTURE_INSTANCE_IDENTITY_MISSING", result.stderr)

    def test_invalid_geometry_rejected(self):
        source = copy.deepcopy(self.source)
        source["root"]["children"][0]["width"] = -1
        result, _ = self.compile("geometry", source=source)
        self.assertIn("STRUCTURE_INVALID_GEOMETRY", result.stderr)
        self.assertNotEqual(result.returncode, 0)

    def test_readback_negative_half_coordinates(self):
        source = copy.deepcopy(self.source)
        source["root"]["children"][0]["y"] = -34.5
        result, output = self.compile("negative-half", source=source)
        self.assertEqual(result.returncode, 0, result.stderr)
        spec = importlib.util.spec_from_file_location(
            "preview_entry", PROJECT / "Source/HtmlDsl/PreviewStructure.py")
        entry = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(entry)
        report = json.loads(Path(str(output) + ".structure.json").read_text(encoding="utf-8"))
        self.assertEqual(entry.verify_structure(source, report), 4)

    def test_no_formal_or_mapping_bypass(self):
        for flag in ("--formal", "--strict-layout"):
            result, _ = self.compile("conflict-" + flag[2:], extra=[flag])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("STRUCTURE_PREVIEW_MODE_CONFLICT", result.stderr)

    def test_overwrite_rejected(self):
        first, _ = self.compile("existing")
        second, _ = self.compile("existing")
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertNotEqual(second.returncode, 0)
        self.assertIn("STRUCTURE_OUTPUT_ALREADY_EXISTS", second.stderr)

    def test_style_injection_rejected(self):
        style = json.loads(STYLE.read_text(encoding="utf-8"))
        style["canvas"] = "url(file:///outside.png)"
        result, _ = self.compile("style-invalid", style=style)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("INITIAL_STYLE_COLOR_INVALID", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
