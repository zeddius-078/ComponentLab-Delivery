"""Generate one source-derived vector candidate, without certifying its SDK slot."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from acquire import new_output
from assets import ResourceBundle
from styled_layout import select_root
from vector_asset import compose_canvas


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("input", "node", "modules", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    for name in ("entry", "main", "modes", "instance", "node-id"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    for name in ("input", "node", "modules", "output"):
        if not getattr(args, name).is_absolute():
            raise ValueError("ABSOLUTE_PATH_REQUIRED")
    result = subprocess.run([str(args.node), str(ROOT / "Source/pixso_native/style_graph.cjs"),
        str(args.input), args.entry, str(args.modules), args.main, args.modes],
        capture_output=True, timeout=60, check=True)
    graph = json.loads(result.stdout)
    canvas = select_root(graph["styledTree"], args.instance, args.node_id)
    bundle = ResourceBundle(args.input, "VectorProbe")
    resource = compose_canvas(canvas, bundle)
    output = new_output(args.output)
    for name, data in bundle.files.items():
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    report = {"status": "candidate-only", "resource": resource,
              "evidence": bundle.evidence, "browserMeasured": False,
              "sdkSlotPlacement": "not-validated", "qtRendering": "not-run"}
    (output / "result.json").write_bytes((json.dumps(report, indent=2) + "\n").encode("utf-8"))
    print(json.dumps({"output": str(output), "resource": resource,
                      "scope": "source canvas composition only; SDK placement not validated"}))


if __name__ == "__main__":
    main()
