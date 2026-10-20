"""验证布局生成器对结构形态的处理，不把具体业务页面当作能力边界。"""
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


PROJECT = Path(__file__).resolve().parents[3]
PIPELINE = PROJECT / "Source/HtmlDsl/pipeline"
FIXTURES = PROJECT / "docs/fixtures/layout"
COMPILER = Path(os.environ.get(
    "D2C_COMPILER",
    str(PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe"),
))


class LayoutShapeMatrixTests(unittest.TestCase):
    """使用同一个 CLI 检查布局方向、嵌套深度和重复输出。"""

    def setUp(self):
        (PROJECT / 'output/work').mkdir(parents=True, exist_ok=True)
        self.review = json.loads(
            (FIXTURES / "design-inputs/review-page/canonical.raw.json").read_text(
                encoding="utf-8"
            )
        )
        self.result = json.loads(
            (FIXTURES / "design-inputs/result-page/canonical.raw.json").read_text(
                encoding="utf-8"
            )
        )

    def generate(self, raw, output):
        input_path = output.parent / "input.json"
        input_path.write_text(json.dumps(raw, ensure_ascii=False), encoding="utf-8")
        environment = dict(os.environ, QT_QPA_PLATFORM="offscreen", D2C_RUNTIME_ROOT=str(PROJECT / "build/tools/layout"), D2C_WORK_ROOT=str(PROJECT))
        qt_root = environment.get("QT_ROOT")
        if qt_root:
            environment["PATH"] = str(Path(qt_root) / "bin") + os.pathsep + environment.get("PATH", "")
        result = subprocess.run(
            [
                str(COMPILER),
                "--raw", str(input_path),
                "--mapping", str(FIXTURES / "mapping"),
                "--extensions", str(FIXTURES / "mapping/extensions.json"),
                "--font", str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
                "--output", str(output),
                "--strict-layout",
            ],
            capture_output=True,
            timeout=30,
            env=environment,
        )
        self.assertEqual(
            result.returncode,
            0,
            result.stderr.decode("utf-8", errors="replace"),
        )

    def assert_valid_ui(self, output):
        ui_file = Path(str(output) + ".ui")
        self.assertTrue(ui_file.is_file())
        ui = ET.parse(ui_file)
        root = ui.getroot().find("widget")
        self.assertIsNotNone(root)
        layouts = ui.findall(".//layout")
        self.assertGreaterEqual(len(layouts), 1)
        names = [node.get("name", "") for node in ui.findall(".//widget") + layouts]
        self.assertEqual(len(names), len(set(names)))
        self.assertTrue(all(name and name[0].islower() for name in names))

    def test_vertical_root_shape(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="layout-shape-vertical-") as directory:
            self.generate(self.review, Path(directory) / "vertical")
            self.assert_valid_ui(Path(directory) / "vertical")

    def test_horizontal_root_shape(self):
        raw = copy.deepcopy(self.review)
        raw["pixTreeNodes"][0]["autoLayout"]["stackMode"] = "HORIZONTAL"
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="layout-shape-horizontal-") as directory:
            self.generate(raw, Path(directory) / "horizontal")
            self.assert_valid_ui(Path(directory) / "horizontal")

    def test_nested_vertical_horizontal_shape(self):
        raw = copy.deepcopy(self.result)
        # 结构矩阵为布局能力测试输入；缺失的布局证据在测试副本中显式补齐，
        # 不改变生成器对真实输入“缺失即失败”的规则。
        def complete_metrics(node):
            if node.get("type") == "FRAME":
                layout = node.setdefault("autoLayout", {})
                layout.setdefault("autoLayoutPaddingLeft", 0)
                layout.setdefault("autoLayoutPaddingTop", 0)
                layout.setdefault("autoLayoutPaddingRight", 0)
                layout.setdefault("autoLayoutPaddingBottom", 0)
                layout.setdefault("autoLayoutItemSpacing", 0)
                layout.setdefault("autoLayoutPrimaryAlign", "flex-start")
                layout.setdefault("autoLayoutCounterAlign", "flex-start")
            for child in node.get("childNode", []):
                complete_metrics(child)
        complete_metrics(raw["pixTreeNodes"][0])
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="layout-shape-nested-") as directory:
            self.generate(raw, Path(directory) / "nested")
            output = Path(directory) / "nested"
            self.assert_valid_ui(output)
            ui_file = Path(str(output) + ".ui")
            self.assertGreaterEqual(len(ET.parse(ui_file).findall(".//layout")), 2)

    def test_same_shape_is_byte_stable(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="layout-shape-repeat-") as directory:
            first = Path(directory) / "first"
            second = Path(directory) / "second"
            self.generate(self.review, first)
            self.generate(self.review, second)
            first_hash = hashlib.sha256(Path(str(first) + ".ui").read_bytes()).hexdigest()
            second_hash = hashlib.sha256(Path(str(second) + ".ui").read_bytes()).hexdigest()
            self.assertEqual(first_hash, second_hash)

    def test_grid_and_form_emit_native_ui_positions(self):
        def paragraph(guid, name, text, width, layout_item):
            return {
                "guid": guid,
                "name": name,
                "type": "PARAGRAPH",
                "width": width,
                "height": 28,
                "visible": True,
                "nodeText": text,
                "fontFamily": "HarmonyOS Sans SC",
                "fontStyle": "Regular",
                "fontWeight": 400,
                "fontSize": 14,
                "fillPaints": [{"type": "SOLID", "color": {"r": 20, "g": 30, "b": 40}}],
                "layoutItem": layout_item,
            }

        def root(name, mode, children, extra=None):
            auto_layout = {
                "stackMode": mode,
                "autoLayoutItemSpacing": 8,
                "autoLayoutPaddingLeft": 12,
                "autoLayoutPaddingTop": 12,
                "autoLayoutPaddingRight": 12,
                "autoLayoutPaddingBottom": 12,
            }
            auto_layout.update(extra or {})
            return {
                "pixTreeNodes": [{
                    "guid": f"fixture:{mode}",
                    "name": name,
                    "type": "FRAME",
                    "width": 420,
                    "height": 180,
                    "visible": True,
                    "autoLayout": auto_layout,
                    "childNode": children,
                }],
                "pixComponentTreeDslNodes": [],
            }

        cases = [
            (
                "grid",
                root(
                    "Grid Fixture",
                    "GRID",
                    [
                        paragraph("fixture:grid:b", "Grid B", "B", 100, {"gridRow": 1, "gridColumn": 0, "gridColumnSpan": 2}),
                        paragraph("fixture:grid:a", "Grid A", "A", 100, {"gridRow": 0, "gridColumn": 0}),
                    ],
                    {"gridColumnGap": 10, "gridRowGap": 6, "gridRowStretch": [1, 2], "gridColumnStretch": [1, 1]},
                ),
                "QGridLayout",
            ),
            (
                "form",
                root(
                    "Form Fixture",
                    "FORM",
                    [
                        paragraph("fixture:form:field", "Form Field", "Field", 180, {"formRow": 0, "formRole": "field"}),
                        paragraph("fixture:form:label", "Form Label", "Label", 80, {"formRow": 0, "formRole": "label"}),
                    ],
                    {"formFieldGrowthPolicy": "allNonFixedFieldsGrow", "formRowWrapPolicy": "dontWrapRows"},
                ),
                "QFormLayout",
            ),
        ]
        for name, raw, expected_class in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix=f"layout-{name}-") as directory:
                output = Path(directory) / name
                self.generate(raw, output)
                ui = ET.parse(Path(str(output) + ".ui"))
                layout = ui.find(f".//layout[@class='{expected_class}']")
                self.assertIsNotNone(layout)
                items = layout.findall("item")
                self.assertEqual(len(items), 2)
                if expected_class == "QGridLayout":
                    grid_item = next(item for item in items if item.find("widget").get("name") == "gridB")
                    self.assertEqual(grid_item.attrib["row"], "1")
                    self.assertEqual(grid_item.attrib["column"], "0")
                    self.assertEqual(grid_item.attrib["colspan"], "2")
                else:
                    positions = {(item.find("widget").get("name"), item.attrib["row"], item.attrib["column"])
                                 for item in items}
                    self.assertIn(("formLabel", "0", "0"), positions)
                    self.assertIn(("formField", "0", "1"), positions)

        mixed = {
            "pixTreeNodes": [{
                "guid": "fixture:mixed",
                "name": "Mixed Layout",
                "type": "FRAME",
                "width": 520,
                "height": 260,
                "visible": True,
                "autoLayout": {
                    "stackMode": "VERTICAL",
                    "autoLayoutItemSpacing": 10,
                    "autoLayoutPaddingLeft": 8,
                    "autoLayoutPaddingTop": 8,
                    "autoLayoutPaddingRight": 8,
                    "autoLayoutPaddingBottom": 8,
                },
                "childNode": [
                    {
                        "guid": "fixture:form-panel",
                        "name": "Form Panel",
                        "type": "FRAME",
                        "width": 500,
                        "height": 100,
                        "visible": True,
                        "autoLayout": {
                            "stackMode": "FORM",
                            "autoLayoutItemSpacing": 6,
                            "autoLayoutPaddingLeft": 4,
                            "autoLayoutPaddingTop": 4,
                            "autoLayoutPaddingRight": 4,
                            "autoLayoutPaddingBottom": 4,
                        },
                        "childNode": [
                            paragraph("fixture:mixed:field", "Mixed Field", "Field", 180, {"formRow": 0, "formRole": "field"}),
                            paragraph("fixture:mixed:label", "Mixed Label", "Label", 80, {"formRow": 0, "formRole": "label"}),
                        ],
                    },
                    {
                        "guid": "fixture:grid-panel",
                        "name": "Grid Panel",
                        "type": "FRAME",
                        "width": 500,
                        "height": 100,
                        "visible": True,
                        "autoLayout": {
                            "stackMode": "GRID",
                            "autoLayoutItemSpacing": 6,
                            "autoLayoutPaddingLeft": 4,
                            "autoLayoutPaddingTop": 4,
                            "autoLayoutPaddingRight": 4,
                            "autoLayoutPaddingBottom": 4,
                        },
                        "childNode": [
                            paragraph("fixture:mixed:grid:b", "Mixed Grid B", "B", 100, {"gridRow": 1, "gridColumn": 0, "gridColumnSpan": 2}),
                            paragraph("fixture:mixed:grid:a", "Mixed Grid A", "A", 100, {"gridRow": 0, "gridColumn": 0}),
                        ],
                    },
                ],
            }],
            "pixComponentTreeDslNodes": [],
        }
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="layout-mixed-") as directory:
            output = Path(directory) / "mixed"
            self.generate(mixed, output)
            ui = ET.parse(Path(str(output) + ".ui"))
            self.assertIsNotNone(ui.find(".//layout[@class='QGridLayout']"))
            self.assertIsNotNone(ui.find(".//layout[@class='QFormLayout']"))


if __name__ == "__main__":
    unittest.main()
