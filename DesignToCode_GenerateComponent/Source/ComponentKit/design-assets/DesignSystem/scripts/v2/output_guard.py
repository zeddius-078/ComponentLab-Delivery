"""Shared guard for the Token compiler's file-writing entry points."""
from pathlib import Path
import importlib.util


def require_build_workspace(root):
    helper = Path(__file__).resolve().parents[4] / "tools/output_guard.py"
    spec = importlib.util.spec_from_file_location("component_output_guard", helper)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.require_build_workspace(root)
