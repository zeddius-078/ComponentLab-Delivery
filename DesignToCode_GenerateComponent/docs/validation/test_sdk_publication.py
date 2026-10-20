"""Pure Python 3.9 SDK publication failure/clean-checkout regression tests."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT / "Source"))
import SDKPackage as sdk
import CheckProject


class SDKPublicationTests(unittest.TestCase):
    def setUp(self):
        root = PROJECT / "output/work/cache/sdk-publication-tests"
        root.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(root))
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name)
        for name in (
            "Source/input.cpp",
            "docs/designRules/design-system/component-traceability-manifest.json",
            "docs/examples/MappingWorkbench/mapping/qt-component-map.json",
            "docs/examples/MappingWorkbench/mapping/qt-native-catalog.json",
            "docs/validation/SDKConsumerTest/CMakeLists.txt",
            "docs/validation/SDKConsumerTest/main.cpp",
            "docs/validation/SDKConsumerTest/Build.ps1",
        ):
            path = self.project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"{}")
        self.stage = self.project / "output/work/cache/stage"
        self.authority = self.project / "build/sdk"

    def package(self, path, marker=b"test-only-package"):
        names = ["lib/GeneratedComponentSDK.lib", "include/MyDesignSystem/MyDesignSystem.h",
                 "cmake/GeneratedComponentSDKConfig.cmake", "cmake/GeneratedComponentSDKConfigVersion.cmake",
                 "resources/map.json", "licenses/LICENSE.txt", "README.md"]
        for name in names:
            target = path / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(marker)
        manifest = {
            "schemaVersion": 2, "package": "GeneratedComponentSDK",
            "owner": "DesignToCode_GenerateComponent",
            "linkage": "static",
            "componentAuthority": True, "tokenAuthority": True,
            "toolchain": {"configuration": "Release", "architecture": "x64", "crt": "MD",
                          "qtVersion": "5.15.2", "pythonVersion": "3.9.13",
                          "cmakeVersion": "test", "compilerVersion": "test", "compilerId": "MSVC"},
        }
        for key in ("componentMap", "nativeCapabilityCatalog", "componentSourceMap",
                    "designComponentCatalog", "v2ComponentMap", "v2ComponentRegistry", "tokenManifest"):
            manifest[key] = "resources/map.json"
        manifest.update(sdk.snapshot(self.project))
        manifest["files"] = sdk.package_files(path)
        (path / "manifest").mkdir(exist_ok=True)
        sdk.write_json(path / sdk.MANIFEST, manifest)
        return manifest

    def test_clean_checkout_does_not_require_sdk(self):
        # The actual source gate must not consult either generated or SDK paths.
        with patch.object(CheckProject, "WORK_SOURCE", self.project / "not-created"):
            result = CheckProject.check_source()
        self.assertEqual(result["status"], "PASS")
        self.assertFalse(result["sdkRequired"])

    def test_snapshot_excludes_disposable_and_history(self):
        before = sdk.snapshot(self.project)["sourceFingerprint"]
        for name in ("build/generated.json", "docs/archive/old-report.json"):
            path = self.project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"arbitrary history")
        self.assertEqual(sdk.snapshot(self.project)["sourceFingerprint"], before)
        (self.project / "Source/input.cpp").write_bytes(b"changed")
        self.assertNotEqual(sdk.snapshot(self.project)["sourceFingerprint"], before)

    def test_full_promotion_preserves_previous_package(self):
        self.package(self.authority, b"old")
        new = self.package(self.stage, b"new")
        result = sdk.promote(self.stage, self.authority, self.project)
        self.assertEqual(sdk.verify(self.authority)["releaseId"], new["releaseId"])
        self.assertEqual((Path(result["previousSdk"]) / "lib/GeneratedComponentSDK.lib").read_bytes(), b"old")

    def test_repeated_publication_keeps_one_controlled_previous(self):
        self.package(self.authority, b"first")
        self.package(self.stage, b"second")
        sdk.promote(self.stage, self.authority, self.project)
        self.package(self.stage, b"third")
        result = sdk.promote(self.stage, self.authority, self.project)
        previous = self.project / "build/.state/previous-sdk"
        self.assertEqual(result["previousSdk"], str(previous))
        self.assertEqual((previous / "lib/GeneratedComponentSDK.lib").read_bytes(), b"second")
        self.assertEqual(len(list((self.project / "build/.state").glob("previous-sdk*"))), 1)

    def test_missing_support_override_does_not_fall_back(self):
        self.package(self.stage)
        with patch.dict("os.environ", {"D2C_SUPPORT_ROOT": str(self.project / "missing-support")}):
            with self.assertRaisesRegex(ValueError, "SDK_SUPPORT_MISSING"):
                sdk.verify(self.stage)

    def test_invalid_stage_does_not_touch_existing_package(self):
        self.package(self.authority, b"old")
        self.package(self.stage)
        (self.stage / "lib/GeneratedComponentSDK.lib").write_bytes(b"corrupted")
        with self.assertRaisesRegex(ValueError, "INTEGRITY"):
            sdk.promote(self.stage, self.authority, self.project)
        self.assertEqual((self.authority / "lib/GeneratedComponentSDK.lib").read_bytes(), b"old")

    def test_failed_rename_restores_previous_package(self):
        previous = self.package(self.authority, b"old")
        self.package(self.stage)
        def fail_second(source, target):
            if source == self.stage:
                raise PermissionError("injected cutover failure")
            source.rename(target)
        with self.assertRaises(PermissionError):
            sdk.promote(self.stage, self.authority, self.project, rename=fail_second)
        self.assertEqual(sdk.verify(self.authority)["releaseId"], previous["releaseId"])

    def test_post_cutover_verification_failure_rolls_back(self):
        previous = self.package(self.authority, b"old")
        new = self.package(self.stage)
        original_verify = sdk.verify
        def fail_final(path, *args, **kwargs):
            if path == self.authority and sdk.read_json(path / sdk.MANIFEST)["releaseId"] == new["releaseId"]:
                raise ValueError("injected final verification failure")
            return original_verify(path, *args, **kwargs)
        with patch.object(sdk, "verify", side_effect=fail_final):
            with self.assertRaises(ValueError):
                sdk.promote(self.stage, self.authority, self.project)
        self.assertEqual(sdk.verify(self.authority)["releaseId"], previous["releaseId"])

    def test_stale_source_blocks_promotion(self):
        self.package(self.stage)
        (self.project / "Source/input.cpp").write_bytes(b"new source")
        with self.assertRaisesRegex(ValueError, "STALE_SOURCE"):
            sdk.promote(self.stage, self.authority, self.project)
        self.assertFalse(self.authority.exists())

    def test_configuration_and_file_inventory_are_verified(self):
        self.package(self.stage)
        with self.assertRaisesRegex(ValueError, "ABI_MISMATCH"):
            sdk.verify(self.stage, "Debug")
        (self.stage / "extra.txt").write_bytes(b"unlisted")
        with self.assertRaisesRegex(ValueError, "INTEGRITY"):
            sdk.verify(self.stage)

    def test_external_destination_rejected(self):
        self.package(self.stage)
        with self.assertRaisesRegex(ValueError, "MUST_BE_IN_CONTROLLED_BUILD"):
            sdk.promote(self.stage, self.project / "external-sdk", self.project)

    def stamp_inputs(self):
        self.package(self.stage)
        version = self.stage / "cmake/GeneratedComponentSDKConfigVersion.cmake"
        version.write_bytes(b'set(PACKAGE_VERSION "1.0.0")\n')
        baseline = self.project / "output/work/snapshot.json"
        sdk.write_json(baseline, sdk.snapshot(self.project))
        build = self.project / "output/work/compiler"
        compiler = build / "CMakeFiles/3.30.5/CMakeCXXCompiler.cmake"
        compiler.parent.mkdir(parents=True)
        compiler.write_bytes(b'set(CMAKE_CXX_COMPILER_ID "MSVC")\n'
                             b'set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID x64)\n'
                             b'set(CMAKE_CXX_COMPILER_VERSION "19.51.0")\n')
        qt = self.project / "test-qt"
        cache = ("CMAKE_BUILD_TYPE:STRING=Release\n"
                 "CMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreadedDLL\n"
                 "Qt5Core_DIR:PATH=" + (qt / "lib/cmake/Qt5Core").as_posix() + "\n")
        (build / "CMakeCache.txt").write_bytes(cache.encode("utf-8"))
        return baseline, qt, build

    def test_stamp_records_measured_toolchain_and_dynamic_version(self):
        baseline, qt, build = self.stamp_inputs()
        with patch.object(sdk, "PROJECT", self.project), \
                patch.object(sdk.subprocess, "check_output", side_effect=[b"5.15.2\n", b"cmake version 3.30.5\n"]):
            result = sdk.stamp(self.stage, baseline, qt, "cmake", build)
        self.assertEqual(result["toolchain"]["compilerVersion"], "19.51.0")
        self.assertEqual(result["toolchain"]["crt"], "MD")
        self.assertNotEqual(result["packageVersion"], "1.0.0")
        self.assertIn(result["packageVersion"], (self.stage / "cmake/GeneratedComponentSDKConfigVersion.cmake").read_text())

    def test_stamp_rejects_misreported_configuration(self):
        baseline, qt, build = self.stamp_inputs()
        cache = build / "CMakeCache.txt"
        cache.write_bytes(cache.read_bytes().replace(b"MultiThreadedDLL", b"MultiThreadedDebugDLL"))
        with patch.object(sdk, "PROJECT", self.project), \
                patch.object(sdk.subprocess, "check_output", side_effect=[b"5.15.2\n", b"cmake version 3.30.5\n"]):
            with self.assertRaisesRegex(ValueError, "CONFIGURATION_MISMATCH"):
                sdk.stamp(self.stage, baseline, qt, "cmake", build)

    @unittest.skipUnless(sys.platform == "win32", "Windows publisher uses a native file lock")
    def test_windows_publisher_lock_is_exclusive(self):
        lock = str(self.project / "lock").replace("'", "''")
        command = (
            "$p='" + lock + "';"
            "$a=[IO.File]::Open($p,'OpenOrCreate','ReadWrite','None');"
            "try { try {$b=[IO.File]::Open($p,'OpenOrCreate','ReadWrite','None');"
            "$b.Dispose();exit 2} catch {exit 0} } finally {$a.Dispose()}"
        )
        result = subprocess.run(["powershell", "-NoProfile", "-Command", command])
        self.assertEqual(result.returncode, 0)


if __name__ == "__main__":
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("Python 3.9 required.")
    unittest.main()
