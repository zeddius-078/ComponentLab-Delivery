import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "component-definitions/generated-design-component-catalog.json"


class FigmaCatalogContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads(CATALOG.read_text(encoding="utf-8"))

    def test_catalog_uses_figma_page_hierarchy(self):
        pages = self.catalog["figmaPages"]
        self.assertEqual(73, len(pages))
        self.assertEqual("20.1.01 · Navbar / 导航栏", pages[0]["name"])
        self.assertEqual("20.9.04 · Input & Progress Dialogs / 输入与进度对话框", pages[-1]["name"])

    def test_checkbox_sets_and_variants_match_figma(self):
        page = next(page for page in self.catalog["figmaPages"] if page["id"] == "234:2717")
        self.assertEqual(["DSM/Checkbox", "DSM/Checkbox/Group"],
                         [item["name"] for item in page["componentSets"]])
        checkbox = page["componentSets"][0]
        self.assertEqual(
            {
                "Size": ["LG", "SM"],
                "Value": ["Unchecked", "Checked", "Indeterminate"],
                "State": ["Default", "Hover", "Focus-visible", "Disabled", "Pressed"],
            },
            checkbox["variantAxes"],
        )
        self.assertEqual(30, len(checkbox["variants"]))

    def test_catalog_keeps_every_live_figma_component_set(self):
        sets = [item for page in self.catalog["figmaPages"] for item in page["componentSets"]]
        variants = [variant for item in sets for variant in item["variants"]]
        self.assertEqual(123, len(sets))
        self.assertEqual(1471, len(variants))

    def test_every_public_component_has_a_runtime_class(self):
        self.assertEqual(85, self.catalog["componentCount"])
        self.assertEqual(85, self.catalog["instantiableCount"])
        self.assertEqual(0, self.catalog["contractOnlyCount"])
        self.assertEqual(set(), {
            component["designComponent"]
            for component in self.catalog["components"]
            if not component["instantiable"] or component["runtimeStatus"] != "implemented"
        })
        self.assertEqual(set(), {
            component["designComponent"]
            for component in self.catalog["components"]
            if not component["sdkClass"].startswith("commonWidget::Z_")
        })

    def test_public_source_counts_use_current_inventory_without_new_widget_types(self):
        sets = {item["id"]: item for page in self.catalog["figmaPages"] for item in page["componentSets"]}
        for component in self.catalog["components"]:
            ids = component["componentSetId"].split(",")
            self.assertEqual(component["variantCount"],sum(len(sets[node_id]["variants"]) for node_id in ids))
        counts = {component["designComponent"]: component["variantCount"] for component in self.catalog["components"]}
        self.assertEqual(20,counts["ComponentLab/ProgressBar"])
        self.assertEqual(16,counts["ComponentLab/SpinBox"])
        self.assertEqual(16,counts["ComponentLab/DoubleSpinBox"])
        self.assertEqual(2,counts["ComponentLab/DesktopBottomNavigation"])
        self.assertNotIn("ComponentLab/MenuItem",counts)

    def test_progress_visual_evidence_matches_all_twenty_variants(self):
        doc = json.loads((ROOT / "component-definitions/figma-progress-visuals.json").read_text(encoding="utf-8"))
        page = next(page for page in self.catalog["figmaPages"] if page["id"]=="410:13")
        self.assertEqual({variant["name"] for variant in doc["variants"]},set(page["componentSets"][0]["variants"]))
        self.assertEqual(20,len(doc["variants"]))
        for variant in doc["variants"]:
            labels = [child for child in variant["children"] if child["type"]=="TEXT"]
            self.assertEqual(1,len(labels))
            self.assertEqual("State=Indeterminate" not in variant["name"],labels[0]["visible"])

    def test_button_captured_visuals_and_gallery_are_complete(self):
        document = json.loads((ROOT / "component-definitions/figma-button-visuals.json").read_text(encoding="utf-8"))
        self.assertEqual("2015:17140", document["galleryId"])
        self.assertEqual({"PRIMARY", "DEFAULT", "DASHED", "TEXT", "LINK", "DANGER", "ICONONLY"}, set(document["recipes"]))
        for recipe in document["recipes"].values():
            self.assertEqual({"default", "hover", "active", "focus-visible", "disabled", "loading"}, set(recipe))
        self.assertEqual([98, 53, 247, 1], document["recipes"]["PRIMARY"]["default"]["fill"])
        self.assertEqual([64, 150, 255, 1], document["recipes"]["DEFAULT"]["hover"]["stroke"])
        self.assertEqual([4, 3], document["recipes"]["DASHED"]["focus-visible"]["dash"])
        import xml.etree.ElementTree as ET
        assets = ROOT / "runtime/assets/icons/figma-button"
        self.assertEqual(8, len(list(assets.glob("*.svg"))))
        for asset in assets.glob("*.svg"):
            self.assertGreater(asset.stat().st_size, 100)
            svg = ET.parse(asset).getroot()
            self.assertEqual("16", svg.attrib["width"])
            self.assertEqual("16", svg.attrib["height"])

    def test_gallery_layout_evidence_preserves_gaps_and_node_geometry(self):
        document = json.loads((ROOT / "component-definitions/figma-gallery-layouts.json").read_text(encoding="utf-8"))
        self.assertEqual(self.catalog["figmaFileKey"], document["fileKey"])
        pages = {page["id"]: page for page in document["pages"]}
        self.assertEqual({page["id"] for page in self.catalog["figmaPages"]}, set(pages))
        missing = {key for key, page in pages.items() if page["status"] != "captured"}
        self.assertEqual(set(), missing)
        for key in missing:
            self.assertNotIn("galleries", pages[key])
            self.assertTrue(pages[key]["reason"])
        gallery = pages["234:2717"]["galleries"][0]
        self.assertEqual(["2015:20254", "Variant Gallery", "FRAME", 64, 1876, 1312, 991], gallery[:7])
        checkbox = gallery[7][1]
        self.assertEqual([206, 85, 900, 540], checkbox[3:7])
        self.assertEqual(30, len(checkbox[7]))
        self.assertEqual(4, len(gallery[7][2][7]))


if __name__ == "__main__":
    unittest.main()
