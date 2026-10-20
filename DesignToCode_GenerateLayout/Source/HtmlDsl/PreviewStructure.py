"""Python 3.9 entry for bounded structure + initial-visual preview.

Uses the existing compiler; does not install tools, infer component mappings,
compile business code or publish a formal delivery.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys


def verify_structure(source, report):
    """Check every captured node, including hidden descendants, not just counts."""
    actual = {node["id"]: node for node in report["nodes"]}
    expected = set()

    def rounded(value):
        # Qt 5.15 qRound(double): half ties toward positive infinity,
        # including -34.5 -> -34. Do not use Python's bankers rounding.
        return math.floor(value + 0.5)

    def walk(node, parent_id="", ancestor_visible=True):
        identity = node["id"]
        expected.add(identity)
        record = actual.get(identity)
        visible = ancestor_visible and node.get("visible", True)
        geometry = [rounded(node["x"]) if parent_id else 0,
                    rounded(node["y"]) if parent_id else 0,
                    rounded(node["width"]), rounded(node["height"])]
        fields = {"parentId": parent_id, "sourceName": node.get("name"),
                  "sourceType": node["type"], "visible": visible, "geometry": geometry,
                  "mainComponent": node.get("mainComponent"), "text": node.get("characters")}
        if record is None or any(record.get(key) != value for key, value in fields.items()):
            raise RuntimeError("STRUCTURE_READBACK_MISMATCH: " + identity)
        for child in node.get("children", []):
            walk(child, identity, visible)

    walk(source["root"])
    if expected != set(actual) or len(actual) != len(report["nodes"]):
        raise RuntimeError("STRUCTURE_NODE_SET_MISMATCH")
    return len(expected)


def main():
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON_39_REQUIRED")
    project = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--style", type=Path, required=True)
    parser.add_argument("--font", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True, help="New prefix inside work-root/output/work")
    parser.add_argument("--tool-root", type=Path, default=Path(os.environ.get("D2C_RUNTIME_ROOT", project / "build/tools/layout")))
    parser.add_argument("--work-root", type=Path, default=Path(os.environ.get("D2C_WORK_ROOT", project)))
    parser.add_argument("--qt-root", type=Path, help="Explicit development Qt 5.15 installation; normally use the published runtime")
    parser.add_argument("--compiler", type=Path)
    args = parser.parse_args()
    args.compiler = args.compiler or args.tool_root / "bin/v2_ui_compiler.exe"
    # Lexical and resolved checks; native compiler additionally rejects junctions.
    output = args.output.absolute()
    for candidate in (output, output.resolve()):
        try:
            candidate.relative_to(args.work_root.resolve() / "output/work")
        except ValueError:
            raise SystemExit("OUTPUT_MUST_BE_IN_BUILD")
    verification = Path(str(output) + ".validation.json")
    if verification.exists():
        raise SystemExit("STRUCTURE_OUTPUT_ALREADY_EXISTS")
    qt_bin = args.qt_root / "bin" if args.qt_root else args.tool_root / "bin"
    qt_plugins = args.qt_root / "plugins" if args.qt_root else qt_bin
    uic = qt_bin / "uic.exe"
    for path in (args.input, args.style, args.font, args.compiler, uic,
                 qt_plugins / "platforms/qoffscreen.dll"):
        if not path.is_file():
            raise SystemExit("MISSING_EXISTING_INPUT_OR_TOOL: " + str(path))
    env = dict(os.environ)
    env["PATH"] = str(qt_bin) + os.pathsep + env.get("PATH", "")
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["QT_PLUGIN_PATH"] = str(qt_plugins)
    env["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(qt_plugins / "platforms")
    env["D2C_RUNTIME_ROOT"] = str(args.tool_root.resolve())
    env["D2C_WORK_ROOT"] = str(args.work_root.resolve())
    subprocess.run([str(args.compiler.resolve()), "--structure-preview",
                    "--raw", str(args.input.resolve()), "--initial-style", str(args.style.resolve()),
                    "--font", str(args.font.resolve()), "--output", str(output)], env=env, check=True)
    captured_bytes = args.input.read_bytes()
    report = json.loads(Path(str(output) + ".structure.json").read_bytes())
    if report["inputSha256"] != hashlib.sha256(captured_bytes).hexdigest():
        raise RuntimeError("STRUCTURE_INPUT_CHANGED")
    count = verify_structure(json.loads(captured_bytes), report)
    # uic stdout is the generated header. Check without another generated source copy.
    subprocess.run([str(uic.resolve()), str(output) + ".ui"],
                   env=env, check=True, stdout=subprocess.PIPE)
    verification.write_bytes((json.dumps({
        "status": "PASS_UI_SYNTAX_ONLY", "python": sys.version,
        "structureReadback": "PASS", "verifiedNodeCount": count,
        "compiler": str(args.compiler.resolve()), "uic": str(uic.resolve()),
        "outputSha256": {suffix: hashlib.sha256(Path(str(output) + suffix).read_bytes()).hexdigest()
                         for suffix in (".ui", ".qss", ".png", ".structure.json")},
        "formal": False, "visualParity": "NOT_CLAIMED",
        "componentContract": "NOT_RUN", "interaction": "NOT_IMPLEMENTED"
    }, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    print(str(output) + ".ui")


if __name__ == "__main__":
    main()
