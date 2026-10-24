import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from styled_layout import (lower_tree, object_name, select_root, inspect_tree, StyledLayoutError)
from layout_engine import LayoutEngine, LayoutError


def element(offset=0, instance="root", **css):
    return {"kind": "element", "tag": "div", "attrs": {"id": "design_1"},
            "source": {"file": "Example.vue", "instancePath": instance, "offset": offset},
            "style": {"values": {"display": "flex", "flex-direction": "column", **css}},
            "inLayout": True, "children": []}


class StyledLayoutTests(unittest.TestCase):
    def test_adapter_to_engine(self):
        root = element(**{"padding-left": "24px", "background-color": "#fff"})
        root["children"] = [element(20, **{"flex-direction": "row"})]
        adapted = lower_tree(root)
        doc = LayoutEngine().document("Example", adapted["root"])
        self.assertEqual(len(list(doc.iter("layout"))), 2)
        self.assertEqual(doc.findtext("./widget/layout/property[@name='leftMargin']/number"), "24")
        self.assertEqual(adapted["pageStyles"][adapted["root"]["name"]],
                         {"background-color": "#fff"})
        self.assertEqual(adapted, lower_tree(copy.deepcopy(root)))

    def test_identity_is_scoped(self):
        first, second = element(instance="first"), element(instance="second")
        tree = {"children": [first, second]}
        self.assertIs(select_root(tree, "second", "design_1"), second)
        self.assertNotEqual(lower_tree(first)["root"]["name"], lower_tree(second)["root"]["name"])
        with self.assertRaisesRegex(StyledLayoutError, "NOT_UNIQUE"):
            select_root({"children": [first, copy.deepcopy(first)]}, "first", "design_1")

    def test_dsl_unicode_name_is_preserved(self):
        node = element()
        node["designName"] = "实例 1"
        node["source"]["designNodeId"] = "2:28522"
        node["attrs"]["id"] = "2_28522"
        name = object_name(node)
        self.assertTrue(name.startswith("px_实例_1_2_28522_"), name)
        self.assertNotIn(" ", name)

    def test_component_boundary_is_not_flattened(self):
        root = element()
        root["componentBoundaries"] = [{"tag": "RealComponent"}]
        root["children"] = [element(20)]
        with self.assertRaisesRegex(StyledLayoutError, "COMPONENT_ADAPTER_REQUIRED"):
            lower_tree(root)
        calls = []
        def adapter(node, name):
            calls.append(node)
            return {"kind": "component", "name": name, "source": node["source"]}
        result = lower_tree(root, adapter)
        self.assertEqual(len(calls), 1)
        self.assertNotIn("children", result["root"])

    def test_css_and_text_not_dropped(self):
        for css in ({"width": "100%"}, {"margin-left": "3px"}, {"top": "2px"}):
            with self.subTest(css=css), self.assertRaises((StyledLayoutError, LayoutError)):
                lower_tree(element(**css))
        root = element()
        root["children"] = [{"kind": "text", "text": "Visible text"}]
        with self.assertRaisesRegex(StyledLayoutError, "TEXT_LOWERING_REQUIRED"):
            lower_tree(root)

    def test_display_none_excluded(self):
        root = element()
        child = element(1)
        child["inLayout"] = False
        child["componentBoundaries"] = [{"tag": "Hidden"}]
        root["children"] = [child]
        self.assertEqual(lower_tree(root)["root"]["children"], [])

    def test_inspection_collects_sibling_failures_without_flattening_components(self):
        root = element(overflow="scroll")
        child = element(1, **{"margin-left": "4px"})
        component = element(2)
        component["componentBoundaries"] = [{"tag": "KnownBoundary"}]
        component["children"] = [element(3, overflow="scroll")]
        root["children"] = [child, component]
        report = inspect_tree(root)
        self.assertEqual(len(report["layoutIssues"]), 2)
        self.assertEqual(len(report["componentBoundaries"]), 1)
        self.assertEqual(report["containers"], 2)
        self.assertEqual(report["pageGeneration"], "not-run")

    def test_zero_minimum_is_recorded_as_no_op(self):
        adapted = lower_tree(element(**{"min-width": "0"}))
        self.assertEqual([(item["property"], item["value"], item["disposition"])
                          for item in adapted["layoutDeclarations"]],
                         [("min-width", "0", "no-op")])
        self.assertNotIn("min-width", adapted["root"]["css"])

    def test_non_zero_minimum_is_rejected(self):
        for value in ("10px", "1em", "auto"):
            with self.subTest(value=value):
                with self.assertRaisesRegex(StyledLayoutError, "UNLOWERED_CSS"):
                    lower_tree(element(**{"min-width": value}))

    def test_stretch_needs_a_definite_cross_size(self):
        root = element(**{"flex-direction": "row"})
        root["children"] = [element(20, **{"align-self": "stretch"})]
        with self.assertRaisesRegex(StyledLayoutError, "UNLOWERED_CSS"):
            lower_tree(root)
        root["children"] = [element(20, **{"align-self": "stretch", "height": "839px",
                                          "box-sizing": "border-box"})]
        adapted = lower_tree(root)
        recorded = [item for item in adapted["layoutDeclarations"]
                    if item["property"] == "align-self"]
        self.assertEqual([(item["value"], item["disposition"]) for item in recorded],
                         [("stretch", "no-op")])
        self.assertNotIn("align-self", adapted["root"]["children"][0]["css"])

    def test_zero_flex_basis_absorbs_the_free_main_axis_space(self):
        root = element(**{"flex-direction": "row"})
        root["children"] = [element(20, **{"flex-basis": "0", "flex-grow": "1",
                                          "width": "100%", "box-sizing": "border-box"})]
        adapted = lower_tree(root)
        recorded = [item for item in adapted["layoutDeclarations"]
                    if item["property"] == "flex-basis"]
        self.assertEqual([(item["value"], item["disposition"]) for item in recorded],
                         [("0", "applied")])
        doc = LayoutEngine().document("Example", adapted["root"])
        widget = doc.find("./widget/layout/item/widget")
        self.assertIsNone(widget.find("./property[@name='maximumSize']"))
        policy = widget.find("./property[@name='sizePolicy']/sizepolicy")
        self.assertEqual(policy.get("hsizetype"), "Expanding")
        self.assertEqual(doc.find("./widget/layout").get("stretch"), "1")

    def test_flex_basis_failures_stay_closed(self):
        with self.assertRaisesRegex(StyledLayoutError, "UNLOWERED_CSS: flex-basis"):
            lower_tree(element(**{"flex-basis": "10px"}))
        root = element(**{"flex-direction": "row"})
        root["children"] = [element(20, **{"flex-basis": "0"})]
        with self.assertRaisesRegex(LayoutError, "FLEX_BASIS_ZERO_REQUIRES_FLEX_GROW"):
            lower_tree(root)
        with self.assertRaisesRegex(LayoutError, "FLEX_BASIS_PARENT_REQUIRED"):
            lower_tree(element(**{"flex-basis": "0", "flex-grow": "1"}))

    def test_clip_preserves_source_and_rejects_rounded_or_scrolling(self):
        adapted = lower_tree(element(overflow="hidden"))
        self.assertEqual(adapted["root"]["css"]["overflow"], "hidden")
        doc = LayoutEngine().document("Example", adapted["root"])
        self.assertEqual(doc.findtext("./widget/property[@name='d2cOverflow']/string"), "hidden")
        for css in ({"overflow": "scroll"},
                    {"overflow": "hidden", "border-radius": "8px"}):
            with self.subTest(css=css), self.assertRaises((StyledLayoutError, LayoutError)):
                lower_tree(element(**css))


if __name__ == "__main__":
    unittest.main()
