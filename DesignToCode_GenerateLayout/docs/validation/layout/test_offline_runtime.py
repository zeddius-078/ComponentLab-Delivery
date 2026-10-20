"""Python 3.9 regression: relocated tools, explicit work roots, no environment install."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest
from unittest.mock import patch


PROJECT = Path(__file__).resolve().parents[3]
FIXTURES = PROJECT / "docs/fixtures/layout"
COMPILER = Path(os.environ.get("D2C_COMPILER", str(PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe")))
NODE = os.environ.get("NODE_EXE", shutil.which("node") or "node")
SHELL = shutil.which("pwsh") or "powershell.exe"
SPEC = importlib.util.spec_from_file_location(
    "capture_output_paths", PROJECT / "Source/HtmlDsl/pipeline/capture/output_paths.py")
OUTPUT_PATHS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(OUTPUT_PATHS)


class OfflineRuntimeTests(unittest.TestCase):
    def setUp(self):
        base = PROJECT / "output/work/offline-runtime-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="case-", dir=base)
        self.addCleanup(self.cleanup_runtime)
        self.root = Path(self.temp.name)
        self.runtime = self.root / "runtime"
        self.work = self.root / "work"
        (self.runtime / "bin").mkdir(parents=True)
        self.work.mkdir()
        self.env = dict(os.environ, D2C_RUNTIME_ROOT=str(self.runtime),
                        D2C_WORK_ROOT=str(self.work), QT_QPA_PLATFORM="offscreen",
                        PYTHONDONTWRITEBYTECODE="1")
        self.env.pop("D2C_BROWSER_EXE", None)
        qt = Path(os.environ.get("QT_ROOT", "D:/QT/5.15.2/msvc2019_64"))
        self.env["PATH"] = str(qt / "bin") + os.pathsep + self.env.get("PATH", "")
        shutil.copy2(COMPILER, self.runtime / "bin/v2_ui_compiler.exe")
        shutil.copy2(qt / "bin/uic.exe", self.runtime / "bin/uic.exe")
        shutil.copytree(FIXTURES / "mapping", self.root / "mapping")
        self.raw = json.loads((FIXTURES / "design-inputs/review-page/canonical.raw.json").read_bytes())

    def cleanup_runtime(self):
        # Windows can briefly retain an executable's image after process exit.
        # Python 3.9's TemporaryDirectory may surface that denied unlink as
        # WinError 267 after attempting to treat the executable as a directory.
        # Retry only these Windows errors; persistent failures still fail the test.
        for attempt in range(6):
            try:
                self.temp.cleanup()
                return
            except OSError as error:
                if os.name != "nt" or error.winerror not in (5, 32, 267) or attempt == 5:
                    raise
                time.sleep(0.2 * (attempt + 1))

    def compiler(self, output, extra=(), env=None):
        raw = self.root / "input.json"
        raw.write_bytes(json.dumps(self.raw).encode("utf-8"))
        return subprocess.run([
            str(self.runtime / "bin/v2_ui_compiler.exe"), "--raw", str(raw),
            "--mapping", str(self.root / "mapping"),
            "--extensions", str(self.root / "mapping/extensions.json"),
            "--font", str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
            "--output", str(output), "--strict-layout", *extra,
        ], env=env or self.env, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)

    def node(self, expression, env=None):
        module = PROJECT / "Source/HtmlDsl/tools/output_paths.cjs"
        return subprocess.run([NODE, "-e", "const guard=require(" + json.dumps(str(module)) + ");" + expression],
                              env=env or self.env, capture_output=True, text=True, encoding="utf-8", timeout=15)

    def compact_fixture(self):
        path = self.root / "mapping/component-mapping.json"
        mapping = json.loads(path.read_bytes())
        mapping["profile"] = "wide-light"
        path.write_bytes(json.dumps(mapping).encode("utf-8"))
        path = self.root / "mapping/extensions.json"
        extensions = json.loads(path.read_bytes())
        extensions["selectOption"]["properties"]["Profile"] = "wide-light"
        path.write_bytes(json.dumps(extensions).encode("utf-8"))
        for modes in self.raw.get("variableSetMap", {}).values():
            for mode in modes:
                if mode.get("name") == "light-compact":
                    mode["name"] = "wide-light"
        def visit(node):
            for prop in node.get("propRefMap", {}).get("variant", []):
                if prop.get("name") == "Profile":
                    prop["value"] = "wide-light"
            for child in node.get("childNode", []):
                visit(child)
        for root in self.raw["pixTreeNodes"]:
            visit(root)

    def test_relocated_compiler_preview_has_no_source_tree(self):
        output = self.work / "output/work/preview/page"
        result = self.compiler(output)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(output.with_suffix(".ui").is_file())
        self.assertFalse((self.runtime / "Source").exists())
        self.assertEqual(sorted(p.name for p in self.runtime.iterdir()), ["bin"])

    def test_explicit_cli_roots_override_environment(self):
        env = dict(self.env, D2C_RUNTIME_ROOT="invalid", D2C_WORK_ROOT="invalid")
        output = self.work / "output/work/preview/page"
        result = self.compiler(output, ["--tool-root", str(self.runtime), "--work-root", str(self.work)], env)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_relocated_formal_uses_bundled_uic_and_separate_reports(self):
        self.compact_fixture()
        output = self.work / "output/delivery/batch/page"
        result = self.compiler(output, ["--formal"])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((output / "form/ReviewPage.ui").is_file())
        self.assertTrue((self.work / "output/logs/formal-delivery/batch/page").is_dir())
        self.assertEqual(sorted(p.name for p in self.runtime.iterdir()), ["bin"])

    def test_offline_does_not_fallback_to_installed_uic(self):
        self.compact_fixture()
        (self.runtime / "bin/uic.exe").unlink()
        output = self.work / "output/delivery/batch/page"
        result = self.compiler(output, ["--formal"])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("UIC_EXECUTABLE_MISSING", result.stderr)
        self.assertFalse(output.exists())

    def test_compiler_rejects_escape_and_wrong_output_lane(self):
        for output in (self.root / "escaped", self.work / "Source/page",
                       self.work / "output/logs/page", self.runtime / "output/work/preview/page"):
            with self.subTest(output=output):
                result = self.compiler(output)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.with_suffix(".ui").exists())

    def test_compiler_requires_complete_absolute_roots(self):
        for env, expected in (
            (dict(self.env, D2C_WORK_ROOT=""), "OFFLINE_ROOTS_REQUIRED"),
            (dict(self.env, D2C_WORK_ROOT="relative"), "OFFLINE_ROOT_INVALID"),
            (dict(self.env, D2C_WORK_ROOT=str(self.runtime)), "WORK_ROOT_INSIDE_RUNTIME"),
        ):
            with self.subTest(expected=expected):
                result = self.compiler(self.work / "output/work/preview/page", env=env)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(expected, result.stderr)

    def test_python_and_node_guards_only_allow_work_build_lanes(self):
        with patch.dict(os.environ, self.env, clear=True):
            for lane in ("work/cache", "work/captures", "work/preview", "delivery", "logs"):
                output = self.work / "output" / lane / "file.json"
                self.assertEqual(OUTPUT_PATHS.output_path(output), output.resolve())
                result = self.node("console.log(guard.outputPath(" + json.dumps(str(output)) + "));")
                self.assertEqual(result.returncode, 0, result.stderr)
            for output in (self.root / "outside", self.work / "Source/file", self.work / "build/arbitrary/file",
                           self.runtime / "bin/file"):
                with self.subTest(output=output):
                    with self.assertRaisesRegex(ValueError, "OUTPUT_MUST_BE_IN_WORK_BUILD"):
                        OUTPUT_PATHS.output_path(output)
                    result = self.node("guard.outputPath(" + json.dumps(str(output)) + ");")
                    self.assertNotEqual(result.returncode, 0)

    def test_offline_browser_is_explicit_and_never_channel_fallback(self):
        result = self.node("guard.browserLaunchOptions();")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OFFLINE_BROWSER_EXECUTABLE_REQUIRED", result.stderr)
        env = dict(self.env, D2C_BROWSER_EXE=str(self.runtime / "missing.exe"))
        result = self.node("guard.browserLaunchOptions();", env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("BROWSER_EXECUTABLE_MISSING", result.stderr)
        env["D2C_BROWSER_EXE"] = str(self.runtime / "bin/uic.exe")
        result = self.node("console.log(JSON.stringify(guard.browserLaunchOptions()));", env)
        self.assertEqual(result.returncode, 0, result.stderr)
        options = json.loads(result.stdout)
        self.assertEqual(Path(options["executablePath"]), Path(env["D2C_BROWSER_EXE"]))
        self.assertNotIn("channel", options)
        env.pop("D2C_BROWSER_EXE")
        env.pop("D2C_RUNTIME_ROOT")
        env.pop("D2C_WORK_ROOT")
        result = self.node("console.log(JSON.stringify(guard.browserLaunchOptions()));", env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OFFLINE_BROWSER_EXECUTABLE_REQUIRED", result.stderr)

    @unittest.skipUnless(os.name == "nt", "NTFS junction test")
    def test_junction_outputs_are_rejected_by_all_guards(self):
        target = self.root / "outside"
        target.mkdir()
        link = self.work / "output/work/preview"
        link.parent.mkdir(parents=True)
        result = subprocess.run([SHELL, "-NoProfile", "-NonInteractive", "-Command",
            "New-Item -ItemType Junction -Path $env:TEST_LINK -Target $env:TEST_TARGET | Out-Null"],
            env=dict(self.env, TEST_LINK=str(link), TEST_TARGET=str(target)), capture_output=True, timeout=15)
        if result.returncode and b"New-Item" in result.stderr:
            self.skipTest("Work filesystem does not support junction creation (e.g. exFAT)")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.addCleanup(lambda: os.rmdir(str(link)))
        output = link / "page"
        with patch.dict(os.environ, self.env, clear=True):
            with self.assertRaisesRegex(ValueError, "OUTPUT_PATH_IS_LINK"):
                OUTPUT_PATHS.output_path(output)
        result = self.node("guard.outputPath(" + json.dumps(str(output)) + ");")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OUTPUT_PATH_IS_LINK", result.stderr)
        result = self.compiler(output)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OUTPUT_PATH_IS_LINK", result.stderr)
        self.assertEqual(list(target.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
