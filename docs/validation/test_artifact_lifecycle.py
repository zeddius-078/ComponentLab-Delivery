"""Synthetic failure tests, not native acceptance."""
import json
from pathlib import Path
import sys
import tempfile
import shutil
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Source"))
import ArtifactLifecycle as lifecycle
import SDKContract


class LifecycleTests(unittest.TestCase):
    def setUp(self):
        base = ROOT / "DesignToCode_GenerateLayout/output/work/lifecycle-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=base)
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name)
        for name in ("Source/main.py", "build/sdk/keep.bin", "output/delivery/batch/keep.bin",
                     "output/work/stage/tool.exe", "output/logs/latest-success/receipt.json"):
            path = self.project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"fixture")
        self.stage = self.project / "output/work/stage"
        self.sdk = self.project / "build/sdk"
        (self.sdk / "manifest").mkdir()
        (self.sdk / "manifest/component-sdk.json").write_bytes(b"{}")
        self.record = self.project / "output/logs/latest-success/receipt.json"
        with patch.object(lifecycle, "verify_sdk"):
            lifecycle.receipt(self.project, self.stage, self.sdk, lifecycle.fingerprint(self.project),
                              "ctest,token,native", self.record)

    def test_cleanup_preserves_controlled_assets_and_deliveries(self):
        lifecycle.clean(self.project)
        self.assertTrue((self.sdk / "keep.bin").is_file())
        self.assertTrue((self.project / "output/delivery/batch/keep.bin").is_file())
        self.assertFalse((self.project / "output/work").exists())

    def test_traversal_and_build_cannot_be_delivery_batch(self):
        for name in ("../build", "../../Source", "batch/other", str(self.sdk)):
            with self.subTest(name=name), self.assertRaises(ValueError):
                lifecycle.clean(self.project, delivery=name)
        self.assertTrue((self.sdk / "keep.bin").is_file())

    def test_changed_stage_rejected(self):
        (self.stage / "tool.exe").write_bytes(b"changed")
        with patch.object(lifecycle, "verify_sdk"), self.assertRaisesRegex(ValueError, "STALE_STAGE"):
            lifecycle.publish(self.project, self.stage, "layout", self.record)

    def test_changed_source_rejected(self):
        (self.project / "Source/main.py").write_bytes(b"changed")
        with patch.object(lifecycle, "verify_sdk"), self.assertRaisesRegex(ValueError, "STALE_SOURCE"):
            lifecycle.publish(self.project, self.stage, "layout", self.record)

    def test_changed_sdk_rejected(self):
        (self.sdk / "manifest/component-sdk.json").write_bytes(b"changed")
        with patch.object(lifecycle, "verify_sdk"), self.assertRaisesRegex(ValueError, "STALE_SDK"):
            lifecycle.publish(self.project, self.stage, "layout", self.record)

    def test_publish_and_clean_lock_conflict(self):
        with lifecycle.operation_lock(self.project), self.assertRaisesRegex(ValueError, "LOCKED"):
            lifecycle.clean(self.project)

    def test_success_survives_output_cleanup(self):
        with patch.object(lifecycle, "verify_sdk"):
            lifecycle.publish(self.project, self.stage, "layout", self.record)
        before = lifecycle.inventory(self.project / "build")
        lifecycle.clean(self.project)
        self.assertEqual(before, lifecycle.inventory(self.project / "build"))
        lifecycle.verify_tools(self.project / "build/tools/layout")

    def test_failed_promotion_restores_previous(self):
        stable = self.project / "build/tools/layout"
        stable.mkdir(parents=True)
        (stable / "old.bin").write_bytes(b"previous")
        original = Path.rename
        def rename(path, target):
            if path == self.stage:
                raise OSError("simulated publication interruption")
            return original(path, target)
        with patch.object(lifecycle, "verify_sdk"), patch.object(Path, "rename", rename), self.assertRaises(OSError):
            lifecycle.publish(self.project, self.stage, "layout", self.record)
        self.assertEqual((stable / "old.bin").read_bytes(), b"previous")

    def test_interrupted_journal_prevents_cleanup(self):
        journal = self.project / "build/.state/pending-layout.json"
        lifecycle.write_json(journal, {"schemaVersion": 1, "hadPrevious": True})
        with self.assertRaisesRegex(ValueError, "RECOVERY_REQUIRED"):
            lifecycle.clean(self.project)
        self.assertTrue(self.stage.exists())

    def test_recovery_restores_missing_destination_before_new_publication(self):
        previous = self.project / "build/.state/previous-layout"
        previous.mkdir(parents=True)
        (previous / "old.bin").write_bytes(b"last valid")
        lifecycle.write_json(self.project / "build/.state/pending-layout.json",
                             {"schemaVersion": 1, "hadPrevious": True})
        with patch.object(lifecycle, "verify_sdk"):
            lifecycle.publish(self.project, self.stage, "layout", self.record)
        self.assertEqual((previous / "old.bin").read_bytes(), b"last valid")
        lifecycle.verify_tools(self.project / "build/tools/layout")

    def test_real_sdk_corruption_and_configuration_rejected(self):
        published = ROOT / "DesignToCode_GenerateComponent/build/sdk"
        if not published.is_dir():
            self.skipTest("No published SDK; source-only checkout.")
        before = lifecycle.inventory(published)
        with self.assertRaisesRegex(ValueError, "ABI_MISMATCH"):
            SDKContract.verify(published, "Debug")
        copied = self.project / "output/work/sdk-corruption"
        shutil.copytree(published, copied)
        SDKContract.verify(copied)
        header = copied / "include/MyDesignSystem/MyDesignSystem.h"
        header.write_bytes(header.read_bytes() + b"\n// deliberate corruption probe\n")
        with self.assertRaisesRegex(ValueError, "INTEGRITY"):
            SDKContract.verify(copied)
        self.assertEqual(before, lifecycle.inventory(published))


if __name__ == "__main__":
    unittest.main()
