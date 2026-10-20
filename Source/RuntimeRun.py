"""Use a published layout tool without VS, CMake, installed Node or SDK rebuilds."""
import argparse
from contextlib import ExitStack
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid

from ArtifactLifecycle import inventory, operation_lock, safe_path, verify_tools, write_json
from SDKContract import reject_links, sha


def run(command, env, work):
    subprocess.run([str(x) for x in command], env=env, cwd=str(work), check=True, timeout=300)


def main():
    if sys.version_info[:2] != (3, 9):
        raise ValueError("PYTHON39_REQUIRED")
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=("doctor", "generate", "preview"))
    p.add_argument("--tool-root", type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument("--runtime-root", type=Path)
    p.add_argument("--work", required=True, type=Path)
    p.add_argument("--capture", type=Path)
    p.add_argument("--snapshot", type=Path)
    p.add_argument("--style", type=Path)
    p.add_argument("--batch")
    p.add_argument("--page")
    p.add_argument("--library-profile")
    p.add_argument("--mapping", type=Path, help="Explicit mapping directory; does not mutate controlled mappings")
    args = p.parse_args()
    tool = args.tool_root.absolute()
    runtime = (args.runtime_root or tool.parent.parent / "runtime").absolute()
    work = args.work.absolute()
    for path in (tool, runtime, work):
        reject_links(path)
    manifest = json.loads((runtime / "runtime-manifest.json").read_bytes())
    config = manifest["runtime"]
    if not work.is_dir() or work.resolve().is_relative_to(tool.resolve()) or work.resolve().is_relative_to(runtime.resolve()):
        raise ValueError("WORK_MUST_EXIST_OUTSIDE_CONTROLLED_ASSETS")
    for project in (tool.parent.parent.parent, runtime.parent.parent):
        for lane in ("Source", "docs", "build"):
            if work.resolve().is_relative_to((project / lane).resolve()):
                raise ValueError("WORK_CANNOT_BE_MAINTAINED_OR_CONTROLLED_TREE")
    scratch = safe_path(work, "output/work/runtime")
    env = {k: os.environ[k] for k in ("SystemRoot", "WINDIR", "COMSPEC") if k in os.environ}
    env.update(
        PATH=str(tool / "bin") + os.pathsep + str(Path(env.get("SystemRoot", "C:/Windows")) / "System32"),
        D2C_RUNTIME_ROOT=str(tool), D2C_WORK_ROOT=str(work),
        D2C_BROWSER_EXE=str(safe_path(runtime, config["browser"])),
        D2C_NODE_MODULES=str(runtime / "node_modules"), NODE_PATH=str(runtime / "node_modules"),
        PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD="1", PLAYWRIGHT_BROWSERS_PATH=str(runtime / "browser"),
        PYTHONDONTWRITEBYTECODE="1", PYTHONNOUSERSITE="1",
        QT_PLUGIN_PATH=str(tool / "bin"), QT_QPA_PLATFORM_PLUGIN_PATH=str(tool / "bin/platforms"),
        QT_QPA_PLATFORM="offscreen", TEMP=str(scratch), TMP=str(scratch),
        LOCALAPPDATA=str(scratch / "local"), APPDATA=str(scratch / "roaming"), USERPROFILE=str(scratch / "user"))
    node = safe_path(runtime, config["node"])
    python = safe_path(runtime, config["python"])
    compiler = tool / "bin/v2_ui_compiler.exe"
    # Generation and cleaning the same work root are mutually exclusive.
    with ExitStack() as locks:
        projects = {str(path.resolve()).casefold(): path for path in (
            work, tool.parent.parent.parent, runtime.parent.parent)}
        for key in sorted(projects):
            locks.enter_context(operation_lock(projects[key]))
        scratch.mkdir(parents=True, exist_ok=True)
        verify_tools(tool)
        manifest = json.loads((runtime / "runtime-manifest.json").read_bytes())
        if manifest.get("files") != inventory(runtime, ("runtime-manifest.json",)):
            raise ValueError("RUNTIME_INTEGRITY_MISMATCH")
        config = manifest["runtime"]
        node = safe_path(runtime, config["node"])
        python = safe_path(runtime, config["python"])
        env["D2C_BROWSER_EXE"] = str(safe_path(runtime, config["browser"]))
        if args.action == "doctor":
            for command in ([python, "-B", "--version"], [node, "--version"],
                            [tool / "bin/uic.exe", "-v"], [compiler, "--help"]):
                run(command, env, work)
            run([node, "-e", "const {chromium}=require('playwright');"
                 "(async()=>{const b=await chromium.launch({executablePath:process.env.D2C_BROWSER_EXE,"
                 "headless:true});console.log(b.version());await b.close()})()"
                 ".catch(e=>{console.error(e);process.exit(1)})"], env, work)
            print("PASS: local controlled runtime; not clean-Windows or visual acceptance.")
            return
        if not all(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", x or "") for x in (args.batch, args.page)):
            raise ValueError("BATCH_AND_PAGE_REQUIRED")
        if args.action == "preview":
            if not args.snapshot:
                raise ValueError("SNAPSHOT_REQUIRED")
            output = safe_path(work, "output/work/preview/" + args.batch + "/" + args.page)
            run([python, "-B", tool / "scripts/PreviewStructure.py",
                 "--input", args.snapshot.resolve(strict=True),
                 "--style", (args.style or tool / "resources/initial-light.json"),
                 "--font", tool / "resources/font.ttf", "--output", output,
                 "--tool-root", tool, "--work-root", work], env, work)
            return
        if not args.capture:
            raise ValueError("CAPTURE_REQUIRED")
        destination = safe_path(work, "output/delivery/" + args.batch + "/" + args.page)
        if destination.exists():
            raise ValueError("DELIVERY_ALREADY_EXISTS")
        # Native formal contract requires a delivery-shaped path, even in staging.
        stage_root = safe_path(work, "output/work/page-stage-" + uuid.uuid4().hex)
        stage_root.mkdir()
        stage = stage_root / "output/delivery" / args.batch / args.page
        env["D2C_WORK_ROOT"] = str(stage_root)
        run([python, "-B", tool / "scripts/capture/capture_manifest.py", "verify",
             "--manifest", args.capture.resolve(strict=True), "--mode", "formal"], env, work)
        adapt = stage_root / "output/work/adapt"
        report = stage_root / "output/logs/adapt"
        run([node, tool / "scripts/tools/adapt_v2.cjs", adapt, report, args.capture.resolve()], env, work)
        mapping = (args.mapping or tool / "resources/mapping").resolve()
        command = [compiler, "--raw", adapt / "hybrid-result.json", "--mapping", mapping,
                   "--extensions", mapping / "extensions.json", "--font", tool / "resources/font.ttf",
                   "--output", stage, "--strict-layout", "--formal", "--tool-root", tool,
                   "--work-root", stage_root]
        if args.library_profile:
            command += ["--library-profile", args.library_profile]
        run(command, env, work)
        for suffix in (".ui", ".h", ".cpp", ".qss", ".qrc"):
            if not list(stage.rglob("*" + suffix)):
                raise ValueError("INCOMPLETE_DELIVERY: " + suffix)
        for ui in stage.rglob("*.ui"):
            subprocess.run([str(tool / "bin/uic.exe"), str(ui)], env=env, check=True, stdout=subprocess.PIPE)
        write_json(stage / "delivery-manifest.json", {
            "schemaVersion": 1, "mode": "formal", "checks": ["capture", "adapt", "compiler", "uic"],
            "captureManifestSha256": sha(args.capture), "toolManifestSha256": sha(tool / "tool-manifest.json"),
            "runtimeManifestSha256": sha(runtime / "runtime-manifest.json"),
            "mappingFiles": inventory(mapping), "files": inventory(stage),
            "visualApproval": False, "freshMachinePassed": False})
        destination.parent.mkdir(parents=True, exist_ok=True)
        stage.rename(destination)
        shutil.rmtree(stage_root)
        print("PASS: generated " + str(destination) + "; visual approval remains separate.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print("FAILED: " + str(error), file=sys.stderr)
        sys.exit(2)
