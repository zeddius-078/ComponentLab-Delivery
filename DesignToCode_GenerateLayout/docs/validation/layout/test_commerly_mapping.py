"""Commerly blocked-profile guards. Native probes are synthetic, not captured DSL."""
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


PROJECT = Path(__file__).resolve().parents[3]
MAPPING = PROJECT / "docs/fixtures/layout/mapping-commerly"
COMPILER = PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe"


def read(name):
    return json.loads((MAPPING / name).read_bytes())


class CommerlyMappingTests(unittest.TestCase):
    def test_python_baseline(self):
        self.assertEqual(sys.version_info[:2], (3, 9))

    def test_inventory_is_blocked_and_unmodified(self):
        inventory = read("inventory.json")
        self.assertEqual(inventory["sourceFileKey"], "HUTs4r6S0Eb3QzdijQyAzA")
        self.assertEqual(inventory["status"], "BLOCKED_NOT_GENERATION_READY")
        self.assertEqual(len(inventory["items"]), 16)
        self.assertEqual(len({x["id"] for x in inventory["items"]}), 16)
        self.assertEqual(inventory["renames"], [])
        for item in inventory["items"]:
            self.assertTrue(item["key"])
            self.assertTrue(item["reasons"])
            self.assertEqual(item["action"], "KEEP_UNCHANGED")
            self.assertEqual(item["status"], "BLOCKED")

    def test_no_speculative_rules_or_external_identity_fallback(self):
        self.assertEqual(read("component-mapping.json")["rules"], [])
        self.assertEqual(read("engineering-names.json"), {})
        self.assertNotIn("selectOption", read("extensions.json"))
        profiles = read("library-profiles.json")
        self.assertEqual(profiles["defaultProfile"], "token-sdk")
        self.assertEqual(set(profiles["profiles"]), {"token-sdk"})
        self.assertEqual(profiles["profiles"]["token-sdk"]["componentOverrides"], {})

    def test_local_checkbox_visibility_gap_is_recorded(self):
        checkbox = next(x for x in read("inventory.json")["items"] if x["id"] == "37:4667")
        self.assertIn("BOOLEAN_VISIBILITY_UNSUPPORTED", checkbox["reasons"])
        self.assertEqual(checkbox["componentProperties"]["Show label#46:6"]["type"], "BOOLEAN")
        self.assertEqual(checkbox["variantNames"], ["State=Unchecked", "State=Checked"])

    def test_native_compiler_rejects_every_unapproved_family(self):
        self.assertTrue(COMPILER.is_file(), "Build with the existing Python 3.9 toolchain first")
        scratch = PROJECT / "output/work/commerly-mapping/native-probes"
        scratch.mkdir(parents=True, exist_ok=True)
        base = json.loads((PROJECT / "docs/examples/offline-capture/canonical.raw.json").read_bytes())
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", D2C_RUNTIME_ROOT=str(PROJECT / "build/tools/layout"), D2C_WORK_ROOT=str(PROJECT))
        qt = Path(os.environ.get("QT_ROOT", "D:/QT/5.15.2/msvc2019_64"))
        self.assertTrue((qt / "bin/Qt5Core.dll").is_file(), "Specify an existing QT_ROOT; do not install")
        env["PATH"] = str(qt / "bin") + os.pathsep + env.get("PATH", "")
        with tempfile.TemporaryDirectory(dir=str(scratch)) as temp:
            directory = Path(temp)
            for index, family in enumerate(read("inventory.json")["items"]):
                with self.subTest(family=family["name"], identity=family["id"]):
                    raw = copy.deepcopy(base)
                    raw["_fixtureNotice"] = "SYNTHETIC rejection probe; not a Pixso capture."
                    master_id = "synthetic:master"
                    definition = {"guid": family["id"], "name": family["name"],
                                  "componentKey": family["key"], "type": "FRAME"}
                    instance = {"guid": "synthetic:instance", "type": "INSTANCE",
                                "name": "blockedComponent", "componentKey": "synthetic-key",
                                "width": 160, "height": 30, "left": 24, "top": 24,
                                "autoLayout": {"stackPrimarySizing": "FIXED",
                                               "stackCounterSizing": "FIXED"}}
                    if family["type"] == "COMPONENT_SET":
                        definition["childNode"] = [{"guid": master_id, "componentKey": "synthetic-key"}]
                        instance.update(mainStateGroup=family["id"], mainComponent=master_id)
                    else:
                        instance.update(mainComponent=family["id"], componentKey=family["key"])
                    raw["pixTreeNodes"][0]["childNode"] = [instance]
                    raw["pixComponentTreeDslNodes"] = [definition]
                    source = directory / ("input-%d.json" % index)
                    source.write_bytes(json.dumps(raw).encode("utf-8"))
                    output = directory / ("output-%d" % index)
                    result = subprocess.run([
                        str(COMPILER), "--raw", str(source), "--mapping", str(MAPPING),
                        "--extensions", str(MAPPING / "extensions.json"),
                        "--font", str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
                        "--output", str(output), "--strict-layout",
                    ], env=env, capture_output=True, timeout=30)
                    self.assertNotEqual(result.returncode, 0)
                    # Current SceneBuilder.option requires mainStateGroup before
                    # ComponentConverter's standalone fallback can run. Record
                    # this distinct blocker; do not synthesize a fake set ID.
                    expected = (b"UNMAPPED_COMPONENT" if family["type"] == "COMPONENT_SET"
                                else b"REFERENCE_MISSING")
                    self.assertIn(expected, result.stderr)
                    self.assertFalse(output.with_suffix(".ui").exists())


if __name__ == "__main__":
    unittest.main()
