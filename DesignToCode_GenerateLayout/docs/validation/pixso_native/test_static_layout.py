"""Run with py -3.9 -B -m unittest discover -s docs/validation/pixso_native."""
import importlib.util
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "native_layout", ROOT / "Source/pixso_native/layout_engine.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def container(name, direction="row", children=None, **css):
    return {"kind": "container", "name": name,
            "source": {"file": "fixture.vue", "line": 1},
            "css": {"display": "flex", "flex-direction": direction, **css},
            "children": children or []}


class LayoutTests(unittest.TestCase):
    def test_css_unitless_zero(self):
        for value in ("0", "0.0", ".0", "-0", "+0"):
            self.assertEqual(MODULE.pixels(value, "padding-left"), 0)
        for value in ("1", "-1", "1.0"):
            with self.assertRaises(MODULE.LayoutError):
                MODULE.pixels(value, "padding-left")

    def test_alternating_three_levels(self):
        tree = container("root", children=[
            container("middle", "column", [container("inner", "row")])])
        doc = MODULE.LayoutEngine().document("Example", tree)
        self.assertEqual([x.get("class") for x in doc.iter("layout")],
                         ["QHBoxLayout", "QVBoxLayout", "QHBoxLayout"])
        self.assertIsNotNone(doc.find("./widget/layout/item/widget/layout/item/widget"))

    def test_margin_order_and_gap(self):
        root = container("root", **{"gap": "6px", "padding-top": 1,
                                   "padding-right": 2, "padding-bottom": 3,
                                   "padding-left": 4})
        layout = MODULE.LayoutEngine().document("Example", root).find("./widget/layout")
        actual = {p.get("name"): p.find("number").text for p in layout.findall("property")}
        self.assertEqual(actual, {"spacing": "6", "leftMargin": "4",
                                  "topMargin": "1", "rightMargin": "2", "bottomMargin": "3"})

    def test_css_not_silently_discarded(self):
        for css in ({"width": "50%"}, {"flex-wrap": "wrap"},
                    {"gap": "calc(2px + 1px)"}, {"gap": 1.5},
                    {"justify-content": "space-around"}):
            with self.subTest(css=css), self.assertRaises(MODULE.LayoutError):
                MODULE.LayoutEngine().document("Example", container("root", **css))

    def test_component_requires_mapping(self):
        component = {"kind": "component", "name": "button", "source": {"file": "x.vue"}}
        with self.assertRaisesRegex(MODULE.LayoutError, "COMPONENT_MAPPING_REQUIRED"):
            MODULE.LayoutEngine().document("Example", container("root", children=[component]))

    def test_source_required(self):
        root = container("root")
        del root["source"]
        with self.assertRaisesRegex(MODULE.LayoutError, "SOURCE_EVIDENCE_REQUIRED"):
            MODULE.LayoutEngine().document("Example", root)

    def test_duplicate_names_fail(self):
        root = container("root", children=[container("same"), container("same")])
        with self.assertRaisesRegex(MODULE.LayoutError, "DUPLICATE_OBJECT_NAME"):
            MODULE.LayoutEngine().document("Example", root)

    def test_center_uses_two_spacers(self):
        root = container("root", children=[container("child")],
                         **{"justify-content": "center", "align-items": "center"})
        layout = MODULE.LayoutEngine().document("Example", root).find("./widget/layout")
        self.assertEqual(len(layout.findall("./item/spacer")), 2)
        self.assertEqual(layout.find("./item/widget/..").get("alignment"), "Qt::AlignVCenter")

    def test_deterministic(self):
        engine = MODULE.LayoutEngine()
        root = container("root", children=[container("child")])
        self.assertEqual(ET.tostring(engine.document("Example", root)),
                         ET.tostring(engine.document("Example", root)))

    def test_fixed_border_box_bounds(self):
        child = container("child", **{"width": "320px", "height": "40px",
                                     "box-sizing": "border-box", "flex-shrink": "0"})
        doc = MODULE.LayoutEngine().document("Example", container("root", children=[child]))
        widget = doc.find("./widget/layout/item/widget")
        for prop in ("minimumSize", "maximumSize"):
            size = widget.find("./property[@name='" + prop + "']/size")
            self.assertEqual((size.findtext("width"), size.findtext("height")), ("320", "40"))

    def test_fixed_dimension_requires_non_shrinking_main_axis(self):
        for direction, axis in (("row", "width"), ("column", "height")):
            with self.subTest(direction=direction):
                child = container("child", **{axis: "40px", "box-sizing": "border-box"})
                with self.assertRaisesRegex(MODULE.LayoutError, "SHRINKABLE"):
                    MODULE.LayoutEngine().document("Example", container("root", direction, [child]))

    def test_fixed_cross_axis_does_not_require_shrink_override(self):
        child = container("child", **{"height": "40px", "box-sizing": "border-box"})
        doc = MODULE.LayoutEngine().document("Example", container("root", children=[child]))
        size = doc.find("./widget/layout/item/widget/property[@name='maximumSize']/size")
        self.assertEqual(size.findtext("height"), "40")
        self.assertEqual(size.findtext("width"), "16777215")

    def test_content_box_and_percentage_are_not_guessed(self):
        for css in ({"width": "40px"}, {"width": "100%", "box-sizing": "border-box"}):
            with self.subTest(css=css), self.assertRaises(MODULE.LayoutError):
                MODULE.LayoutEngine().document("Example", container("root", **css))

    def test_auto_does_not_become_fixed_or_hug(self):
        doc = MODULE.LayoutEngine().document("Example", container("root", width="auto"))
        self.assertEqual(doc.findall("./widget/property"), [])

    def test_full_cross_axis_uses_expanding_without_alignment(self):
        for direction, axis, policy_name in (("column", "width", "hsizetype"),
                                              ("row", "height", "vsizetype")):
            child = container("child", **{axis: "100%", "box-sizing": "border-box"})
            root = container("root", direction, [child],
                             **{"align-items": "flex-start", axis: "400px",
                                "box-sizing": "border-box"})
            doc = MODULE.LayoutEngine().document("Example", root)
            item = doc.find("./widget/layout/item")
            self.assertIsNone(item.get("alignment"))
            policy = item.find("./widget/property[@name='sizePolicy']/sizepolicy")
            self.assertEqual(policy.get(policy_name), "Expanding")

    def test_percentage_parent_must_be_definite(self):
        child = container("child", **{"height": "100%", "box-sizing": "border-box"})
        with self.assertRaisesRegex(MODULE.LayoutError, "DEFINITE_PERCENTAGE_PARENT"):
            MODULE.LayoutEngine().document("Example", container("root", children=[child]))

    def test_main_axis_percentage_not_treated_as_remaining_space(self):
        child = container("child", **{"width": "100%", "box-sizing": "border-box",
                                     "flex-shrink": "0"})
        with self.assertRaises(MODULE.LayoutError):
            MODULE.LayoutEngine().document("Example", container("root", children=[child]))

    def test_single_grow_owns_free_space_before_justify(self):
        grow = container("grow", **{"flex-grow": "1", "flex-shrink": "0", "width": "auto"})
        root = container("root", children=[container("fixed"), grow],
                         **{"justify-content": "space-between"})
        layout = MODULE.LayoutEngine().document("Example", root).find("./widget/layout")
        self.assertEqual(layout.get("stretch"), "0,1")
        self.assertEqual(layout.findall("./item/spacer"), [])
        policy = layout.find("./item/widget[@name='grow']/property[@name='sizePolicy']/sizepolicy")
        self.assertEqual(policy.get("hsizetype"), "Expanding")

    def test_unproved_grow_cases_rejected(self):
        for css in ({"flex-grow": "2"}, {"flex-grow": "1"},
                    {"flex-grow": "1", "flex-shrink": "0", "width": "40px",
                     "box-sizing": "border-box"}):
            with self.subTest(css=css), self.assertRaises(MODULE.LayoutError):
                MODULE.LayoutEngine().document("Example",
                    container("root", children=[container("child", **css)]))
        children = [container(name, **{"flex-grow": "1", "flex-shrink": "0"})
                    for name in ("first", "second")]
        with self.assertRaisesRegex(MODULE.LayoutError, "MULTIPLE_FLEX_GROW"):
            MODULE.LayoutEngine().document("Example", container("root", children=children))

    def test_zero_flex_basis_replaces_the_main_axis_length(self):
        child = container("child", **{"flex-basis": "0", "flex-grow": "1",
                                      "width": "320px", "box-sizing": "border-box"})
        doc = MODULE.LayoutEngine().document("Example", container("root", "row", [child]))
        widget = doc.find("./widget/layout/item/widget")
        self.assertIsNone(widget.find("./property[@name='maximumSize']"))
        policy = widget.find("./property[@name='sizePolicy']/sizepolicy")
        self.assertEqual(policy.get("hsizetype"), "Expanding")
        self.assertEqual(doc.find("./widget/layout").get("stretch"), "1")

    def test_flex_basis_failures_stay_closed(self):
        cases = (({"flex-basis": "10px", "flex-grow": "1"}, "UNSUPPORTED_FLEX_BASIS"),
                 ({"flex-basis": "0"}, "FLEX_BASIS_ZERO_REQUIRES_FLEX_GROW"))
        for css, message in cases:
            with self.subTest(css=css):
                with self.assertRaisesRegex(MODULE.LayoutError, message):
                    MODULE.LayoutEngine().document(
                        "Example", container("root", "row", [container("child", **css)]))
        with self.assertRaisesRegex(MODULE.LayoutError, "FLEX_BASIS_PARENT_REQUIRED"):
            MODULE.LayoutEngine().document(
                "Example", container("root", **{"flex-basis": "0", "flex-grow": "1"}))

    def test_root_percentage_uses_initial_viewport_not_fixed_bounds(self):
        root = container("root", **{"width": "100%", "height": "100%",
                                   "box-sizing": "border-box"})
        root["viewport"] = {"width": 640, "height": 480, "source": "fixture viewport"}
        doc = MODULE.LayoutEngine().document("Example", root)
        self.assertEqual(doc.findtext("./widget/property[@name='geometry']/rect/width"), "640")
        self.assertIsNone(doc.find("./widget/property[@name='maximumSize']"))
        policy = doc.find("./widget/property[@name='sizePolicy']/sizepolicy")
        self.assertEqual(policy.get("hsizetype"), "Expanding")
        self.assertEqual(policy.get("vsizetype"), "Expanding")

    def test_viewport_requires_evidence_and_root(self):
        root = container("root")
        root["viewport"] = {"width": 640, "height": 480}
        with self.assertRaisesRegex(MODULE.LayoutError, "VIEWPORT_SOURCE"):
            MODULE.LayoutEngine().document("Example", root)
        root["viewport"]["source"] = "fixture"
        with self.assertRaisesRegex(MODULE.LayoutError, "ONLY_ON_PAGE_ROOT"):
            MODULE.LayoutEngine().document("Example", container("outer", children=[root]))

    def test_percentage_chain_retains_resizable_constraints(self):
        full = {"width": "100%", "height": "100%", "box-sizing": "border-box"}
        leaf = container("leaf", **full)
        middle = container("middle", "column", [leaf], **full)
        root = container("root", "row", [middle], **full)
        root["viewport"] = {"width": 640, "height": 480, "source": "fixture"}
        engine = MODULE.LayoutEngine()
        doc = engine.document("Example", root)
        self.assertEqual(engine.resolved_axes["leaf"], {"width", "height"})
        self.assertEqual(doc.findall(".//property[@name='maximumSize']"), [])
        self.assertEqual(doc.findall(".//spacer"), [])

    def test_main_percentage_with_sibling_refused(self):
        child = container("child", **{"width": "100%", "box-sizing": "border-box"})
        root = container("root", children=[child, container("sibling")],
                         **{"width": "640px", "box-sizing": "border-box"})
        with self.assertRaisesRegex(MODULE.LayoutError, "ONLY_CHILD"):
            MODULE.LayoutEngine().document("Example", root)


if __name__ == "__main__":
    unittest.main()
