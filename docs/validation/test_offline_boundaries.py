"""Python 3.9 offline orchestration and independent consumer boundary checks.

Fixtures are synthetic and never substitute for native or fresh-machine tests.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

WORKSPACE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(WORKSPACE / "Source"))
spec = importlib.util.spec_from_file_location(
    "tested_offline", WORKSPACE / "Source/offline/offline.py")
offline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(offline)
receipt_spec = importlib.util.spec_from_file_location(
    "tested_receipt", WORKSPACE / "Source/BuildReceipt.py")
receipt = importlib.util.module_from_spec(receipt_spec)
receipt_spec.loader.exec_module(receipt)


class OfflineBoundaryTests(unittest.TestCase):
    def setUp(self):
        base = WORKSPACE / "DesignToCode_GenerateLayout/output/work/offline-boundary-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(base))
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.package = self.base / "package"
        self.package.mkdir()
        self.work = self.base / "work"
        self.work.mkdir()
        self.config = {
            "node": "bin/node.exe", "browser": "bin/browser.exe", "compiler": "bin/compiler.exe",
            "uic": "bin/uic.exe", "gallery": "bin/gallery.exe",
            "font": "font.ttf", "mapping": "mapping",
        }
        (self.package / "mapping").mkdir()
        for value in self.config.values():
            path = self.package / value
            if path.is_dir():
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"test fixture only")
        platforms = self.package / "bin/platforms"
        platforms.mkdir()
        (platforms / "qwindows.dll").write_bytes(b"synthetic platform")
        (platforms / "qoffscreen.dll").write_bytes(b"synthetic platform")
        self.write_manifest()

    def write_manifest(self, change=None):
        manifest = {"schemaVersion": 1, "configuration": "Release",
                    "runtime": dict(self.config), "files": []}
        for path in self.package.rglob("*"):
            if path.is_file() and path.name != "release.json":
                data = path.read_bytes()
                manifest["files"].append({"path": path.relative_to(self.package).as_posix(),
                                          "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
        if change:
            change(manifest)
        (self.package / "release.json").write_bytes(json.dumps(manifest).encode("utf-8"))

    def test_complete_inventory(self):
        self.assertEqual(offline.verify(self.package)["configuration"], "Release")
        (self.package / "undeclared.dll").write_bytes(b"not listed")
        with self.assertRaisesRegex(ValueError, "INVENTORY"):
            offline.verify(self.package)

    def test_changed_payload_fails(self):
        (self.package / "font.ttf").write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError, "HASH"):
            offline.verify(self.package)

    def test_missing_offscreen_plugin_fails_before_launch(self):
        (self.package / "bin/platforms/qoffscreen.dll").unlink()
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, "QT_PLATFORM_PLUGIN_MISSING"):
            offline.verify(self.package)

    def test_manifest_path_cannot_escape(self):
        for value in ("../outside.txt", str(self.base / "outside.txt")):
            with self.subTest(value=value), self.assertRaises(ValueError):
                offline.inside(self.package, value)

    def test_case_insensitive_duplicate_paths_fail(self):
        self.write_manifest(lambda m: m["files"].append(
            {**m["files"][0], "path": m["files"][0]["path"].upper()}))
        with self.assertRaisesRegex(ValueError, "DUPLICATE"):
            offline.verify(self.package)

    def test_runtime_path_cannot_escape(self):
        self.write_manifest(lambda m: m["runtime"].update(browser="../external.exe"))
        with self.assertRaisesRegex(ValueError, "UNSAFE_MANIFEST_PATH"):
            offline.verify(self.package)

    def test_work_must_be_separate_in_both_directions(self):
        child = self.package / "inside"
        child.mkdir()
        for work in (self.package, child, self.base):
            with self.subTest(work=work), \
                    patch.object(offline, "__file__", str(self.package / "scripts/offline.py")), \
                    patch.object(sys, "argv", ["offline.py", "doctor", "--work", str(work)]), \
                    self.assertRaisesRegex(ValueError, "WORK_ROOT_MUST"):
                offline.main()
        self.assertFalse((self.package / "build").exists())

    def test_environment_removes_host_tool_fallbacks(self):
        with patch.dict("os.environ", {"PYTHONPATH": "host-python", "NODE_PATH": "host-node",
                                     "PATH": "host-tools", "SystemRoot": "C:/Windows"}, clear=True):
            env = offline.environment(self.package, self.work, self.config)
        self.assertNotIn("PYTHONPATH", env)
        self.assertNotIn("host-tools", env["PATH"])
        self.assertNotIn("host-node", env["NODE_PATH"])
        self.assertEqual(env["PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD"], "1")
        self.assertTrue(Path(env["TEMP"]).is_relative_to(self.work / "output/work"))

    def test_generation_uses_external_work_build_only(self):
        capture = self.work / "capture.json"
        capture.write_bytes(b"{}")
        calls = []
        def fake_run(command, work, env):
            calls.append((command, work))
            if "--output" in command:
                output = Path(command[command.index("--output") + 1])
                output.mkdir(parents=True)
                for suffix in (".ui", ".h", ".cpp", ".qss", ".qrc"):
                    (output / ("page" + suffix)).write_bytes(b"synthetic")
        with patch.object(offline, "__file__", str(self.package / "scripts/offline.py")), \
                patch.object(offline, "run", side_effect=fake_run), \
                patch.object(sys, "argv", ["offline.py", "generate", "--work", str(self.work),
                                          "--capture", str(capture), "--batch", "batch-1", "--page", "page_1"]):
            offline.main()
        self.assertEqual(len(calls), 3)
        self.assertTrue(all(work == self.work for _, work in calls))
        output = self.work / "output/delivery/batch-1/page_1"
        self.assertTrue((output / "page.ui").is_file())
        self.assertFalse((self.package / "build").exists())
        self.assertIn("--tool-root", calls[-1][0])
        self.assertIn("--work-root", calls[-1][0])


@unittest.skipUnless(sys.platform == "win32", "Independent consumer is a PowerShell entry")
class GalleryConsumerBoundaryTests(unittest.TestCase):
    def setUp(self):
        base = WORKSPACE / "ComponentLab/output/work/sdk-contract-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(base))
        self.addCleanup(self.temp.cleanup)
        self.sdk = Path(self.temp.name)
        (self.sdk / "manifest").mkdir()
        self.manifest = {
            "schemaVersion": 2, "package": "GeneratedComponentSDK",
            "owner": "DesignToCode_GenerateComponent",
            "componentAuthority": True, "tokenAuthority": True,
            "toolchain": {"configuration": "Release", "architecture": "x64",
                          "crt": "MD", "qtVersion": "5.15.2", "pythonVersion": "3.9.13",
                          "cmakeVersion": "3.24.2", "compilerVersion": "19.44", "compilerId": "MSVC"},
            "linkage": "static", "packageVersion": "1.0.0.1",
            "files": [],
        }

    def verify(self):
        (self.sdk / "manifest/component-sdk.json").write_bytes(json.dumps(self.manifest).encode("utf-8"))
        result = subprocess.run(
            ["powershell", "-NoProfile", "-File",
             str(WORKSPACE / "ComponentLab/Source/Verify-GeneratedComponentSDK.ps1"),
             "-Package", str(self.sdk), "-Configuration", "Release"],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return result

    def test_empty_sdk_cannot_pass_independent_consumer(self):
        result = self.verify()
        self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))
        self.assertIn(b"SDK_REQUIRED_FILE_MISSING", result.stderr)

    def test_release_cannot_claim_debug_crt(self):
        self.manifest["toolchain"]["crt"] = "MDd"
        result = self.verify()
        self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))
        self.assertIn(b"SDK_ABI_MISMATCH: crt", result.stderr)


class BuildReceiptCoverageTests(unittest.TestCase):
    def setUp(self):
        base = WORKSPACE / "DesignToCode_GenerateLayout/output/work/receipt-coverage-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(base))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.sdk = self.root / "fake-sdk"
        for name in (
            "fake-sdk/manifest/component-sdk.json",
            "DesignToCode_GenerateLayout/build/tools/layout/bin/v2_ui_compiler.exe",
            "ComponentLab/build/tools/gallery/component_lab.exe",
            "ComponentLab/build/tools/gallery/Qt5Core.dll",
            "DesignToCode_GenerateComponent/docs/designRules/design-system/component-traceability-manifest.json",
            "Source/entry.py", "docs/validation/test_entry.py",
        ):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"synthetic fixture")

    def test_deployed_dll_change_invalidates_receipt(self):
        with patch.object(receipt, "ROOT", self.root):
            before = receipt.current(self.sdk)
            (self.root / "ComponentLab/build/tools/gallery/Qt5Core.dll").write_bytes(b"changed deployment")
            after = receipt.current(self.sdk)
        self.assertNotEqual(before, after, "A verified exe cannot certify subsequently changed runtime DLLs.")

    def test_workspace_test_change_invalidates_receipt(self):
        with patch.object(receipt, "ROOT", self.root):
            before = receipt.inputs()
            (self.root / "docs/validation/test_entry.py").write_bytes(b"changed verification")
            after = receipt.inputs()
        self.assertNotEqual(before, after, "Root regression changes require a new verification receipt.")


if __name__ == "__main__":
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("Python 3.9 required.")
    unittest.main()
