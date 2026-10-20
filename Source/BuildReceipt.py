"""Bind local build/verification receipts to exact maintained inputs and SDK."""
import argparse
import hashlib
import json
from pathlib import Path
from ArtifactLifecycle import fingerprint, verify_tools

ROOT = Path(__file__).resolve().parent.parent
PROJECTS = ("DesignToCode_GenerateComponent", "DesignToCode_GenerateLayout", "ComponentLab")
RECEIPT = ROOT / "DesignToCode_GenerateLayout/build/.state/workspace-acceptance.json"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    files = []
    for directory in (ROOT / "Source", *(ROOT / p / "Source" for p in PROJECTS)):
        files.extend(p for p in directory.rglob("*") if p.is_file()
                     and "__pycache__" not in p.parts and p.suffix != ".pyc")
    for project in PROJECTS:
        for sub in ("validation", "fixtures", "examples"):
            files.extend(p for p in (ROOT / project / "docs" / sub).rglob("*")
                         if p.is_file() and "__pycache__" not in p.parts)
    files.append(ROOT / "DesignToCode_GenerateComponent/docs/designRules/design-system/component-traceability-manifest.json")
    files.extend(p for p in (ROOT / "docs/validation").rglob("*")
                 if p.is_file() and "__pycache__" not in p.parts)
    return {p.relative_to(ROOT).as_posix(): sha(p) for p in sorted(set(files))}


def current(sdk):
    binaries = ("DesignToCode_GenerateLayout/build/tools/layout/bin/v2_ui_compiler.exe",
                "ComponentLab/build/tools/gallery/component_lab.exe")
    return {"inputs": inputs(), "sdkManifestSha256": sha(sdk / "manifest/component-sdk.json"),
            "binaries": {p: sha(ROOT / p) for p in binaries},
            "galleryDeployment": {
                p.relative_to(ROOT / "ComponentLab/build/tools/gallery").as_posix(): sha(p)
                for p in sorted((ROOT / "ComponentLab/build/tools/gallery").rglob("*"))
                if p.is_file()},
            "layoutDeployment": {
                p.relative_to(ROOT / "DesignToCode_GenerateLayout/build/tools/layout").as_posix(): sha(p)
                for p in sorted((ROOT / "DesignToCode_GenerateLayout/build/tools/layout").rglob("*"))
                if p.is_file()}}


def check(sdk, require_verified=True):
    for name, role in (("DesignToCode_GenerateLayout", "layout"), ("ComponentLab", "gallery")):
        project = ROOT / name
        tool = verify_tools(project / "build/tools" / role)
        accepted = tool["acceptance"]
        if accepted["sourceFingerprint"] != fingerprint(project):
            raise ValueError("STALE_TOOL_SOURCE: " + name)
        if accepted["sdkManifestSha256"] != sha(sdk / "manifest/component-sdk.json"):
            raise ValueError("STALE_TOOL_SDK: " + name)
    previous = json.loads(RECEIPT.read_bytes())
    now = current(sdk)
    if any(previous.get(key) != value for key, value in now.items()):
        raise ValueError("STALE_BUILD_RECEIPT: rebuild/verify after input changes")
    if require_verified and not previous.get("developmentRegressionPassed"):
        raise ValueError("FULL_DEVELOPMENT_VERIFICATION_REQUIRED")
    return previous


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["record", "verify"])
    parser.add_argument("--sdk", required=True, type=Path)
    parser.add_argument("--verified", action="store_true")
    args = parser.parse_args()
    if args.action == "verify":
        check(args.sdk)
    else:
        result = current(args.sdk)
        result.update(developmentRegressionPassed=args.verified, freshMachinePassed=False,
                      visualApproval=False)
        RECEIPT.parent.mkdir(parents=True, exist_ok=True)
        RECEIPT.write_bytes(json.dumps(result, indent=2).encode("utf-8"))
    print(RECEIPT)
