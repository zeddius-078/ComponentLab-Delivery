"""Create a delivery inventory for the display-only ComponentLab package."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from CheckProject import PROJECT, validation_inputs

MANIFEST = PROJECT / "build/reports/DeliveryManifest.json"
RECORD = PROJECT / "build/reports/Validation.json"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def inventory():
    paths = [PROJECT / "README.md"]
    for name in ("Source", "docs", "build/demo/component_gallery"):
        paths.extend(p for p in (PROJECT / name).rglob("*") if p.is_file()
                     and "__pycache__" not in p.parts)
    return [{"path": p.relative_to(PROJECT).as_posix(), "bytes": p.stat().st_size,
             "sha256": digest(p)} for p in sorted(set(paths))]


def refresh(results):
    inputs = validation_inputs()
    proof = json.loads((results / "build-verification.json").read_text(encoding="utf-8-sig"))
    if proof["status"] != "passed" or proof["validationInputs"] != inputs:
        raise ValueError("Build evidence does not match current sources and authoritative SDK")
    exe_hash = digest(PROJECT / "build/demo/component_gallery/component_lab.exe")
    if proof["exeSha256"].upper() != exe_hash:
        raise ValueError("Build evidence does not match the delivered executable")
    cases = ET.parse(results / "ctest.xml").getroot().findall(".//testcase")
    failed = [case for case in cases if case.find("failure") is not None or case.find("error") is not None]
    if not cases or failed:
        raise ValueError("CTest results are missing or failed")
    timestamp = datetime.now(timezone.utc).isoformat()
    record = {"verifiedAtUtc": timestamp, "scope": "SDK consumer build and display tests",
              "exeSha256": exe_hash,
              "inputDigest": hashlib.sha256(json.dumps(inputs, sort_keys=True).encode()).hexdigest(),
              "ctestGroups": {"total": len(cases), "passed": len(cases), "failed": 0}}
    RECORD.parent.mkdir(parents=True, exist_ok=True)
    RECORD.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    manifest = {"schema": "component-lab-display-v3", "generatedAtUtc": timestamp,
                "componentAuthority": False, "sdkOwner": "DesignToCode_GenerateComponent",
                "entrypoints": {"run": "build/demo/component_gallery/Run.cmd", "source": "Source/CMakeLists.txt"},
                "exeSha256": exe_hash, "validationStatus": "build-and-tests-passed",
                "ctestGroups": record["ctestGroups"],
                "fileListScope": "Source, docs and deployable demo; SDK is an external authoritative dependency", "files": inventory()}
    MANIFEST.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def check():
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8-sig"))
    if manifest["files"] != inventory():
        raise ValueError("Delivery inventory changed; rebuild to refresh it")
    print(f"PASS: {len(manifest['files'])} files; ComponentLab display package")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.results:
        refresh(args.results)
    if not args.results and not args.check:
        parser.error("Use --results or --check")
    check()
