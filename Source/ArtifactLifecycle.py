"""Controlled build tools, transactional promotion and scoped output cleanup.

Source is editable; build is protected; only output is the execution workspace.
No environment installation or network access.
"""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import uuid

from SDKContract import reject_links, sha, verify as verify_sdk
from AgentCapabilityProfile import binding as execution_capability_binding

MANIFEST = "tool-manifest.json"
_HELD_LOCKS = set()


def verify_execution_capability(value):
    """Validate the label carried by a newly controlled tool without guessing its model."""
    required = {"canonicalId", "displayName", "executionClass", "profileSha256"}
    if not isinstance(value, dict) or required - value.keys():
        raise ValueError("EXECUTION_CAPABILITY_PROFILE_MISSING")
    if (not isinstance(value["canonicalId"], str) or not value["canonicalId"].strip()
            or not isinstance(value["profileSha256"], str)
            or len(value["profileSha256"]) != 64):
        raise ValueError("EXECUTION_CAPABILITY_PROFILE_INVALID")
    return value


def safe_path(project, relative):
    project = Path(project).absolute()
    reject_links(project)
    relative = Path(relative)
    if relative.is_absolute() or ".." in relative.parts or not relative.parts:
        raise ValueError("UNSAFE_RELATIVE_PATH")
    target = project / relative
    reject_links(target)
    if not target.resolve().is_relative_to(project.resolve()):
        raise ValueError("PATH_ESCAPE")
    return target


def under_work(project, path):
    path = Path(path).absolute()
    reject_links(path)
    root = safe_path(project, "output/work")
    if path.resolve() == root.resolve() or not path.resolve().is_relative_to(root.resolve()):
        raise ValueError("STAGE_MUST_BE_IN_OUTPUT_WORK")
    return path


def inventory(root, exclude=()):
    root = Path(root)
    reject_links(root)
    if not root.is_dir():
        raise ValueError("ARTIFACT_DIRECTORY_MISSING: " + str(root))
    result = {}
    for path in sorted(root.rglob("*")):
        reject_links(path)
        if path.is_file():
            name = path.relative_to(root).as_posix()
            if name not in exclude:
                result[name] = {"sha256": sha(path), "bytes": path.stat().st_size}
    return result


