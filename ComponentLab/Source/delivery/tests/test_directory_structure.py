"""Check that ComponentLab remains a display-only SDK consumer."""
from pathlib import Path
import json
import os
import unittest

SOURCE = Path(__file__).resolve().parents[2]
PROJECT = SOURCE.parent
SDK = Path(os.environ.get(
    "GENERATED_COMPONENT_SDK_ROOT",
    PROJECT.parent / "DesignToCode_GenerateComponent" / "build" / "sdk",
)).resolve()


class DirectoryTests(unittest.TestCase):
    def test_source_roles(self):
        self.assertEqual({p.name for p in SOURCE.iterdir() if p.is_dir()},
                         {"delivery", "presentation"})
        self.assertFalse((SOURCE / "runtime").exists())
        self.assertFalse((SOURCE / "design-assets").exists())

    def test_published_sdk_identity(self):
        manifest = json.loads((SDK / "manifest/component-sdk.json").read_bytes())
        self.assertEqual(manifest["owner"], "DesignToCode_GenerateComponent")
        self.assertTrue(manifest["componentAuthority"])
        if manifest.get("packageKind") == "qt-native-baseline":
            self.assertEqual(manifest["publicationState"], "baseline-native")
            self.assertFalse(manifest["tokenAuthority"])
            self.assertTrue(manifest["visualQssAuthority"])
            self.assertFalse(manifest["runtimeTokenJson"])
            self.assertEqual(manifest["nativeComponentCount"], 59)
        else:
            self.assertTrue(manifest["tokenAuthority"])
        self.assertIn(manifest["linkage"], ("static", "shared"))

    def test_no_component_or_token_compiler(self):
        names = {p.name for p in SOURCE.rglob("*") if p.is_file()}
        for forbidden in ("Z_PushButton.cpp", "Z_CheckBox.cpp", "Z_InputField.cpp",
                          "Z_Select.cpp", "token_compiler.cpp", "export_tokens.py"):
            self.assertNotIn(forbidden, names)
        cmake = (SOURCE / "CMakeLists.txt").read_text(encoding="utf-8-sig")
        self.assertNotIn("COMPONENTLAB_BUILD_GENERATORS", cmake)
        self.assertIn("GeneratedComponentSDK::Components", cmake)


if __name__ == "__main__":
    unittest.main()
