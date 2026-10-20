"""Prepare an isolated maintained-input copy without old build or history.

Includes current tracked working-tree bytes and new nonignored source files;
does not commit changes, checkout/reset files, or copy Git metadata.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
from BuildReceipt import ROOT, sha
sys.path.insert(0, str(ROOT / "Source/offline"))
from offline import inside, no_links


def prepare(name, output_root=None):
    if not name or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_" for c in name):
        raise ValueError("INVALID_CLEAN_COPY_NAME")
    parent = (Path(output_root).absolute() if output_root else
              ROOT / "DesignToCode_GenerateLayout/output/work")
    if parent.is_relative_to(ROOT) and not parent.is_relative_to(ROOT / "DesignToCode_GenerateLayout/output/work"):
        raise ValueError("CLEAN_COPY_CANNOT_WRITE_MAINTAINED_INPUTS")
    output = parent / name
    no_links(output)
    if output.exists():
        raise ValueError("CLEAN_COPY_ALREADY_EXISTS")
    files = subprocess.check_output(
        ["git", "-C", str(ROOT), "ls-files", "--cached", "--others", "--exclude-standard", "-z"]
    ).decode("utf-8").split("\0")
    records = {}
    for relative in sorted(set(files)):
        if not relative:
            continue
        parts = Path(relative).parts
        if any(part.lower() in ("build", "output", "archive", "__pycache__", ".git") for part in parts):
            continue
        source = ROOT / relative
        no_links(source)
        if not source.is_file():
            continue  # Deleted maintained input in the working tree.
        destination = inside(output, relative)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        if sha(source) != sha(destination):
            raise ValueError("CLEAN_COPY_BYTE_MISMATCH: " + relative)
        records[relative] = sha(source)
    report = output.parent / (name + "-inputs.json")
    report.write_bytes(json.dumps({"copy": str(output), "files": records,
                                  "oldBuildCopied": False, "historyCopied": False,
                                  "pendingWorkingTreeChangesIncluded": True}, indent=2).encode("utf-8"))
    print(output)


if __name__ == "__main__":
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--output-root", type=Path,
                        help="Explicit short validation root; avoids legacy MSVC MAX_PATH in nested copies")
    args = parser.parse_args()
    prepare(args.name, args.output_root)
