"""Import explicitly approved, already present runtime materials; never install or fetch."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import uuid

from ArtifactLifecycle import inventory, operation_lock, safe_path, write_json, switch_directory
from SDKContract import reject_links, sha
from AgentCapabilityProfile import binding as execution_capability_binding

ROOT = Path(__file__).resolve().parent.parent


def copy(source, target):
    reject_links(source)
    if source.is_dir():
        inventory(source)
        shutil.copytree(source, target, ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "site-packages"))
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def promote(project, stage, relative, name, manifest_name):
    destination = safe_path(project, relative)
    with operation_lock(project, allow_reentrant=True):
        expected = json.loads((stage / manifest_name).read_bytes())["files"]
        if expected != inventory(stage, (manifest_name,)):
            raise ValueError("STAGE_INTEGRITY_MISMATCH")
        def validate(path):
            saved = json.loads((path / manifest_name).read_bytes())
            if saved["files"] != inventory(path, (manifest_name,)):
                raise ValueError("PROMOTED_INTEGRITY_MISMATCH")
        switch_directory(project, stage, destination, name, validate)
    return destination


def support(project):
    stage = safe_path(project, "output/work/stage/support-" + uuid.uuid4().hex)
    stage.mkdir(parents=True)
    for name in ("SDKContract.py", "ArtifactLifecycle.py", "AgentCapabilityProfile.py", "OperationLock.ps1",
                 "RuntimeRun.py", "offline/LayoutRun.cmd"):
        copy(ROOT / "Source" / name, stage / name)
    write_json(stage / "support-manifest.json", {"schemaVersion": 1, "files": inventory(stage)})
    return promote(project, stage, "build/tools/support", "support", "support-manifest.json")


def runtime(project, inputs):
    config = json.loads(inputs.read_bytes())
    keys = ("pythonRoot", "nodeExe", "nodeLicense", "nodeModules", "browserRoot", "browserExe")
    missing = [k for k in keys if not config.get(k) or not Path(config[k]).exists()]
    if missing:
        raise ValueError("EXISTING_RUNTIME_INPUTS_MISSING: " + ", ".join(missing))
    paths = {k: Path(config[k]).absolute() for k in keys}
    version = subprocess.check_output([str(paths["pythonRoot"] / "python.exe"), "-B", "-c",
                                       "import sys;print('%d.%d'%sys.version_info[:2])"], text=True).strip()
    if version != "3.9":
        raise ValueError("PYTHON39_REQUIRED")
    stage = safe_path(project, "output/work/stage/runtime-" + uuid.uuid4().hex)
    stage.mkdir(parents=True)
    for name in ("python.exe", "python39.dll", "python3.dll", "vcruntime140.dll",
                 "vcruntime140_1.dll", "LICENSE.txt", "DLLs", "Lib"):
        copy(paths["pythonRoot"] / name, stage / "python" / name)
    (stage / "python/python39._pth").write_bytes(
        b".\nLib\nDLLs\n../../tools/layout/scripts\n../../tools/layout/scripts/capture\n../../tools/support\n")
    copy(paths["nodeExe"], stage / "node/node.exe")
    copy(paths["nodeLicense"], stage / "licenses/Node-LICENSE.txt")
    for module in ("playwright", "playwright-core"):
        copy(paths["nodeModules"] / module, stage / "node_modules" / module)
    copy(paths["browserRoot"], stage / "browser")
    relative_browser = paths["browserExe"].resolve().relative_to(paths["browserRoot"].resolve()).as_posix()
    playwright = json.loads((stage / "node_modules/playwright/package.json").read_bytes())
    core = json.loads((stage / "node_modules/playwright-core/package.json").read_bytes())
    if playwright["version"] != core["version"]:
        raise ValueError("PLAYWRIGHT_VERSION_MISMATCH")
    browser = next(p for p in json.loads((stage / "node_modules/playwright-core/browsers.json").read_bytes())["browsers"]
                   if p["name"] == "chromium")
    probe = (
        "const{chromium}=require('playwright');(async()=>{"
        "const b=await chromium.launch({executablePath:process.argv[1],headless:true});"
        "if(b.version()!==process.argv[2])throw Error('BROWSER_VERSION_MISMATCH '+b.version());"
        "const p=await b.newPage();await p.goto('chrome://credits');"
        "process.stdout.write(await p.content());await b.close()})()"
        ".catch(e=>{console.error(e);process.exit(1)})")
    env = dict(os.environ, NODE_PATH=str(stage / "node_modules"), PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD="1")
    credits = subprocess.check_output([str(stage / "node/node.exe"), "-e", probe,
                                       str(stage / "browser" / relative_browser), browser["browserVersion"]],
                                      env=env, timeout=60)
    if len(credits) < 10000:
        raise ValueError("CHROMIUM_LICENSES_MISSING")
    (stage / "licenses/Chromium-third-party.html").write_bytes(credits)
    # Preserve review status, not a new redistribution approval.
    (stage / "licenses/input-provenance.json").write_bytes(inputs.read_bytes())
    write_json(stage / "runtime-manifest.json", {
        "schemaVersion": 1, "kind": "controlled-runtime", "inputManifestSha256": sha(inputs),
        "executionCapabilityProfile": execution_capability_binding(),
        "redistributionReviewed": config.get("redistributionReviewed") is True,
        "reviewReference": config.get("reviewReference"), "playwright": playwright["version"],
        "chromium": browser, "runtime": {"python": "python/python.exe", "node": "node/node.exe",
                                        "browser": "browser/" + relative_browser},
        "files": inventory(stage)})
    return promote(project, stage, "build/runtime", "runtime", "runtime-manifest.json")


if __name__ == "__main__":
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("support", "runtime"))
    parser.add_argument("--project", required=True, type=Path)
    parser.add_argument("--inputs", type=Path)
    args = parser.parse_args()
    if args.action == "runtime" and not args.inputs:
        parser.error("--inputs is required for runtime")
    with operation_lock(args.project):
        print(support(args.project) if args.action == "support" else runtime(args.project, args.inputs))
