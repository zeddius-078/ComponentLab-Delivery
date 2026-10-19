"""Regression checks for the maintained-input/disposable-output boundary.

Run from the project root with: python -B -m unittest discover -s docs/validation
"""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest

PROJECT = Path(__file__).resolve().parents[2]


def load_module(name, relative):
    spec = importlib.util.spec_from_file_location(name, PROJECT / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def snapshot():
    return {path.relative_to(PROJECT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for folder in ("Source", "docs")
            for path in (PROJECT / folder).rglob("*") if path.is_file()}


class SourceBoundaryTests(unittest.TestCase):
    def test_output_paths(self):
        prepare = load_module("prepare_boundary", "Source/PrepareBuild.py")
        prepare.safe_output(PROJECT / "build/work")
        for relative in ("Source/generated", "docs/generated", "build/../Source/generated"):
            with self.assertRaises(ValueError):
                prepare.safe_output(PROJECT / relative)

    def test_generator_guard(self):
        guard = load_module("output_guard_boundary", "Source/ComponentKit/tools/output_guard.py")
        guard.require_build_workspace(PROJECT / "build/work/Source/ComponentKit")
        with self.assertRaises(RuntimeError):
            guard.require_build_workspace(PROJECT / "Source/ComponentKit")

    def test_source_has_no_build_artifacts(self):
        forbidden = {".pyc", ".obj", ".exe", ".dll", ".lib", ".pdb"}
        for path in (PROJECT / "Source").rglob("*"):
            self.assertNotIn(path.name, {"generated", "__pycache__"}, str(path))
            self.assertNotIn(path.suffix, forbidden, str(path))

    def test_preparation_preserves_inputs(self):
        before = snapshot()
        result = subprocess.run([sys.executable, "-B", str(PROJECT / "Source/PrepareBuild.py")],
                                capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(before, snapshot(), "Build preparation changed Source or docs")


if __name__ == "__main__":
    unittest.main()
