"""Negative tests for SDK consumption; synthetic packages are test fixtures only."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[3]
SHELL = shutil.which("pwsh") or "powershell.exe"


class SdkBuildGateTests(unittest.TestCase):
    def setUp(self):
        base = PROJECT / "output/work/sdk-build-gate"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="case-", dir=base)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.sdk = self.root / "external-sdk"
        self.manifest = {"schemaVersion": 2, "package": "GeneratedComponentSDK", "packageVersion": "1.0.0.1", "releaseId": "test",
                         "owner": "DesignToCode_GenerateComponent",
                         "componentAuthority": True, "tokenAuthority": True, "linkage": "static", "files": [],
                         "toolchain": {"configuration": "Release", "architecture": "x64", "crt": "MD",
                                       "qtVersion": "5.15.2", "pythonVersion": "3.9.13",
                                       "cmakeVersion": "fixture", "compilerVersion": "fixture", "compilerId": "MSVC"}}
        self.manifest["sourceFiles"] = [{"path": "Source/fixture.cpp", "sha256": "0" * 64}]
        self.manifest["sourceFingerprint"] = hashlib.sha256(json.dumps(self.manifest["sourceFiles"], sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()
        for key in ("componentMap", "nativeCapabilityCatalog", "componentSourceMap", "designComponentCatalog", "v2ComponentMap", "v2ComponentRegistry", "tokenManifest"):
            self.manifest[key] = "resources/" + key + ".json"
        for name in ("lib/GeneratedComponentSDK.lib", "cmake/GeneratedComponentSDKConfig.cmake",
                     "cmake/GeneratedComponentSDKConfigVersion.cmake", "licenses/NOTICE.txt",
                     "include/MyDesignSystem/MyDesignSystem.h", *[self.manifest[key] for key in ("componentMap", "nativeCapabilityCatalog", "componentSourceMap", "designComponentCatalog", "v2ComponentMap", "v2ComponentRegistry", "tokenManifest")]):
            target = self.sdk / name
            target.parent.mkdir(parents=True, exist_ok=True)
            data = b"negative-test-fixture\n"
            target.write_bytes(data)
            self.manifest["files"].append({"path": name, "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)})
        (self.sdk / "manifest").mkdir()

    def verify(self, build=False):
        (self.sdk / "manifest/component-sdk.json").write_bytes(json.dumps(self.manifest).encode("utf-8"))
        script = PROJECT / "Source" / ("Build.ps1" if build else "Verify-GeneratedComponentSDK.ps1")
        # Deliberately omit Qt; invalid SDK must fail first, without building or publishing anything.
        env = dict(os.environ, GENERATED_COMPONENT_SDK_ROOT=str(self.sdk))
        env.pop("QT_ROOT", None)
        return subprocess.run([SHELL, "-NoProfile", "-NonInteractive", "-File", str(script)],
                              env=env, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)

    def test_build_rejects_tampered_external_sdk_before_environment_probe(self):
        target = self.sdk / "lib/GeneratedComponentSDK.lib"
        target.write_bytes(b"changed")
        result = self.verify(build=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SDK_FILE_INTEGRITY_MISMATCH", result.stderr)
        self.assertNotIn("Set QT_ROOT", result.stderr)
        self.assertEqual(target.read_bytes(), b"changed")

    def test_manifest_cannot_omit_or_escape_inventory(self):
        for change, expected in (("empty", "SDK_FILE_INTEGRITY_MISMATCH"), ("traversal", "SDK_FILE_INTEGRITY_MISMATCH"),
                                 ("duplicate", "SDK_DUPLICATE_FILE"), ("unlisted", "SDK_FILE_INTEGRITY_MISMATCH")):
            with self.subTest(change=change):
                saved = list(self.manifest["files"])
                if change == "empty":
                    self.manifest["files"] = []
                elif change == "traversal":
                    self.manifest["files"] = [{"path": "../outside", "sha256": "0" * 64, "bytes": 1}]
                elif change == "duplicate":
                    self.manifest["files"].append(dict(self.manifest["files"][0]))
                else:
                    (self.sdk / "unexpected.txt").write_bytes(b"unlisted")
                result = self.verify()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(expected, result.stderr)
                self.manifest["files"] = saved

    def test_old_schema_and_wrong_abi_are_rejected(self):
        self.manifest["schemaVersion"] = 1
        self.assertIn("SDK_SCHEMA_UNSUPPORTED", self.verify().stderr)
        self.manifest["schemaVersion"] = 2
        self.manifest["toolchain"]["crt"] = "MDd"
        self.assertIn("SDK_ABI_MISMATCH", self.verify().stderr)

    def test_explicit_package_precedes_environment(self):
        (self.sdk / "manifest/component-sdk.json").write_bytes(json.dumps(self.manifest).encode("utf-8"))
        result = subprocess.run([
            SHELL, "-NoProfile", "-NonInteractive", "-File",
            str(PROJECT / "Source/Verify-GeneratedComponentSDK.ps1"), "-Package", str(self.sdk)],
            env=dict(os.environ, GENERATED_COMPONENT_SDK_ROOT=str(self.root / "nonexistent")),
            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_cmake_refuses_conflicting_cached_sdk(self):
        cmake = os.environ.get("CMAKE_EXE") or shutil.which("cmake") or "D:/QT/Tools/CMake_64/bin/cmake.exe"
        script = self.root / "conflict.cmake"
        dependency = (PROJECT / "Source/GeneratedComponentDependency.cmake").as_posix()
        for cached in ("D2C_CONFIGURED_SDK_ROOT", "GeneratedComponentSDK_DIR"):
            script.write_bytes((
                'set(GENERATED_COMPONENT_SDK_ROOT "' + self.sdk.as_posix() + '")\n'
                'set(' + cached + ' "' + (self.root / "other-sdk").as_posix() + '")\n'
                'include("' + dependency + '")\n').encode("utf-8"))
            result = subprocess.run([cmake, "-P", str(script)], capture_output=True, text=True, timeout=15)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("SDK_CACHE_CONFLICT", result.stderr)


if __name__ == "__main__":
    unittest.main()
