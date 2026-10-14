"""Verify the capture manifest is a real input contract, not a filename convention."""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[3]
TOOL = PROJECT / "Source/HtmlDsl/pipeline/capture/capture_manifest.py"


class CaptureManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="capture-manifest-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.html = self.root / "page.html"
        self.dsl = self.root / "page.json"
        self.font = self.root / "font.ttf"
        self.icon = self.root / "icon.svg"
        self.html.write_text(
            '<style>@font-face{src:url(http://local/font.ttf)}</style>'
            '<img src="http://local/icon.svg">',
            encoding="utf-8",
        )
        self.dsl.write_text(json.dumps({
            "pixTreeNodes": [{"guid": "1:1", "type": "FRAME", "childNode": []}]
        }), encoding="utf-8")
        self.font.write_bytes(b"font")
        self.icon.write_bytes(b"icon")

    def run_tool(self, *arguments):
        return subprocess.run(
            [sys.executable, "-B", str(TOOL), *arguments],
            capture_output=True,
            text=True,
            check=False,
        )

    def make_preview_manifest(self):
        output = self.root / "capture.json"
        result = self.run_tool(
            "create", "--output", str(output), "--html", str(self.html),
            "--dsl", str(self.dsl), "--root-id", "1:1", "--file-key", "fixture",
            "--font-family", "Fixture", "--manifest-type", "preview-fixture",
            "--resource", f"http://local/font.ttf={self.font}",
            "--resource", f"http://local/icon.svg={self.icon}",
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return output

    def test_create_and_verify_writes_relative_paths_and_hashes(self):
        manifest_path = self.make_preview_manifest()
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        self.assertEqual(manifest["manifestType"], "preview-fixture")
        self.assertFalse(Path(manifest["sourceHtml"]).is_absolute())
        self.assertEqual(len(manifest["sourceHashes"]["dslSha256"]), 64)
        result = self.run_tool("verify", "--manifest", str(manifest_path), "--mode", "preview")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_formal_manifest_requires_independent_layout_reference(self):
        manifest_path = self.make_preview_manifest()
        result = self.run_tool("verify", "--manifest", str(manifest_path), "--mode", "formal")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("FORMAL_CANNOT_USE_PREVIEW_FIXTURE", result.stderr + result.stdout)

    def test_root_mismatch_fails_before_generation(self):
        manifest_path = self.make_preview_manifest()
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["expectedRootId"] = "9:9"
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        result = self.run_tool("verify", "--manifest", str(manifest_path), "--mode", "preview")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("CAPTURE_ROOT_MISMATCH", result.stderr + result.stdout)


if __name__ == "__main__":
    unittest.main()