def fingerprint(project):
    project = Path(project).absolute()
    files = {}
    for relative in ("Source", "docs/validation", "docs/fixtures", "docs/examples"):
        root = project / relative
        if not root.is_dir():
            continue
        for name, item in inventory(root).items():
            if "__pycache__" not in Path(name).parts and Path(name).suffix != ".pyc":
                files[relative + "/" + name] = item["sha256"]
    support = project.parent / "Source"
    if not (support / "SDKContract.py").is_file():
        support = project / "build/tools/support"
    shared = ["SDKContract.py", "ArtifactLifecycle.py", "OperationLock.ps1"]
    if project.name == "DesignToCode_GenerateLayout":
        shared += ["RuntimeRun.py", "offline/LayoutRun.cmd"]
    for name in shared:
        if (support / name).is_file():
            files["shared-support/" + name] = sha(support / name)
    if not files:
        raise ValueError("SOURCE_INPUTS_MISSING")
    return hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def write_json(path, value):
    path = Path(path)
    reject_links(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_name(path.name + ".writing-" + uuid.uuid4().hex)
    temp.write_bytes((json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    os.replace(str(temp), str(path))


@contextmanager
def operation_lock(project, allow_reentrant=False, allow_inherited=True):
    lock = safe_path(project, "build/.state/operation.lock")
    key = str(Path(project).resolve()).casefold()
    # A direct Python child of a locked PowerShell build shares that operation.
    inherited = os.environ.get("D2C_OPERATION_LOCK_ROOT", "")
    inherited_owner = os.environ.get("D2C_OPERATION_LOCK_OWNER_PID", "")
    if (allow_reentrant and key in _HELD_LOCKS) or (allow_inherited and inherited and str(Path(inherited).resolve()).casefold() == key
                             and inherited_owner == str(os.getppid())):
        yield
        return
    lock.parent.mkdir(parents=True, exist_ok=True)
    handle = None
    locked = False
    try:
        try:
            handle = open(str(lock), "a+b")
            if os.name == "nt":
                import msvcrt
                if lock.stat().st_size == 0:
                    handle.write(b"\0")
                    handle.flush()
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            locked = True
            _HELD_LOCKS.add(key)
        except OSError as error:
            raise ValueError("PROJECT_OPERATION_LOCKED") from error
        yield
    finally:
        if handle:
            if locked:
                _HELD_LOCKS.discard(key)
            if locked and os.name == "nt":
                import msvcrt
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            handle.close()


def receipt(project, stage, sdk, source_fingerprint, checks, output):
    project = Path(project).absolute()
    stage = under_work(project, stage)
    output = Path(output).absolute()
    reject_links(output)
    if not output.resolve().is_relative_to(safe_path(project, "output/logs").resolve()):
        raise ValueError("RECEIPT_MUST_BE_IN_OUTPUT_LOGS")
    if source_fingerprint != fingerprint(project):
        raise ValueError("SOURCE_CHANGED_DURING_BUILD")
    verify_sdk(sdk)
    checks = [name for name in checks.split(",") if name]
    if not checks:
        raise ValueError("VALIDATION_CHECKS_REQUIRED")
    files = inventory(stage)
    if not files or MANIFEST in files:
        raise ValueError("INVALID_TOOL_STAGE")
    value = {"schemaVersion": 1, "status": "PASS", "project": project.name,
             "sourceFingerprint": source_fingerprint, "sdkRoot": str(Path(sdk).resolve()),
             "sdkManifestSha256": sha(Path(sdk) / "manifest/component-sdk.json"),
             "stageFiles": files, "checks": checks,
             "executionCapabilityProfile": execution_capability_binding(),
             "freshMachinePassed": False, "visualApproval": False}
    write_json(output, value)
    return value


def verify_tools(root):
    root = Path(root).absolute()
    reject_links(root)
    manifest = json.loads((root / MANIFEST).read_bytes())
    if manifest.get("schemaVersion") != 1 or manifest.get("kind") != "controlled-tool":
        raise ValueError("TOOL_MANIFEST_INVALID")
    if manifest.get("files") != inventory(root, (MANIFEST,)):
        raise ValueError("TOOL_INTEGRITY_MISMATCH")
    acceptance = manifest.get("acceptance", {})
    if acceptance.get("status") != "PASS" or not acceptance.get("checks"):
        raise ValueError("TOOL_ACCEPTANCE_MISSING")
    # Packages published before this profile existed remain rebuildable but are
    # deliberately not misrepresented as newly baseline-labelled tools.
    if "executionCapabilityProfile" in acceptance:
        verify_execution_capability(acceptance["executionCapabilityProfile"])
    return manifest


def switch_directory(project, stage, destination, name, validator):
    """Caller holds the project lock. Recover an interrupted rename on next run."""
    previous = safe_path(project, "build/.state/previous-" + name)
    journal = safe_path(project, "build/.state/pending-" + name + ".json")
    destination.parent.mkdir(parents=True, exist_ok=True)
    if journal.exists():
        pending = json.loads(journal.read_bytes())
        valid = False
        if destination.exists():
            try:
                validator(destination)
                valid = True
            except (ValueError, OSError, KeyError):
                destination.rename(safe_path(project, "output/work/interrupted-" + name + "-" + uuid.uuid4().hex))
        if not valid and pending["hadPrevious"] and previous.exists():
            previous.rename(destination)
        elif not valid and pending["hadPrevious"]:
            raise ValueError("PUBLICATION_RECOVERY_MISSING_PREVIOUS")
        journal.unlink()
    if previous.exists():
        previous.rename(safe_path(project, "output/work/obsolete-" + name + "-" + uuid.uuid4().hex))
    had_previous = destination.exists()
    write_json(journal, {"schemaVersion": 1, "hadPrevious": had_previous})
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        if had_previous:
            destination.rename(previous)
        stage.rename(destination)
        validator(destination)
    except BaseException:
        # If the first rename failed, destination is still the previous valid tree.
        if destination.exists() and (not had_previous or previous.exists()):
            destination.rename(safe_path(project, "output/work/failed-" + name + "-" + uuid.uuid4().hex))
        if had_previous and previous.exists():
            previous.rename(destination)
        journal.unlink()
        raise
    journal.unlink()


def publish(project, stage, name, receipt_path):
    project = Path(project).absolute()
    if not re.fullmatch(r"[a-z][a-z0-9-]*", name):
        raise ValueError("INVALID_TOOL_NAME")
    stage = under_work(project, stage)
    destination = safe_path(project, "build/tools/" + name)
    with operation_lock(project):
        record = json.loads(Path(receipt_path).read_bytes())
        if record.get("status") != "PASS" or record.get("project") != project.name:
            raise ValueError("ACCEPTANCE_INVALID")
        if record.get("sourceFingerprint") != fingerprint(project):
            raise ValueError("ACCEPTANCE_STALE_SOURCE")
        verify_execution_capability(record.get("executionCapabilityProfile"))
        verify_sdk(record["sdkRoot"])
        if sha(Path(record["sdkRoot"]) / "manifest/component-sdk.json") != record["sdkManifestSha256"]:
            raise ValueError("ACCEPTANCE_STALE_SDK")
        if record.get("stageFiles") != inventory(stage):
            raise ValueError("ACCEPTANCE_STALE_STAGE")
        manifest = {"schemaVersion": 1, "kind": "controlled-tool", "name": name,
                    "files": record["stageFiles"], "acceptance": record}
        write_json(stage / MANIFEST, manifest)
        verify_tools(stage)
        switch_directory(project, stage, destination, name, verify_tools)
    return {"status": "PASS", "toolRoot": str(destination)}


def clean(project, delivery=None, mark_only=False, dry_run=False):
    project = Path(project).absolute()
    targets = [safe_path(project, "output/work"), safe_path(project, "output/logs")]
    if delivery:
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", delivery):
            raise ValueError("INVALID_DELIVERY_BATCH")
        targets = [safe_path(project, "output/delivery/" + delivery)]
    if dry_run:
        return {"dryRun": True, "targets": [str(p) for p in targets if p.exists()]}
    results = []
    with operation_lock(project, allow_inherited=False):
        if list(safe_path(project, "build/.state").glob("pending-*.json")):
            raise ValueError("PUBLICATION_RECOVERY_REQUIRED")
        for target in targets:
            if not target.exists():
                continue
            inventory(target)  # Check all descendants before recursive operations.
            if not mark_only:
                try:
                    shutil.rmtree(str(target))
                    results.append({"removed": str(target)})
                    continue
                except OSError:
                    pass
            pending = target.with_name("需删除-" + target.name + "-" + uuid.uuid4().hex[:8])
            target.rename(pending)
            results.append({"pendingDeletion": str(pending)})
    return {"status": "CLEANED_OR_MARKED", "results": results}


def main():
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="action", required=True)
    fp = subs.add_parser("fingerprint")
    fp.add_argument("--project", type=Path, required=True)
    rp = subs.add_parser("receipt")
    for key in ("project", "stage", "sdk", "output"):
        rp.add_argument("--" + key, type=Path, required=True)
    rp.add_argument("--source-fingerprint", required=True)
    rp.add_argument("--checks", required=True)
    pp = subs.add_parser("publish-tools")
    for key in ("project", "stage", "receipt"):
        pp.add_argument("--" + key, type=Path, required=True)
    pp.add_argument("--name", required=True)
    vp = subs.add_parser("verify-tools")
    vp.add_argument("--root", type=Path, required=True)
    cp = subs.add_parser("clean")
    cp.add_argument("--project", type=Path, required=True)
    cp.add_argument("--delivery")
    cp.add_argument("--mark-only", action="store_true")
    cp.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if args.action == "fingerprint":
        print(fingerprint(args.project))
        return
    if args.action == "receipt":
        result = receipt(args.project, args.stage, args.sdk, args.source_fingerprint, args.checks, args.output)
        result = {"status": result["status"], "receipt": str(args.output), "checks": result["checks"]}
    elif args.action == "publish-tools":
        result = publish(args.project, args.stage, args.name, args.receipt)
    elif args.action == "verify-tools":
        result = verify_tools(args.root)
        result = {"status": "PASS", "toolRoot": str(args.root), "fileCount": len(result["files"])}
    else:
        result = clean(args.project, args.delivery, args.mark_only, args.dry_run)
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
