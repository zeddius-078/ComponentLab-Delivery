"""Small publication contract tests; no native build and no installed-tool mutation."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

PROJECT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "gallery_manifest", PROJECT / "Source/delivery/tools/DeliveryManifest.py")
gallery = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gallery)


class GalleryPublicationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if sys.version_info[:2] != (3, 9):
            raise RuntimeError("PYTHON39_REQUIRED")
        base = PROJECT / "output/work/tests"
        base.mkdir(parents=True, exist_ok=True)
        cls.work = Path(tempfile.mkdtemp(prefix="gallery-publication-", dir=str(base)))

    def setUp(self):
        self.acceptance = {
            "project": PROJECT.name, "status": "PASS", "checks": ["ctest", "token", "native"],
            "sourceFingerprint": "source", "sdkRoot": str(self.work / "sdk"),
            "sdkManifestSha256": "sdk", "stageFiles": {
                "component_lab.exe": {"sha256": "exe", "bytes": 3},
                "Run.cmd": {"sha256": "run", "bytes": 3}}}
        self.manifest = {"acceptance": self.acceptance, "files": self.acceptance["stageFiles"]}
        self.lifecycle = mock.Mock()
        self.lifecycle.verify_tools.return_value = self.manifest
        self.lifecycle.fingerprint.return_value = "source"
        self.contract = mock.Mock()
        self.contract.sha.return_value = "sdk"

    def validate(self, receipt=None):
        return gallery.validate(self.work / "gallery", receipt, self.lifecycle, self.contract)

    def test_current_explicit_receipt_passes(self):
        receipt = self.work / "receipt.json"
        receipt.write_bytes(json.dumps(self.acceptance).encode("utf-8"))
        self.assertEqual(self.validate(receipt), self.manifest)

    def test_missing_validation_never_uses_old_logs(self):
        self.acceptance["checks"] = ["ctest"]
        with self.assertRaisesRegex(ValueError, "FULL_VALIDATION_REQUIRED"):
            self.validate()

    def test_stale_source_rejected(self):
        self.lifecycle.fingerprint.return_value = "changed"
        with self.assertRaisesRegex(ValueError, "SOURCE_CHANGED"):
            self.validate()

    def test_stale_sdk_rejected(self):
        self.contract.sha.return_value = "changed"
        with self.assertRaisesRegex(ValueError, "SDK_CHANGED"):
            self.validate()

    def test_explicit_receipt_cannot_override_installed_acceptance(self):
        changed = copy.deepcopy(self.acceptance)
        changed["sourceFingerprint"] = "different"
        path = self.work / "different-receipt.json"
        path.write_bytes(json.dumps(changed).encode("utf-8"))
        with self.assertRaisesRegex(ValueError, "EXPLICIT_RECEIPT"):
            self.validate(path)

    def test_package_refuses_outside_delivery_and_overwrite(self):
        with mock.patch.object(gallery, "support_modules", return_value=(self.lifecycle, self.contract)):
            with self.assertRaisesRegex(ValueError, "OUTPUT_DELIVERY"):
                gallery.package(self.work / "gallery", self.work / "bad.zip")
            fake_project = self.work / "package-fixture"
            existing = fake_project / "output/delivery/exists.zip"
            existing.parent.mkdir(parents=True, exist_ok=True)
            existing.write_bytes(b"unchanged")
            with mock.patch.object(gallery, "PROJECT", fake_project):
                with self.assertRaisesRegex(ValueError, "PACKAGE_ALREADY_EXISTS"):
                    gallery.package(self.work / "gallery", existing)
            self.assertEqual(existing.read_bytes(), b"unchanged")

    def test_package_contains_only_runtime_and_release_manifest(self):
        fake_project = self.work / "isolated-package"
        dist = fake_project / "build/tools/gallery"
        dist.mkdir(parents=True, exist_ok=True)
        (dist / "component_lab.exe").write_bytes(b"synthetic-test-not-a-real-exe")
        (dist / "Run.cmd").write_bytes(b"synthetic")
        output = fake_project / "output/delivery/example/ComponentLab.zip"
        inventory = {p.name: {"bytes": p.stat().st_size,
                             "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
                     for p in dist.iterdir()}
        lifecycle = mock.Mock()
        lifecycle.inventory.return_value = inventory
        lifecycle.operation_lock.return_value = mock.MagicMock()
        contract = mock.Mock()
        contract.sha.return_value = "archive-hash"
        with mock.patch.object(gallery, "PROJECT", fake_project), \
             mock.patch.object(gallery, "support_modules", return_value=(lifecycle, contract)), \
             mock.patch.object(gallery, "validate", return_value=self.manifest):
            gallery.package(dist, output)
        with zipfile.ZipFile(str(output)) as archive:
            self.assertEqual(set(archive.namelist()),
                             {"ComponentLab/Run.cmd", "ComponentLab/component_lab.exe",
                              "ComponentLab/DELIVERY.json"})

    def test_powershell_rejects_source_and_build_execution_outputs(self):
        shell = os.environ.get("PSHOME")
        executable = str(Path(shell) / "pwsh.exe") if shell else "pwsh"
        helper = str(PROJECT / "Source/ProjectTools.ps1").replace("'", "''")
        for path in (PROJECT / "build/cache/bad", PROJECT / "Source/bad",
                     PROJECT / "output/work/../../build/bad"):
            command = ". '" + helper + "'; Assert-ProjectOutput '" + str(path).replace("'", "''") + "'"
            result = subprocess.run([executable, "-NoProfile", "-Command", command],
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("OUTPUT_OUTSIDE_WORK", result.stderr)

    def run_log_retention(self, project, source, destination, prelude=""):
        helper = str(PROJECT / "Source/ProjectTools.ps1").replace("'", "''")
        script = str(PROJECT / "Source/delivery/tools/Build.ps1").replace("'", "''")
        quote = lambda path: "'" + str(path).replace("'", "''") + "'"
        command = (
            "$ErrorActionPreference='Stop'; . '" + helper + "'; "
            "$ComponentLabProject=" + quote(project) + "; "
            "$tokens=$null; $errors=$null; "
            "$ast=[System.Management.Automation.Language.Parser]::ParseFile('" + script
            + "',[ref]$tokens,[ref]$errors); if($errors.Count){throw 'PARSE_ERROR'}; "
            "$fn=$ast.Find({param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst]"
            " -and $n.Name -eq 'Complete-BuildLogRun'},$true); "
            ". ([scriptblock]::Create($fn.Extent.Text)); " + prelude
            + " Complete-BuildLogRun -Logs " + quote(source) + " -Destination " + quote(destination))
        return subprocess.run(["pwsh", "-NoProfile", "-Command", command],
                              capture_output=True, text=True)

    def test_build_log_retention_keeps_one_run_and_preserves_previous(self):
        project = self.work / "log-retention"
        source = project / "output/logs/build-current"
        destination = project / "output/logs/latest-success/run"
        source.mkdir(parents=True)
        destination.mkdir(parents=True)
        (source / "transcript.log").write_bytes(b"current")
        (destination / "transcript.log").write_bytes(b"previous")
        result = self.run_log_retention(project, source, destination)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(source.exists())
        self.assertEqual((destination / "transcript.log").read_bytes(), b"current")
        obsolete = list((project / "output/work").glob("obsolete-log-*"))
        self.assertEqual(len(obsolete), 1)
        self.assertEqual((obsolete[0] / "transcript.log").read_bytes(), b"previous")

    def test_build_log_retention_rejects_destination_outside_logs(self):
        project = self.work / "log-escape"
        source = project / "output/logs/build-current"
        source.mkdir(parents=True)
        (source / "transcript.log").write_bytes(b"untouched")
        result = self.run_log_retention(project, source, project / "Source/run")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OUTPUT_OUTSIDE_LOGS", result.stderr)
        self.assertEqual((source / "transcript.log").read_bytes(), b"untouched")

    def test_build_log_retention_rejects_reparse_descendant_before_moving(self):
        project = self.work / "log-reparse"
        source = project / "output/logs/build-current"
        source.mkdir(parents=True)
        (source / "link-marker").write_bytes(b"synthetic-reparse-test")
        # G: may be exFAT. Emulate only the Get-Item attribute rather than create
        # a system junction or require administrator/developer-mode changes.
        prelude = (
            "function Get-Item {param([string]$LiteralPath,[switch]$Force) "
            "if($LiteralPath -like '*link-marker'){"
            "[pscustomobject]@{Attributes=[IO.FileAttributes]::ReparsePoint}"
            "}else{Microsoft.PowerShell.Management\\Get-Item -LiteralPath $LiteralPath -Force:$Force}};")
        result = self.run_log_retention(
            project, source, project / "output/logs/latest-failure/run", prelude)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OUTPUT_PATH_IS_LINK", result.stderr)
        self.assertTrue((source / "link-marker").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
