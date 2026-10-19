"""Named, data-only target libraries: real CLI generation, no target implementations."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

PROJECT = Path(__file__).resolve().parents[3]
FIXTURES = PROJECT / "docs/fixtures/layout"
COMPILER = PROJECT / "build/cache/V2-html-dsl/v2_ui_compiler.exe"


class ComponentLibraryTests(unittest.TestCase):
    def setUp(self):
        scratch = PROJECT / "build/preview/library-contract-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(scratch))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.mapping = self.root / "mapping"
        shutil.copytree(FIXTURES / "mapping", self.mapping)
        self.raw = json.loads((FIXTURES / "design-inputs/review-page/canonical.raw.json").read_bytes())
        self.library_path = self.mapping / "component-libraries/analysis-system.json"
        self.library = json.loads(self.library_path.read_bytes())
        self.environment = dict(os.environ, QT_QPA_PLATFORM="offscreen")

    def generate(self, profile="analysis-system", name="page", formal=False):
        source = self.root / "input.json"
        source.write_bytes(json.dumps(self.raw, ensure_ascii=False).encode("utf-8"))
        output = self.root / name
        args = [str(COMPILER), "--raw", str(source), "--mapping", str(self.mapping),
                "--extensions", str(self.mapping / "extensions.json"), "--font",
                str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
                "--output", str(output), "--strict-layout"]
        if profile is not None:
            args += ["--library-profile", profile]
        if formal:
            args.append("--formal")
        result = subprocess.run(args, env=self.environment, capture_output=True, timeout=45)
        return result, output

    def accepted(self, profile="analysis-system", name="page"):
        result, output = self.generate(profile, name)
        self.assertEqual(result.returncode, 0, result.stderr.decode("utf-8", errors="replace"))
        return ET.parse(str(output) + ".ui"), json.loads(Path(str(output) + ".components.json").read_bytes())

    def rejected(self, error, **kwargs):
        result, output = self.generate(**kwargs)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(error, result.stderr.decode("utf-8", errors="replace"))
        self.assertFalse(Path(str(output) + ".ui").exists())

    def save_library(self):
        self.library_path.write_bytes(json.dumps(self.library, ensure_ascii=False).encode("utf-8"))

    def test_same_input_two_libraries_and_unchanged_default(self):
        token, old_manifest = self.accepted("token-sdk", "token")
        default, _ = self.accepted(None, "default")
        self.assertEqual(ET.tostring(token.getroot()), ET.tostring(default.getroot()))
        self.assertNotIn("componentLibrarySha256", old_manifest)
        custom, manifest = self.accepted()
        token_classes = {w.get("class") for w in token.findall(".//widget")}
        classes = {w.get("class") for w in custom.findall(".//widget")}
        self.assertIn("commonWidget::Z_PushButton", token_classes)
        self.assertIn("Z_PushButton", classes)
        self.assertIn("Z_LineEdit", classes)
        self.assertFalse(any(c.startswith("commonWidget::") for c in classes))
        self.assertEqual(manifest["implementationStatus"], "planned")
        self.assertEqual(manifest["componentLibrarySha256"], hashlib.sha256(self.library_path.read_bytes()).hexdigest())
        declarations = {c.findtext("class"): c for c in custom.findall(".//customwidget")}
        self.assertEqual(declarations["Z_PushButton"].findtext("extends"), "QPushButton")
        self.assertEqual(declarations["Z_PushButton"].findtext("header"), "AnalysisWidgets/Z_PushButton.h")
        self.assertEqual([l.get("class") for l in token.findall(".//layout")],
                         [l.get("class") for l in custom.findall(".//layout")])

    def test_arbitrary_third_library_needs_only_json_changes(self):
        index_path = self.mapping / "library-profiles.json"
        index = json.loads(index_path.read_bytes())
        profile = copy.deepcopy(index["profiles"]["analysis-system"])
        profile["componentLibrary"] = "component-libraries/third.json"
        index["profiles"]["third-system"] = profile
        index_path.write_bytes(json.dumps(index).encode("utf-8"))
        self.library["id"] = "third-system"
        self.library["headerPrefix"] = "Third/"
        self.library["classPrefix"] = "Third_"
        (self.mapping / profile["componentLibrary"]).write_bytes(json.dumps(self.library).encode("utf-8"))
        ui, manifest = self.accepted("third-system")
        self.assertEqual(manifest["libraryId"], "third-system")
        self.assertIn("Third_PushButton", {w.get("class") for w in ui.findall(".//widget")})

    def test_one_prefix_switches_every_target_class_and_header(self):
        original, before = self.accepted(name="before")
        self.library["classPrefix"] = "Acme_"
        self.save_library()
        switched, after = self.accepted(name="after")
        old = {c.findtext("class"): c for c in original.findall(".//customwidget")}
        new = {c.findtext("class"): c for c in switched.findall(".//customwidget")}
        self.assertEqual({"Acme_" + name[2:] for name in old}, set(new))
        for name, declaration in new.items():
            self.assertEqual(declaration.findtext("header"), "AnalysisWidgets/" + name + ".h")
            self.assertEqual(declaration.findtext("extends"), old["Z_" + name[5:]].findtext("extends"))
        self.assertEqual([c["libraryComponentId"] for c in before["components"]],
                         [c["libraryComponentId"] for c in after["components"]])

    def test_duplicate_target_class_rejected(self):
        self.library["components"][1]["classSuffix"] = self.library["components"][0]["classSuffix"]
        self.save_library()
        self.rejected("DUPLICATE_LIBRARY_ID_OR_CLASS")

    def test_unknown_component_is_not_guessed(self):
        self.library["components"] = [e for e in self.library["components"] if e["id"] != "QPushButton"]
        self.save_library()
        self.rejected("UNMAPPED_LIBRARY_COMPONENT: QPushButton")

    def test_invalid_header_rejected(self):
        self.library["headerPrefix"] = "../Outside/"
        self.save_library()
        self.rejected("INVALID_LIBRARY_HEADER")

    def test_prefix_cannot_impersonate_native_qt_classes(self):
        self.library["classPrefix"] = "Q"
        self.save_library()
        self.rejected("LIBRARY_TARGET_COLLIDES_WITH_QT_BASE")

    def test_planned_library_cannot_be_published_as_formal(self):
        self.rejected("CONTRACT_ONLY_LIBRARY_REQUIRES_PREVIEW", formal=True)

    def test_every_widget_declaration_can_emit_and_pass_uic(self):
        # Synthetic fixture checks type declarations, not actual design completeness or implementations.
        widgets = [e for e in self.library["components"] if e["kind"] == "widget"]
        root = self.raw["pixTreeNodes"][0]
        root["childNode"] = []
        self.raw["pixComponentTreeDslNodes"] = []
        rules_path = self.mapping / "component-mapping.json"
        rules = json.loads(rules_path.read_bytes())
        rules["rules"] = []
        names_path = self.mapping / "engineering-names.json"
        names = json.loads(names_path.read_bytes())
        for number, entry in enumerate(widgets):
            master, instance = "9000:" + str(number), "9001:" + str(number)
            family = "9002:" + str(number)
            identity = "Fixture/" + entry["id"]
            self.raw["pixComponentTreeDslNodes"].append({"guid": family, "name": identity,
                "componentKey": family, "type": "COMPONENT_SET", "childNode": [
                    {"guid": master, "componentKey": master, "type": "COMPONENT", "childNode": []}]})
            root["childNode"].append({"guid": instance, "name": "libraryItem" + str(number),
                "type": "INSTANCE", "mainComponent": master, "mainStateGroup": family, "componentKey": master,
                "width": 120, "height": 32, "propRefMap": {"variant": []}})
            names[instance] = "libraryItem" + str(number)
            rules["rules"].append({"designComponentSetId": family, "designComponentName": identity,
                "qtClass": entry["qtBase"], "propertyMappings": {}, "stateMappings": {}})
            self.library["designBindings"][identity] = entry["id"]
        rules_path.write_bytes(json.dumps(rules).encode("utf-8"))
        names_path.write_bytes(json.dumps(names).encode("utf-8"))
        self.save_library()
        ui, manifest = self.accepted()
        classes = {c.findtext("class") for c in ui.findall(".//customwidget")}
        self.assertTrue({self.library["classPrefix"] + e["classSuffix"] for e in widgets}.issubset(classes))
        self.assertEqual(len(manifest["components"]), len(widgets))
        uic = Path(os.environ["QT_ROOT"]) / "bin/uic.exe"
        result = subprocess.run([str(uic), str(self.root / "page.ui")], capture_output=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.library["classPrefix"] = "Acme_"
        self.save_library()
        switched, _ = self.accepted(name="prefixed")
        switched_classes = {c.findtext("class") for c in switched.findall(".//customwidget")}
        self.assertTrue({"Acme_" + e["classSuffix"] for e in widgets}.issubset(switched_classes))
        result = subprocess.run([str(uic), str(self.root / "prefixed.ui")], capture_output=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
