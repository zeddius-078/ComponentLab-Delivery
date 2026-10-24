import copy
from pathlib import Path
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from assets import AssetError, ResourceBundle
from vector_asset import compose_canvas


def canvas():
    leaf = {"kind": "element", "visible": True, "children": [],
            "source": {"file": "Example.vue", "offset": 10},
            "style": {"values": {"position": "absolute", "width": "50%", "height": "50%",
                "left": "25%", "top": "25%", "background-size": "100% 100%",
                "background-repeat": "no-repeat", "background-image": "url(@/icon.svg)"},
                "origins": {"background-image": {"file": "Example.vue"}}}}
    return {"kind": "element", "visible": True, "source": {"file": "Example.vue"},
            "style": {"values": {"width": "16px", "height": "16px", "overflow": "hidden",
                                   "position": "absolute", "left": "0px", "top": "0px"}},
            "children": [leaf, copy.deepcopy(leaf)]}


class VectorAssetTests(unittest.TestCase):
    def bundle(self):
        return ResourceBundle(Path(__file__).parent / "fixtures", "Example")

    def test_order_geometry_and_reproducibility(self):
        one, two = self.bundle(), self.bundle()
        self.assertEqual(compose_canvas(canvas(), one), compose_canvas(canvas(), two))
        self.assertEqual(one.files, two.files)
        record = one.evidence[-1]
        self.assertEqual(record["slotPlacement"], "not-validated")
        self.assertEqual([x["rect"] for x in record["layers"]], [[4, 4, 8, 8], [4, 4, 8, 8]])
        svg = ET.fromstring(one.files["view/res/" + record["alias"]])
        self.assertEqual(len(svg), 2)
        self.assertEqual(svg[0].get("transform"), "translate(4 4) scale(0.5 0.5) translate(-0 -0)")
        self.assertEqual(len(list(svg.iter("{http://www.w3.org/2000/svg}svg"))), 1)

    def test_unsupported_styles_and_bounds_fail(self):
        for change in ({"transform": "rotate(45deg)"}, {"opacity": "0.5"},
                       {"left": "80%"}, {"width": "calc(50% - 1px)"},
                       {"background-repeat": "repeat"}, {"z-index": "2"}):
            node = canvas()
            node["children"][0]["style"]["values"].update(change)
            with self.subTest(change=change), self.assertRaises(AssetError):
                compose_canvas(node, self.bundle())

    def test_not_a_general_dom_rasterizer(self):
        for change in ("text", "nested", "unclipped"):
            node = canvas()
            if change == "text":
                node["children"].append({"kind": "text", "text": "Do not discard"})
            elif change == "nested":
                node["children"][0]["children"] = [{"kind": "element"}]
            else:
                node["style"]["values"]["overflow"] = "visible"
            with self.subTest(change=change), self.assertRaises(AssetError):
                compose_canvas(node, self.bundle())


if __name__ == "__main__":
    unittest.main()
