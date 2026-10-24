import sys
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from assets import ResourceBundle, AssetError, css_url, validate_svg, bind_collections, bind_icons
from page_output import render_page


class AssetTests(unittest.TestCase):
    def test_system_icon_is_explicit_missing_only_fallback(self):
        bundle = ResourceBundle(Path(__file__).parent / "fixtures", "Example")
        record = {"declaredProps": {"Content": "icon-only"}, "content": {"images": []}}
        spec = {"leadingIcon": {"when": {"Content": "icon-only"},
                 "source": "single-visible-image", "missingFallback": "SP_FileIcon"}}
        slots, consumed, warnings = bind_icons(record, spec, bundle)
        self.assertEqual(slots["leadingIcon"], {"kind": "qt-standard", "name": "SP_FileIcon"})
        self.assertEqual(consumed, [])
        self.assertEqual(warnings[0]["reason"], "SOURCE_ICON_MISSING")
        record["content"]["images"] = [{"reference": "@/icon.svg", "kind": "image",
                                        "source": {"file": "Item.vue"}}]
        slots, consumed, warnings = bind_icons(record, spec, bundle)
        self.assertEqual(slots["leadingIcon"]["kind"], "resource")
        self.assertEqual(warnings, [])
        self.assertEqual(len(consumed), 1)
        record["content"]["images"][0]["reference"] = "https://invalid.test/a.svg"
        with self.assertRaisesRegex(AssetError, "LOCAL_ASSET"):
            bind_icons(record, spec, bundle)

    def test_collection_resources_and_values_bound_without_text_rewrite(self):
        bundle = ResourceBundle(Path(__file__).parent / "fixtures", "Example")
        collection = {"items": [{"text": {"text": "&nbsp;"}, "images": [{
            "reference": "url(@/icon.svg)", "kind": "css-background",
            "source": {"file": "Example.vue"}}]}], "selectedIndex": 0}
        spec = {"outputs": {"labels": "items", "icons": "icons", "selectedIndex": "current"}}
        slots = bind_collections({"main": collection}, {"main": spec}, bundle)
        self.assertEqual(slots["items"], ["&nbsp;"])
        self.assertEqual(slots["current"], 0)
        self.assertTrue(slots["icons"][0].startswith(":/pages/Example/assets/"))
        self.assertEqual(len(bundle.files), 1)
        spec["outputs"]["icons"] = "items"
        with self.assertRaisesRegex(AssetError, "DUPLICATE_OR_INVALID_COLLECTION_SLOT"):
            bind_collections({"main": collection}, {"main": spec}, bundle)

    def test_local_asset_deduplicated_and_registered(self):
        bundle = ResourceBundle(Path(__file__).parent / "fixtures", "Example")
        first = bundle.add("@/icon.svg", "Example.vue")
        self.assertEqual(first, bundle.add("icon.svg", "Example.vue"))
        self.assertEqual(len(bundle.files), 1)
        self.assertEqual(len(bundle.evidence), 1)
        root = {"kind": "container", "name": "root", "source": {"file": "fixture.vue"},
                "css": {"display": "flex"}, "children": []}
        mapping = {"schemaVersion": 1, "sdkRelease": "fixture", "profiles": ["fixture"], "components": []}
        mapping["pageHost"] = {"class": "QWidget", "header": "QWidget", "constructor": "parent-only"}
        files = render_page("Example", root, mapping, "fixture", "fixture", bundle)
        qrc = ET.fromstring(files["view/res/Example.qrc"])
        for resource in qrc.findall("./qresource/file"):
            self.assertIn("view/res/" + resource.text, files)
        self.assertEqual(len(files), 6)

    def test_network_and_absolute_paths_refused(self):
        bundle = ResourceBundle(Path(__file__).parent / "fixtures", "Example")
        for reference in ("https://example.test/icon.svg", "//server/icon.svg", "C:/icon.svg",
                          "data:image/png,abc", "/icon.svg", "icon.svg?x=1"):
            with self.subTest(reference=reference), self.assertRaises(AssetError):
                bundle.add(reference, "Example.vue")

    def test_traversal_refused(self):
        bundle = ResourceBundle(Path(__file__).parent / "fixtures", "Example")
        with self.assertRaisesRegex(AssetError, "OUTSIDE_INPUT"):
            bundle.add("../test_assets.py", "Example.vue")

    def test_css_url(self):
        for value in ('url(@/icon.svg)', 'url("@/icon.svg")', "url('@/icon.svg')"):
            self.assertEqual(css_url(value), "@/icon.svg")
        with self.assertRaises(AssetError):
            css_url("url(a.svg),url(b.svg)")

    def test_active_svg_refused(self):
        for source in (
            b'<svg><script>run()</script></svg>',
            b'<svg><image href="https://example.test/image.png"/></svg>',
            b'<!DOCTYPE svg><svg/>',
            b'<svg onload="run()"/>',
        ):
            with self.subTest(source=source), self.assertRaises(AssetError):
                validate_svg(source)


if __name__ == "__main__":
    unittest.main()
