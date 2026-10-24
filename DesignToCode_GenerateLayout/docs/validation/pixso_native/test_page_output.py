"""Static page-unit contract; not an SDK compile/runtime acceptance test."""
import sys
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from page_output import render_page, surface_color, page_surfaces


class PageTests(unittest.TestCase):
    def setUp(self):
        self.mapping = {"schemaVersion": 1, "sdkRelease": "fixture",
                        "profiles": ["fixture"], "components": [],
                        "pageHost": {"class": "designer_widgets::ThemeHost",
                                     "header": "MyDesignSystem/DesignerWidgets.h",
                                     "constructor": "profile-parent"}}
        self.root = {"kind": "container", "name": "root", "source": {"file": "fixture.vue"},
                     "css": {"display": "flex"}, "children": []}

    def render(self):
        return render_page("Example", self.root, self.mapping, "fixture", "fixture")

    def test_exact_five_files(self):
        self.assertEqual(set(self.render()), {
            "form/Example.ui", "include/Example.h", "src/Example.cpp",
            "view/res/qss/Example.qss", "view/res/Example.qrc"})

    def test_resource_path_is_relative_and_runtime_path_matches(self):
        files = self.render()
        qrc = ET.fromstring(files["view/res/Example.qrc"])
        self.assertEqual(qrc.find("./qresource/file").text, "qss/Example.qss")
        self.assertIn(b":/pages/Example/qss/Example.qss", files["src/Example.cpp"])
        self.assertEqual(ET.fromstring(files["form/Example.ui"]).find(
            "./resources/include").get("location"), "../view/res/Example.qrc")

    def test_no_runtime_replayer_or_source_path(self):
        for value in self.render().values():
            for forbidden in (b"node.exe", b"python.exe", b"Vue", b"JSON.parse", b"output/work"):
                self.assertNotIn(forbidden, value)

    def test_stable_and_lf(self):
        self.assertEqual(self.render(), self.render())
        self.assertTrue(all(b"\r\n" not in value for value in self.render().values()))

    def test_theme_host_and_static_setup(self):
        files = self.render()
        self.assertIn(b"designer_widgets::ThemeHost", files["include/Example.h"])
        self.assertIn(b"ui->setupUi(this)", files["src/Example.cpp"])

    def test_alternate_host_is_configuration_only_not_an_sdk_claim(self):
        self.mapping["pageHost"] = {
            "class": "QWidget", "header": "QWidget", "constructor": "parent-only"}
        files = self.render()
        self.assertIn(b"class Example final : public QWidget", files["include/Example.h"])
        self.assertIn(b": QWidget(parent)", files["src/Example.cpp"])
        for data in files.values():
            self.assertNotIn(b"MyDesignSystem", data)
            self.assertNotIn(b"ThemeHost", data)

    def test_missing_or_unsafe_host_fails(self):
        for host in (None, {"class": "Bad;Code", "header": "QWidget", "constructor": "parent-only"},
                     {"class": "QWidget", "header": "../outside.h", "constructor": "parent-only"},
                     {"class": "QWidget", "header": "QWidget", "constructor": "arbitrary()"},
                     {"class": "Example", "header": "QWidget", "constructor": "parent-only"}):
            with self.subTest(host=host), self.assertRaises(ValueError):
                self.mapping["pageHost"] = host
                self.render()

    def test_surface_style_is_scoped_and_not_geometry(self):
        files = render_page("Example", self.root, self.mapping, "fixture", "fixture",
                            page_styles={"root": {"background-color": "rgba(17, 34, 51, 1)"}})
        self.assertIn(b"QWidget#root { background-color: rgba(17,34,51,255); }",
                      files["view/res/qss/Example.qss"])
        self.assertIn(b"this->setAttribute(Qt::WA_StyledBackground, true);",
                      files["src/Example.cpp"])

    def test_unknown_targets_properties_and_injection_refused(self):
        for styles in ({"missing": {"background-color": "#fff"}},
                       {"root": {"font-size": "14px"}},
                       {"root": {"width": "40px"}},
                       {"root": {"background-color": "red; } QWidget {color:red"}}):
            with self.subTest(styles=styles), self.assertRaises(ValueError):
                render_page("Example", self.root, self.mapping, "fixture", "fixture",
                            page_styles=styles)

    def test_alpha_and_hex_are_explicit(self):
        self.assertEqual(surface_color("rgba(0, 1, 2, 0.5)"), "rgba(0,1,2,128)")
        self.assertEqual(surface_color("#AaBbCc"), "#aabbcc")
        for invalid in ("#ff000080", "rgba(256,0,0,1)", "rgba(0,0,0,2)", "var(--a)"):
            with self.assertRaises(ValueError):
                surface_color(invalid)

    def test_sdk_surface_override_refused(self):
        document = ET.fromstring(
            '<ui><widget class="QWidget" name="root"><widget class="SdkWidget" name="sdk"/></widget></ui>')
        with self.assertRaisesRegex(ValueError, "OWN_CONTAINER"):
            page_surfaces(document, {"sdk": {"background-color": "#fff"}})


if __name__ == "__main__":
    unittest.main()
