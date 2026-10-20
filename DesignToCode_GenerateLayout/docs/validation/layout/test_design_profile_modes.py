"""Keep source variable modes separate from SDK contexts at the real CLI boundary."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[3]
FIXTURE = PROJECT / "docs/fixtures/layout/design-inputs/review-page/canonical.raw.json"
COMPILER = PROJECT / "build/cache/V2-html-dsl/v2_ui_compiler.exe"


class DesignProfileModeTests(unittest.TestCase):
    def setUp(self):
        cache = PROJECT / "build/preview/design-profile-tests"
        cache.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(cache))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.original = FIXTURE.read_bytes()
        self.addCleanup(lambda: self.assertEqual(FIXTURE.read_bytes(), self.original))
        self.raw = json.loads(self.original)

    def generate(self):
        source = self.root / "input.json"
        source.write_bytes(json.dumps(self.raw, ensure_ascii=False).encode("utf-8"))
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
        if env.get("QT_ROOT"):
            env["PATH"] = str(Path(env["QT_ROOT"]) / "bin") + os.pathsep + env.get("PATH", "")
        return subprocess.run([
            str(COMPILER), "--raw", str(source),
            "--mapping", str(PROJECT / "docs/fixtures/layout/mapping"),
            "--extensions", str(PROJECT / "docs/fixtures/layout/mapping/extensions.json"),
            "--font", str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
            "--output", str(self.root / "page"), "--strict-layout",
        ], capture_output=True, env=env, timeout=30)

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


if __name__ == "__main__":
    unittest.main()
