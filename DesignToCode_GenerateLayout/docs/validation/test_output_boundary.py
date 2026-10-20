"""Regression checks: commands must reject maintained-tree output before writing."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import unittest

PROJECT = Path(__file__).resolve().parents[2]
CAPTURE = PROJECT / "Source/HtmlDsl/pipeline/capture"
spec = importlib.util.spec_from_file_location("output_paths", CAPTURE / "output_paths.py")
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)


class OutputBoundaryTests(unittest.TestCase):
    def test_python_rejects_maintained_paths(self):
        for target in (PROJECT / "Source/probe", PROJECT / "docs/probe", PROJECT / "build/../Source/probe"):
            with self.assertRaisesRegex(ValueError, "OUTPUT_MUST_BE_IN_WORK_BUILD"):
                policy.output_path(target)
        self.assertEqual(policy.output_path(PROJECT / "output/work/cache/probe"), PROJECT / "output/work/cache/probe")

    def test_capture_rejects_before_network(self):
        result = subprocess.run([sys.executable, "-B", str(CAPTURE / "capture_mcp.py"),
            "--endpoint", "http://127.0.0.1:1/mcp", "--tool", "eval_script",
            "--arguments", "{}", "--output", str(PROJECT / "Source/probe")], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("OUTPUT_MUST_BE_IN_WORK_BUILD", result.stderr)
        self.assertFalse((PROJECT / "Source/probe").exists())

    def test_node_rejects_maintained_paths(self):
        script = "const p=require(process.argv[1]);for(const v of process.argv.slice(2)){try{p.outputPath(v);process.exit(2)}catch(e){if(e.message!=='OUTPUT_MUST_BE_IN_WORK_BUILD')throw e}}"
        result = subprocess.run([os.environ.get("NODE_EXE", "node"), "-e", script,
            str(PROJECT / "Source/HtmlDsl/tools/output_paths.cjs"),
            str(PROJECT / "Source/probe"), str(PROJECT / "docs/probe"),
            str(PROJECT / "build/../docs/probe")], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_compiler_rejects_maintained_paths(self):
        compiler = PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe"
        if not compiler.exists():
            self.skipTest("Build Source/Build.ps1 first")
        environment = dict(os.environ, QT_QPA_PLATFORM="offscreen", D2C_RUNTIME_ROOT=str(PROJECT / "build/tools/layout"), D2C_WORK_ROOT=str(PROJECT))
        for folder in ("Source", "docs"):
            result = subprocess.run([str(compiler), "--raw", "missing", "--mapping", "missing",
                "--extensions", "missing", "--font", "missing", "--output", str(PROJECT / folder / "probe")],
                capture_output=True, text=True, env=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("OFFLINE_PREVIEW_OUTSIDE_WORK_BUILD", result.stderr)


if __name__ == "__main__":
    unittest.main()
