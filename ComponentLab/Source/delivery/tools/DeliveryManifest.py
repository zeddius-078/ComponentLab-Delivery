"""Validate one accepted gallery and create a bounded release ZIP.

Acceptance belongs to the installed tool manifest (or an explicit identical
receipt), never to a search through accumulated logs.
"""
import argparse
import importlib
import json
import os
from pathlib import Path
import sys
import uuid
import zipfile

PROJECT = Path(__file__).resolve().parents[3]


def support_modules():
    configured = os.environ.get("D2C_SUPPORT_ROOT")
    candidates = ([Path(configured)] if configured else
                  [PROJECT.parent / "Source", PROJECT / "build/tools/support"])
    for root in candidates:
        if (root / "ArtifactLifecycle.py").is_file() and (root / "SDKContract.py").is_file():
            sys.path.insert(0, str(root.resolve()))
            return importlib.import_module("ArtifactLifecycle"), importlib.import_module("SDKContract")
    raise ValueError("SUPPORT_TOOL_MISSING")


def validate(dist, receipt=None, lifecycle=None, contract=None):
    if lifecycle is None or contract is None:
        lifecycle, contract = support_modules()
    manifest = lifecycle.verify_tools(dist)
    acceptance = manifest["acceptance"]
    if receipt is not None:
        explicit = json.loads(Path(receipt).read_bytes())
        if explicit != acceptance:
            raise ValueError("EXPLICIT_RECEIPT_DOES_NOT_MATCH_INSTALLED_TOOL")
    if acceptance.get("project") != PROJECT.name or acceptance.get("status") != "PASS":
        raise ValueError("GALLERY_ACCEPTANCE_INVALID")
    if not {"ctest", "token", "native"}.issubset(set(acceptance.get("checks", []))):
        raise ValueError("GALLERY_FULL_VALIDATION_REQUIRED")
    if acceptance.get("sourceFingerprint") != lifecycle.fingerprint(PROJECT):
        raise ValueError("GALLERY_SOURCE_CHANGED_REBUILD_REQUIRED")
    sdk = Path(acceptance["sdkRoot"])
    contract.verify(sdk)
    if acceptance.get("sdkManifestSha256") != contract.sha(sdk / "manifest/component-sdk.json"):
        raise ValueError("GALLERY_SDK_CHANGED_REBUILD_REQUIRED")
    if acceptance.get("stageFiles") != manifest.get("files"):
        raise ValueError("GALLERY_RECEIPT_FILE_MISMATCH")
    if "component_lab.exe" not in manifest["files"] or "Run.cmd" not in manifest["files"]:
        raise ValueError("GALLERY_REQUIRED_FILE_MISSING")
    return manifest


def package(dist, output, receipt=None):
    lifecycle, contract = support_modules()
    output = Path(output).absolute()
    contract.reject_links(output)
    delivery = PROJECT / "output/delivery"
    if not output.resolve().is_relative_to(delivery.resolve()) or output.suffix.lower() != ".zip":
        raise ValueError("PACKAGE_MUST_BE_ZIP_IN_OUTPUT_DELIVERY")
    if output.exists():
        raise ValueError("PACKAGE_ALREADY_EXISTS")
    # Same project lock as promotion/cleanup: the stable gallery cannot change
    # halfway through packaging.
    with lifecycle.operation_lock(PROJECT):
        manifest = validate(dist, receipt, lifecycle, contract)
        files = lifecycle.inventory(dist)
        acceptance = manifest["acceptance"]
        record = {"schemaVersion": 1, "kind": "component-lab-gallery",
                  "componentAuthority": False, "entrypoint": "Run.cmd",
                  "sourceFingerprint": acceptance["sourceFingerprint"],
                  "sdkManifestSha256": acceptance["sdkManifestSha256"],
                  "checks": acceptance["checks"], "freshMachinePassed": False,
                  "visualApproval": False, "files": files}
        output.parent.mkdir(parents=True, exist_ok=True)
        temporary = output.with_name(output.name + ".staging-" + uuid.uuid4().hex)
        try:
            with zipfile.ZipFile(str(temporary), "x", compression=zipfile.ZIP_DEFLATED) as archive:
                for name in files:
                    archive.write(str(Path(dist) / name), "ComponentLab/" + name)
                archive.writestr("ComponentLab/DELIVERY.json",
                                 json.dumps(record, ensure_ascii=False, indent=2).encode("utf-8"))
            if lifecycle.inventory(dist) != files:
                raise ValueError("GALLERY_CHANGED_DURING_PACKAGE")
            # On Windows rename refuses an existing destination; never overwrite.
            if output.exists():
                raise ValueError("PACKAGE_ALREADY_EXISTS")
            temporary.rename(output)
        except BaseException:
            if temporary.exists():
                temporary.unlink()
            raise
    return {"status": "PASS", "path": str(output), "sha256": contract.sha(output),
            "bytes": output.stat().st_size, "freshMachinePassed": False}


def main():
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dist", type=Path, default=PROJECT / "build/tools/gallery")
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.output:
        print(json.dumps(package(args.dist, args.output, args.receipt), ensure_ascii=False))
    elif args.check:
        result = validate(args.dist, args.receipt)
        print(json.dumps({"status": "PASS", "files": len(result["files"])}))
    else:
        parser.error("Use --check or --output; no historical report search is supported")


if __name__ == "__main__":
    main()
