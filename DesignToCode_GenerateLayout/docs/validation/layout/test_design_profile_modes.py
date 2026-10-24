"""Keep source variable modes separate from SDK contexts at the real CLI boundary."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

PROJECT = Path(__file__).resolve().parents[3]
FIXTURE = PROJECT / "docs/fixtures/layout/design-inputs/review-page/canonical.raw.json"
COMPILER = PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe"


class DesignProfileModeTests(unittest.TestCase):
    def setUp(self):
        cache = PROJECT / "output/work/preview/design-profile-tests"
        cache.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(cache))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.original = FIXTURE.read_bytes()
        self.addCleanup(lambda: self.assertEqual(FIXTURE.read_bytes(), self.original))
        self.raw = json.loads(self.original)

    def generate(self, extra=(), strict_layout=True):
        source = self.root / "input.json"
        source.write_bytes(json.dumps(self.raw, ensure_ascii=False).encode("utf-8"))
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", D2C_RUNTIME_ROOT=str(PROJECT / "build/tools/layout"), D2C_WORK_ROOT=str(PROJECT))
        if env.get("QT_ROOT"):
            env["PATH"] = str(Path(env["QT_ROOT"]) / "bin") + os.pathsep + env.get("PATH", "")
        command = [
            str(COMPILER), "--raw", str(source),
            "--mapping", str(PROJECT / "docs/fixtures/layout/mapping"),
            "--extensions", str(PROJECT / "docs/fixtures/layout/mapping/extensions.json"),
            "--font", str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
            "--output", str(self.root / "page"),
        ]
        if strict_layout:
            command.append("--strict-layout")
        command += list(extra)
        return subprocess.run(command, capture_output=True, env=env, timeout=30)

    def accepted(self, design_profile):
        result = self.generate()
        self.assertEqual(result.returncode, 0, result.stderr.decode("utf-8", errors="replace"))
        components = json.loads((self.root / "page.components.json").read_bytes())
        tree = json.loads((self.root / "page.design-tree.json").read_bytes())
        self.assertEqual(components["designProfile"], design_profile)
        self.assertEqual(components["profile"], "standard-light")
        self.assertEqual(tree["variableValidation"]["status"], "PASS")
        self.assertGreater(tree["variableValidation"]["referenceCount"], 0)
        self.assertEqual(tree["sourceSha256"], hashlib.sha256((self.root / "input.json").read_bytes()).hexdigest())

    def rejected(self, code):
        result = self.generate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(code, result.stderr.decode("utf-8", errors="replace"))
        self.assertFalse((self.root / "page.ui").exists())

    def test_original_design_mode_with_sdk_alias(self):
        self.accepted("light-comfortable")

    def test_canonical_design_mode_without_alias(self):
        # Synthetic input only: do not rename the immutable captured evidence.
        self.raw = json.loads(json.dumps(self.raw).replace('"light-comfortable"', '"standard-light"'))
        self.accepted("standard-light")

    def test_missing_design_profile_uses_unique_alias_for_variable_modes(self):
        # 设计侧没有 Profile 标记时，映射默认上下文登记的唯一别名决定变量模式选择。
        def strip(node):
            if isinstance(node, dict):
                variants = node.get("propRefMap", {}).get("variant")
                if isinstance(variants, list):
                    node["propRefMap"]["variant"] = [
                        item for item in variants if item.get("name") != "Profile"]
                for value in node.values():
                    strip(value)
            elif isinstance(node, list):
                for value in node:
                    strip(value)
        strip(self.raw["pixTreeNodes"])
        self.accepted("light-comfortable")

    def test_missing_value_is_not_filled_from_another_mode(self):
        del self.raw["variableMap"]["12:466"]["modeValues"]["12:423"]
        self.rejected("VARIABLE_MODE_VALUE_MISSING")

    def test_explicit_mode_overrides_profile_name(self):
        self.raw["pixTreeNodes"][0].setdefault("variableModeBySetMap", {})["12:424"] = "12:425"
        del self.raw["variableMap"]["12:466"]["modeValues"]["12:423"]
        self.accepted("light-comfortable")

    def test_invalid_explicit_mode_is_not_silently_replaced(self):
        self.raw["pixTreeNodes"][0].setdefault("variableModeBySetMap", {})["12:424"] = "missing-mode"
        self.rejected("VARIABLE_MODE_VALUE_MISSING")

    def test_ambiguous_design_modes_are_rejected(self):
        self.raw["variableSetMap"]["12:424"].append({"id": "duplicate-mode", "name": "light-comfortable"})
        self.rejected("VARIABLE_MODE_AMBIGUOUS")

    def test_undeclared_profile_alias_is_rejected(self):
        self.raw = json.loads(json.dumps(self.raw).replace('"light-comfortable"', '"unknown-design-profile"'))
        self.rejected("PROFILE_MISMATCH")

    def test_layout_only_skips_unresolved_variable_mode_selection(self):
        # 结构模式不消费 Token 值。多模式集合无法由导出数据唯一选择时，严格
        # 模式应继续失败；结构模式只保留节点/布局/组件类，不允许调用方注入模式。
        for modes in self.raw["variableSetMap"].values():
            for index, mode in enumerate(modes):
                mode["name"] = "opaque-mode-" + str(index)
        result = self.generate(extra=("--layout-only",), strict_layout=False)
        self.assertEqual(result.returncode, 0, result.stderr.decode("utf-8", errors="replace"))
        tree = json.loads((self.root / "page.design-tree.json").read_bytes())
        self.assertEqual(tree["variableValidation"]["status"], "NOT_APPLICABLE")
        self.assertEqual(tree["variableValidation"]["reason"],
                         "STRUCTURE_ONLY_SKIPS_TOKEN_MODE_RESOLUTION")

    def test_layout_only_defaults_missing_numeric_metrics_and_records_them(self):
        self.raw["pixTreeNodes"][0]["autoLayout"].pop("autoLayoutPaddingLeft")
        result = self.generate(extra=("--layout-only",), strict_layout=False)
        self.assertEqual(result.returncode, 0, result.stderr.decode("utf-8", errors="replace"))
        components = json.loads((self.root / "page.components.json").read_bytes())
        layout = components["trace"]["layouts"][0]
        self.assertIn("autoLayoutPaddingLeft", layout["defaultedMetricFields"])

    def test_layout_only_accepts_explicit_static_geometry_without_auto_layout(self):
        # 静态容器不是不完整的 Auto Layout：设计明确给出父坐标系中的
        # left/top/width/height 时，结构模式须保留为 QWidget 嵌套与 geometry。
        self.raw = {
            "pixTreeNodes": [{
                "guid": "static:root", "name": "StaticRoot", "type": "FRAME",
                "width": 320, "height": 180, "autoLayout": {}, "childNode": [{
                    "guid": "static:child", "name": "StaticChild", "type": "FRAME",
                    "left": 17, "top": 23, "width": 200, "height": 90,
                    "autoLayout": {}, "childNode": [{
                        "guid": "static:text", "name": "StaticText", "type": "PARAGRAPH",
                        "left": 11, "top": 13, "width": 80, "height": 24,
                        "nodeText": "static content", "childNode": []
                    }]
                }]
            }], "pixComponentTreeDslNodes": [], "variableMap": {}, "variableSetMap": {}
        }
        strict = self.generate()
        self.assertNotEqual(strict.returncode, 0)
        self.assertIn("UNMAPPED_LAYOUT_MODE", strict.stderr.decode("utf-8", errors="replace"))
        complete = self.raw
        self.raw = json.loads(json.dumps(complete))
        del self.raw["pixTreeNodes"][0]["childNode"][0]["top"]
        incomplete = self.generate(extra=("--layout-only",), strict_layout=False)
        self.assertNotEqual(incomplete.returncode, 0)
        self.assertIn("STATIC_GEOMETRY_REQUIRED: static:child/top",
                      incomplete.stderr.decode("utf-8", errors="replace"))
        self.raw = complete
        result = self.generate(extra=("--layout-only",), strict_layout=False)
        self.assertEqual(result.returncode, 0, result.stderr.decode("utf-8", errors="replace"))
        components = json.loads((self.root / "page.components.json").read_bytes())
        self.assertEqual(components["trace"]["staticLayouts"][0]["strategy"], "absolute-geometry")
        tree = ET.parse(str(self.root / "page.ui"))
        child = next(widget for widget in tree.findall(".//widget")
                     if widget.get("name") == "StaticChild")
        rect = child.find("property[@name='geometry']/rect")
        self.assertEqual([rect.findtext(field) for field in ("x", "y", "width", "height")],
                         ["17", "23", "200", "90"])


if __name__ == "__main__":
    unittest.main()
