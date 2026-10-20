"""Create the disposable generation workspace; Source and docs are read-only inputs.

The workspace preserves relative C++ includes and QRC paths. It is not a second
source authority: edits belong in Source, and every preparation refreshes inputs.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import stat
import subprocess
import sys

PROJECT = Path(__file__).resolve().parent.parent
WORK = PROJECT / "output/work/generated"
KIT = WORK / "Source/ComponentKit"


def safe_output(value):
    path = Path(value).absolute()
    if not path.resolve().is_relative_to(PROJECT / "output/work"):
        raise ValueError("OUTPUT_MUST_BE_IN_WORK")
    for parent in (path, *path.parents):
        if parent.exists() and (parent.is_symlink() or
                getattr(parent.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
            raise ValueError("OUTPUT_PATH_IS_LINK")
    return path


def prepare(generate=True):
    safe_output(WORK)
    inputs = list((PROJECT / "Source/ComponentKit").rglob("*"))
    inputs += [PROJECT / "docs/designRules/design-system/component-traceability-manifest.json"]
    inventory = {}
    for source in sorted(inputs):
        if not source.is_file() or "__pycache__" in source.parts or source.suffix == ".pyc":
            continue
        if source.is_symlink():
            raise ValueError(f"SOURCE_LINK_NOT_ALLOWED: {source}")
        relative = source.relative_to(PROJECT)
        target = safe_output(WORK / relative)
        data = source.read_bytes()
        inventory[relative.as_posix()] = hashlib.sha256(data).hexdigest()
        if not target.is_file() or target.read_bytes() != data:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
    manifest = WORK / "input-manifest.json"
    if manifest.exists():
        previous = json.loads(manifest.read_text(encoding="utf-8"))["files"]
        for relative in previous.keys() - inventory.keys():
            stale = safe_output(WORK / relative)
            if stale.is_file():
                stale.unlink()  # Only files recorded as old maintained-input copies.
    manifest.write_text(json.dumps({"authority": "Source and docs", "files": inventory},
                                  ensure_ascii=False, indent=2), encoding="utf-8")
    if generate:
        for relative in ("design-assets/tools/export_tokens.py", "tools/generate_native_components.py",
                         "tools/generate_design_component_catalog.py"):
            subprocess.run([sys.executable, "-B", str(KIT / relative)], check=True)
    return KIT


if __name__ == "__main__":
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("Python 3.9 required.")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--copy-only", action="store_true")
    args = parser.parse_args()
    print(prepare(not args.copy_only))
