"""SDK provenance, complete-package verification and recoverable local promotion.

Python 3.9 standard library only. Verification never writes into the SDK.
The publisher holds its OS-level exclusive lock across build and promotion.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import uuid

PROJECT = Path(__file__).resolve().parent.parent
MANIFEST = "manifest/component-sdk.json"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def write_json(path, value):
    Path(path).write_bytes((json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))


def reject_links(path):
    for entry in (path, *path.parents):
        if entry.exists() and (entry.is_symlink() or
                getattr(entry.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
            raise ValueError("SDK_PATH_IS_LINK: " + str(entry))


def build_path(path, project=PROJECT):
    path = Path(path).absolute()
    reject_links(path)
    root = (Path(project) / "output/work").resolve()
    if path.resolve() == root or not path.resolve().is_relative_to(root):
        raise ValueError("SDK_OUTPUT_MUST_BE_IN_WORK: " + str(path))
    return path


def controlled_path(path, project=PROJECT):
    path = Path(path).absolute()
    reject_links(path)
    root = (Path(project) / "build").resolve()
    if path.resolve() == root or not path.resolve().is_relative_to(root):
        raise ValueError("SDK_AUTHORITY_MUST_BE_IN_CONTROLLED_BUILD: " + str(path))
    return path


def shared_contract():
    # An explicit support override is authoritative, including when invalid.
    configured = os.environ.get("D2C_SUPPORT_ROOT")
    support = Path(configured) if configured else Path(__file__).resolve().parents[2] / "Source"
    module_path = support / "SDKContract.py"
    if not configured and not support.exists():
        # Standalone project copies consume the controlled support tool copy.
        module_path = PROJECT / "build/tools/support/SDKContract.py"
    if not module_path.is_file():
        raise ValueError("SDK_SUPPORT_MISSING: supply D2C_SUPPORT_ROOT with SDKContract.py")
    reject_links(module_path)
    spec = importlib.util.spec_from_file_location("d2c_shared_sdk_contract", module_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def source_inventory(project):
    project = Path(project).resolve()
    if not (project / "Source").is_dir():
        raise ValueError("SDK_SOURCE_DIRECTORY_MISSING")
    inputs = list((project / "Source").rglob("*"))
    inputs += [project / relative for relative in (
        "docs/designRules/design-system/component-traceability-manifest.json",
        "docs/examples/MappingWorkbench/mapping/qt-component-map.json",
        "docs/examples/MappingWorkbench/mapping/qt-native-catalog.json",
        "docs/validation/SDKConsumerTest/CMakeLists.txt",
        "docs/validation/SDKConsumerTest/main.cpp",
        "docs/validation/SDKConsumerTest/Build.ps1",
    )]
    files = {}
    for path in sorted(inputs):
        if "__pycache__" in path.parts or path.suffix == ".pyc":
            continue
        reject_links(path)
        if path.is_dir():
            continue
        if not path.is_file():
            raise ValueError("SDK_SOURCE_INPUT_MISSING: " + str(path))
        files[path.relative_to(project).as_posix()] = digest(path.read_bytes())
    if not files:
        raise ValueError("SDK_SOURCE_INPUTS_EMPTY")
    return files


def fingerprint(files):
    return digest(json.dumps(files, sort_keys=True, separators=(",", ":")).encode("utf-8"))


def snapshot(project):
    files = source_inventory(project)
    source_hash = fingerprint(files)
    def git(*args):
        try:
            return subprocess.check_output(["git", "-C", str(project), *args],
                                           stderr=subprocess.DEVNULL).decode("utf-8").strip()
        except (OSError, subprocess.CalledProcessError):
            return None
    now = datetime.now(timezone.utc)
    return {
        "sourceFingerprint": source_hash, "sourceFiles": files,
        "git": {"commit": git("rev-parse", "HEAD"),
                "dirty": bool(git("status", "--porcelain", "--", ".")),
                "available": git("rev-parse", "--is-inside-work-tree") == "true"},
        "createdAtUtc": now.isoformat(),
        "packageVersion": "{}.{}.{}.{}".format(now.year, now.month, now.day, now.strftime("%H%M%S")),
        "releaseId": now.strftime("%Y%m%dT%H%M%SZ") + "-" + source_hash[:12] + "-" + uuid.uuid4().hex[:8],
    }


def package_files(sdk):
    files = []
    reject_links(sdk)
    for path in sorted(sdk.rglob("*")):
        reject_links(path)
        if not path.is_file() or path.relative_to(sdk).as_posix() == MANIFEST:
            continue
        data = path.read_bytes()
        files.append({"path": path.relative_to(sdk).as_posix(), "sha256": digest(data), "bytes": len(data)})
    return sorted(files, key=lambda entry: entry["path"])


def verify(sdk, configuration="Release", project=None):
    sdk = Path(sdk).absolute()
    manifest = shared_contract().verify(sdk, configuration)
    if project is not None and fingerprint(source_inventory(project)) != manifest["sourceFingerprint"]:
        raise ValueError("SDK_STALE_SOURCE: rebuild from current maintained inputs")
    return manifest


def stamp(sdk, snapshot_path, qt_root, cmake, build_directory):
    sdk = build_path(sdk)
    baseline = read_json(snapshot_path)
    if fingerprint(source_inventory(PROJECT)) != baseline["sourceFingerprint"]:
        raise ValueError("SDK_SOURCE_CHANGED_DURING_BUILD")
    qmake = Path(qt_root) / "bin/qmake.exe"
    qt_version = subprocess.check_output([str(qmake), "-query", "QT_VERSION"]).decode().strip()
    cmake_version = subprocess.check_output([cmake, "--version"]).decode().splitlines()[0]
    cache = (Path(build_directory) / "CMakeCache.txt").read_text(encoding="utf-8")
    def cache_setting(name):
        match = re.search(r"^" + name + r":[^=]+=(.*)$", cache, re.M)
        if not match:
            raise ValueError("SDK_BUILD_CONFIGURATION_MISSING: " + name)
        return match.group(1).strip()
    if cache_setting("CMAKE_BUILD_TYPE") != "Release" or \
            cache_setting("CMAKE_MSVC_RUNTIME_LIBRARY") != "MultiThreadedDLL":
        raise ValueError("SDK_BUILD_CONFIGURATION_MISMATCH")
    if Path(cache_setting("Qt5Core_DIR")).resolve() != (Path(qt_root) / "lib/cmake/Qt5Core").resolve():
        raise ValueError("SDK_CACHED_QT_ROOT_MISMATCH: configure a fresh build directory")
    compiler_files = sorted(Path(build_directory).glob("CMakeFiles/*/CMakeCXXCompiler.cmake"))
    if not compiler_files:
        raise ValueError("SDK_COMPILER_METADATA_MISSING")
    compiler = compiler_files[-1].read_text(encoding="utf-8")
    def cmake_setting(name):
        match = re.search(r'set\(' + name + r'\s+"?([^")\n]+)', compiler)
        if not match:
            raise ValueError("SDK_COMPILER_METADATA_MISSING: " + name)
        return match.group(1).strip()
    compiler_id = cmake_setting("CMAKE_CXX_COMPILER_ID")
    architecture = cmake_setting("CMAKE_CXX_COMPILER_ARCHITECTURE_ID")
    if compiler_id != "MSVC" or architecture.lower() not in ("x64", "amd64"):
        raise ValueError("SDK_REQUIRES_MSVC_X64")
    manifest = read_json(sdk / MANIFEST)
    manifest.update(baseline)
    manifest["schemaVersion"] = 2
    manifest["toolchain"] = {
        "configuration": "Release", "architecture": "x64", "crt": "MD",
        "qtVersion": qt_version, "pythonVersion": ".".join(map(str, sys.version_info[:3])),
        "cmakeVersion": cmake_version, "compilerId": compiler_id,
        "compilerVersion": cmake_setting("CMAKE_CXX_COMPILER_VERSION"),
    }
    version_file = sdk / "cmake/GeneratedComponentSDKConfigVersion.cmake"
    text = version_file.read_text(encoding="utf-8-sig")
    text = re.sub(r'set\(PACKAGE_VERSION "[^"]+"\)',
                  'set(PACKAGE_VERSION "{}")'.format(baseline["packageVersion"]), text)
    version_file.write_bytes(text.encode("utf-8"))
    manifest["files"] = package_files(sdk)
    write_json(sdk / MANIFEST, manifest)
    return verify(sdk, project=PROJECT)


def promote(stage, destination, project=PROJECT, rename=None):
    """Rename only after validation. A failed cutover restores the previous SDK."""
    stage = build_path(stage, project)
    destination = controlled_path(destination, project)
    if destination != Path(project).absolute() / "build/sdk":
        raise ValueError("SDK_DESTINATION_MUST_BE_LOCAL_AUTHORITY")
    if stage == destination or stage.is_relative_to(destination):
        raise ValueError("SDK_STAGE_MUST_BE_SEPARATE")
    verify(stage, project=project)
    move = rename or (lambda source, target: source.rename(target))
    backup = controlled_path(Path(project) / "build/.state/previous-sdk", project)
    backup.parent.mkdir(parents=True, exist_ok=True)
    # Keep exactly one previous valid release in the controlled area. An older
    # rollback package is parked in disposable work until cutover succeeds.
    retired = build_path(Path(project) / "output/work/cache" / ("retired-sdk-" + uuid.uuid4().hex), project)
    retired.parent.mkdir(parents=True, exist_ok=True)
    if backup.exists() and not destination.exists():
        verify(backup)
        backup.rename(destination)
    if destination.exists():
        verify(destination)
    had_backup = backup.exists()
    if had_backup:
        backup.rename(retired)
    had_previous = destination.exists()
    installed = False
    try:
        if had_previous:
            move(destination, backup)
        move(stage, destination)
        installed = True
        verify(destination, project=project)
    except BaseException:
        if installed and destination.exists():
            failed = build_path(Path(project) / "output/work/cache" / ("failed-sdk-" + uuid.uuid4().hex), project)
            destination.rename(failed)
        if had_previous and backup.exists():
            backup.rename(destination)
        if had_backup and retired.exists():
            retired.rename(backup)
        raise
    retired_scratch = None
    if retired.exists():
        reject_links(retired)
        for item in retired.rglob("*"):
            reject_links(item)
        try:
            shutil.rmtree(retired)
        except OSError:
            retired_scratch = str(retired)
    return {"sdk": str(destination), "previousSdk": str(backup) if had_previous else None,
            "retiredScratch": retired_scratch}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    fp = commands.add_parser("fingerprint")
    fp.add_argument("--project-root", type=Path, default=PROJECT)
    check = commands.add_parser("verify")
    check.add_argument("--sdk", type=Path, required=True)
    check.add_argument("--configuration", choices=("Release", "Debug"), default="Release")
    check.add_argument("--project-root", type=Path)
    mark = commands.add_parser("stamp")
    for name in ("sdk", "snapshot", "qt-root", "cmake", "build-directory"):
        mark.add_argument("--" + name, required=True)
    switch = commands.add_parser("promote")
    switch.add_argument("--stage", required=True)
    switch.add_argument("--destination", required=True)
    args = parser.parse_args()
    if sys.version_info[:2] != (3, 9):
        parser.error("Python 3.9 is required; do not update the installed environment.")
    if args.command == "fingerprint":
        result = snapshot(args.project_root)
    elif args.command == "verify":
        manifest = verify(args.sdk, args.configuration, args.project_root)
        result = {"status": "PASS", "releaseId": manifest["releaseId"], "sourceFingerprint": manifest["sourceFingerprint"]}
    elif args.command == "stamp":
        manifest = stamp(args.sdk, args.snapshot, args.qt_root, args.cmake, args.build_directory)
        result = {"status": "PASS", "releaseId": manifest["releaseId"]}
    else:
        result = promote(args.stage, args.destination)
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
