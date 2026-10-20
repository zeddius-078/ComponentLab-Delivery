"""Offline orchestration only: reuse the maintained capture/adapter/compiler."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid
from contextlib import contextmanager


@contextmanager
def generation_lock(work):
    # Shipped alongside this launcher; no developer source lookup.
    from ArtifactLifecycle import operation_lock
    with operation_lock(work):
        yield


def no_links(path):
    for item in (path, *path.parents):
        if item.exists() and (item.is_symlink() or
                             getattr(item.lstat(), "st_file_attributes", 0) & 0x400):
            raise ValueError("PATH_REDIRECTED: " + str(item))


def inside(root, relative):
    if Path(relative).is_absolute() or ".." in Path(relative).parts:
        raise ValueError("UNSAFE_MANIFEST_PATH: " + relative)
    result = root / relative
    no_links(result)
    if not result.resolve().is_relative_to(root.resolve()):
        raise ValueError("PATH_ESCAPE: " + relative)
    return result


def verify(root):
    no_links(root)
    manifest = json.loads((root / "release.json").read_bytes())
    if manifest["schemaVersion"] != 1 or manifest["configuration"] != "Release":
        raise ValueError("INCOMPATIBLE_PACKAGE")
    seen = set()
    for item in manifest["files"]:
        path = inside(root, item["path"])
        key = item["path"].casefold()
        if key in seen:
            raise ValueError("DUPLICATE_MANIFEST_PATH")
        seen.add(key)
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != item["sha256"]:
            raise ValueError("PACKAGE_HASH_MISMATCH: " + item["path"])
    if not seen:
        raise ValueError("EMPTY_PACKAGE_MANIFEST")
    actual = {p.relative_to(root).as_posix().casefold() for p in root.rglob("*")
              if p.is_file() and p != root / "release.json"}
    if actual != seen:
        raise ValueError("PACKAGE_FILE_INVENTORY_MISMATCH")
    for key in ("node", "browser", "compiler", "uic", "gallery", "font", "mapping"):
        path = inside(root, manifest["runtime"][key])
        if not path.exists():
            raise ValueError("PACKAGE_INPUT_MISSING: " + key)
    for plugin in ("qwindows.dll", "qoffscreen.dll"):
        if not inside(root, "bin/platforms/" + plugin).is_file():
            raise ValueError("QT_PLATFORM_PLUGIN_MISSING: " + plugin)
    return manifest


def environment(root, work, config):
    env = {key: os.environ[key] for key in ("SystemRoot", "WINDIR", "COMSPEC")
           if key in os.environ}
    env.update({
        "PATH": str(root / "bin") + os.pathsep + str(Path(env.get("SystemRoot", "C:/Windows")) / "System32"),
        "D2C_RUNTIME_ROOT": str(root), "D2C_WORK_ROOT": str(work),
        "D2C_BROWSER_EXE": str(inside(root, config["browser"])),
        "D2C_NODE_MODULES": str(root / "node_modules"), "NODE_PATH": str(root / "node_modules"),
        "PYTHONDONTWRITEBYTECODE": "1", "PYTHONNOUSERSITE": "1",
        "PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD": "1", "PLAYWRIGHT_BROWSERS_PATH": str(root / "browser"),
        "QT_PLUGIN_PATH": str(root / "bin"), "QT_QPA_PLATFORM": "offscreen",
        "TEMP": str(work / "output/work/cache"), "TMP": str(work / "output/work/cache"),
        "LOCALAPPDATA": str(work / "output/work/cache/local"), "APPDATA": str(work / "output/work/cache/roaming"),
        "USERPROFILE": str(work / "output/work/cache/user"),
    })
    return env


def run(command, work, env):
    print("RUN:", subprocess.list2cmdline([str(c) for c in command]), flush=True)
    subprocess.run([str(c) for c in command], cwd=str(work), env=env, check=True, timeout=180)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["doctor", "generate", "gallery"])
    parser.add_argument("--work", required=True, type=Path, help="Existing work folder outside package")
    parser.add_argument("--capture", type=Path)
    parser.add_argument("--batch")
    parser.add_argument("--page")
    parser.add_argument("--library-profile")
    parser.add_argument("--quit-after", type=int, help="Gallery smoke-test duration in milliseconds")
    args = parser.parse_args()
    if sys.version_info[:2] != (3, 9):
        raise ValueError("PYTHON39_REQUIRED")
    root = Path(__file__).resolve().parents[1]
    config = verify(root)["runtime"]
    work = args.work.absolute()
    no_links(work)
    if not work.is_dir() or work.resolve().is_relative_to(root) or root.is_relative_to(work.resolve()):
        raise ValueError("WORK_ROOT_MUST_EXIST_AND_BE_SEPARATE_FROM_PACKAGE")
    for directory in ("output/work/cache", "output/logs", "output/delivery", "output/work/preview"):
        path = inside(work, directory)
        path.mkdir(parents=True, exist_ok=True)
    env = environment(root, work, config)
    node = inside(root, config["node"])
    if args.action == "doctor":
        run([sys.executable, "-I", "-B", "-X", "utf8", "--version"], work, env)
        run([node, "--version"], work, env)
        run([inside(root, config["uic"]), "-v"], work, env)
        run([inside(root, config["compiler"]), "--help"], work, env)
        run([node, "-e", "const {chromium}=require('playwright');"
             "(async()=>{const b=await chromium.launch({executablePath:process.env.D2C_BROWSER_EXE,headless:true});"
             "console.log('Browser:',b.version());await b.close()})().catch(e=>{console.error(e);process.exit(1)})"],
            work, env)
        print("PASS: package integrity and runtime tools. Not a visual or fresh-machine acceptance.")
    elif args.action == "gallery":
        env["QT_QPA_PLATFORM"] = "windows"
        command = [inside(root, config["gallery"])]
        if args.quit_after is not None:
            command.extend(["--quit-after", str(args.quit_after)])
        run(command, work, env)
    else:
        with generation_lock(work):
            generate(root, work, config, args, env, node)


def generate(root, work, config, args, env, node):
        if not args.capture or not all(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", v or "")
                                       for v in (args.batch, args.page)):
            raise ValueError("GENERATE_REQUIRES_CAPTURE_BATCH_PAGE")
        output = inside(work, "output/delivery/" + args.batch + "/" + args.page)
        if output.exists():
            raise ValueError("OUTPUT_ALREADY_EXISTS")
        capture = args.capture.resolve(strict=True)
        stage_root = inside(work, "output/work/page-stage-" + uuid.uuid4().hex)
        stage_root.mkdir()
        stage = stage_root / "output/delivery" / args.batch / args.page
        env = dict(env, D2C_WORK_ROOT=str(stage_root))
        run([sys.executable, "-I", "-B", "-X", "utf8", root / "scripts/capture/capture_manifest.py",
             "verify", "--manifest", capture, "--mode", "formal"], work, env)
        scratch = inside(stage_root, "output/work/adapt")
        report = inside(stage_root, "output/logs/adapt")
        run([node, root / "scripts/tools/adapt_v2.cjs", scratch, report, capture], work, env)
        mapping = inside(root, config["mapping"])
        command = [inside(root, config["compiler"]), "--raw", scratch / "hybrid-result.json",
                   "--mapping", mapping, "--extensions", mapping / "extensions.json",
                   "--font", inside(root, config["font"]), "--output", stage, "--strict-layout", "--formal",
                   "--tool-root", root, "--work-root", stage_root]
        if args.library_profile:
            command.extend(["--library-profile", args.library_profile])
        run(command, work, env)
        for extension in (".ui", ".h", ".cpp", ".qss", ".qrc"):
            if not list(stage.rglob("*" + extension)):
                raise ValueError("GENERATION_OUTPUT_MISSING: " + extension)
        from ArtifactLifecycle import inventory, write_json
        write_json(stage / "delivery-manifest.json", {
            "schemaVersion": 1, "mode": "formal",
            "captureManifestSha256": hashlib.sha256(capture.read_bytes()).hexdigest(),
            "releaseManifestSha256": hashlib.sha256((root / "release.json").read_bytes()).hexdigest(),
            "files": inventory(stage), "visualApproval": False, "freshMachinePassed": False})
        output.parent.mkdir(parents=True, exist_ok=True)
        stage.rename(output)
        shutil.rmtree(stage_root)
        print("PASS: generated page package at " + str(output) + "; visual approval remains separate.")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    try:
        main()
    except (ValueError, KeyError, OSError, subprocess.SubprocessError) as error:
        print("FAILED:", error, file=sys.stderr)
        sys.exit(2)
