"""Create a Release-only offline candidate from explicit, approved local inputs.

No downloads, package installs, SDK rebuilds or global environment changes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import uuid
import zipfile

from BuildReceipt import ROOT, check, sha
from ArtifactLifecycle import inventory, operation_lock, verify_tools
from SDKContract import verify as verify_sdk
sys.path.insert(0, str(Path(__file__).parent / "offline"))
from offline import no_links, inside, verify


def copy(source, destination):
    no_links(source)
    if source.is_dir():
        for item in source.rglob("*"):
            no_links(item)
        shutil.copytree(source, destination, ignore=shutil.ignore_patterns(
            "__pycache__", "*.pyc", "site-packages", "node_modules/.cache"))
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def package(inputs_path, sdk, stage_only=False):
    if sys.version_info[:2] != (3, 9):
        raise ValueError("PYTHON39_REQUIRED")
    config = json.loads(inputs_path.read_bytes())
    required = () if stage_only else ("vcLicense",)
    missing = [key for key in required if not config.get(key) or not Path(config[key]).exists()]
    if missing:
        raise ValueError("RELEASE_INPUTS_MISSING: " + ", ".join(missing))
    if not stage_only and (config.get("redistributionReviewed") is not True or not config.get("reviewReference")):
        raise ValueError("REDISTRIBUTION_REVIEW_REQUIRED: review all licenses, including Qt/uic/VC/fonts")
    receipt = check(sdk, require_verified=not stage_only)
    verify_sdk(sdk, "Release")
    project = ROOT / "DesignToCode_GenerateLayout"
    tool = project / "build/tools/layout"
    gallery = ROOT / "ComponentLab/build/tools/gallery"
    verify_tools(tool)
    verify_tools(gallery)
    runtime = project / "build/runtime"
    runtime_manifest = json.loads((runtime / "runtime-manifest.json").read_bytes())
    if runtime_manifest["files"] != inventory(runtime, ("runtime-manifest.json",)):
        raise ValueError("RUNTIME_INTEGRITY_MISMATCH")
    sdk_manifest = json.loads((sdk / "manifest/component-sdk.json").read_bytes())
    release_id = sdk_manifest["releaseId"]
    # SDK release IDs become filenames; do not accept separators from an input.
    if any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-" for c in release_id):
        raise ValueError("UNSAFE_RELEASE_ID")
    output_root = project / "output/work/releases"
    no_links(output_root)
    output_root.mkdir(parents=True, exist_ok=True)
    stage = output_root / ("candidate-" + release_id + "-" + uuid.uuid4().hex[:8])
    stage.mkdir()
    paths = {key: Path(config[key]).resolve() for key in required}
    font_name = "HarmonyOS_Sans_SC_Regular.ttf"
    try:
        python = stage / "python"
        copy(runtime / "python", python)
        # Pin stdlib and our capture modules; no registry/global/site-package fallback.
        (python / "python39._pth").write_bytes(b".\nLib\nDLLs\n../scripts\n../scripts/capture\n")
        copy(runtime / "node/node.exe", stage / "node/node.exe")
        for name in ("playwright", "playwright-core"):
            copy(runtime / "node_modules" / name, stage / "node_modules" / name)
        playwright = json.loads((stage / "node_modules/playwright/package.json").read_bytes())
        core = json.loads((stage / "node_modules/playwright-core/package.json").read_bytes())
        if playwright["version"] != core["version"]:
            raise ValueError("PLAYWRIGHT_VERSION_MISMATCH")
        copy(runtime / "browser", stage / "browser")
        browser_relative = Path(runtime_manifest["runtime"]["browser"]).relative_to("browser").as_posix()
        browser_expected = next(p for p in json.loads(
            (stage / "node_modules/playwright-core/browsers.json").read_bytes())["browsers"]
                                if p["name"] == "chromium")
        env = dict(os.environ, NODE_PATH=str(stage / "node_modules"))
        credits_js = (
            "const {chromium}=require('playwright');"
            "(async()=>{const b=await chromium.launch({executablePath:process.argv[1],headless:true});"
            "if(b.version()!==process.argv[2])throw Error('BROWSER_VERSION_MISMATCH '+b.version());"
            "const p=await b.newPage();await p.goto('chrome://credits');"
            "process.stdout.write(await p.content());await b.close()})()"
            ".catch(e=>{console.error(e);process.exit(1)})")
        credits = subprocess.check_output([str(stage / "node/node.exe"), "-e", credits_js,
                                           str(stage / "browser" / browser_relative),
                                           browser_expected["browserVersion"]], env=env)
        if len(credits) < 10000:
            raise ValueError("BROWSER_LICENSE_INVENTORY_EMPTY")
        licenses = stage / "licenses"
        licenses.mkdir()
        (licenses / "Chromium-third-party.html").write_bytes(credits)
        copy(runtime / "licenses/Node-LICENSE.txt", licenses / "Node-LICENSE.txt")
        if config.get("vcLicense") and Path(config["vcLicense"]).is_file():
            copy(Path(config["vcLicense"]), licenses / "VC-runtime-LICENSE.txt")
        copy(sdk, stage / "sdk")
        copy(gallery, stage / "bin")
        copy(tool / "bin/v2_ui_compiler.exe",
             stage / "bin/v2_ui_compiler.exe")
        copy(tool / "bin/uic.exe", stage / "bin/uic.exe")
        # windeployqt selects the visible windows platform only. The compiler
        # deliberately uses offscreen, so ship that exact Qt installation's plugin too.
        copy(tool / "bin/platforms/qoffscreen.dll", stage / "bin/platforms/qoffscreen.dll")
        scripts = tool / "scripts"
        copy(scripts / "tools", stage / "scripts/tools")
        copy(scripts / "capture", stage / "scripts/capture")
        copy(Path(__file__).parent / "offline/offline.py", stage / "scripts/offline.py")
        for name in ("ArtifactLifecycle.py", "SDKContract.py"):
            copy(Path(__file__).parent / name, stage / "scripts" / name)
        copy(Path(__file__).parent / "offline/Run.cmd", stage / "Run.cmd")
        copy(tool / "resources/mapping", stage / "resources/mapping")
        mapping_file = stage / "resources/mapping/component-mapping.json"
        mapping = json.loads(mapping_file.read_bytes())
        mapping["profile"] = "wide-light"
        mapping_file.write_bytes(json.dumps(mapping, ensure_ascii=False, indent=2).encode("utf-8"))
        copy(tool / "resources/font.ttf", stage / "resources/fonts" / font_name)
        copy(ROOT / "DesignToCode_GenerateLayout/docs/examples/offline-capture", stage / "examples/capture")
        copy(tool / "resources/font.ttf", stage / "examples/resources" / font_name)
        copy(ROOT / "docs/tutorial/离线交付.md", stage / "README.md")
        copy(ROOT / "docs/designRules/architecture/工具与交付依赖.md", stage / "DEPENDENCIES.md")
        manifest = {
            "schemaVersion": 1, "releaseId": release_id, "configuration": "Release",
            "sdkManifestSha256": sha(sdk / "manifest/component-sdk.json"),
            "sourceFingerprint": sdk_manifest["sourceFingerprint"],
            "acceptance": {"developmentRegression": receipt["developmentRegressionPassed"],
                           "freshMachine": False, "relocation": False, "visual": False},
            "status": ("internal-test-only-not-for-distribution" if stage_only
                       else "candidate-awaiting-clean-Windows-acceptance"),
            "platform": "Windows 10 22H2 / Windows 11 x64",
            "thirdParty": {"playwright": playwright["version"],
                           "chromium": browser_expected, "redistributionReview": config.get("reviewReference")},
            "runtime": {"node": "node/node.exe", "browser": "browser/" + browser_relative,
                        "compiler": "bin/v2_ui_compiler.exe", "uic": "bin/uic.exe",
                        "gallery": "bin/component_lab.exe", "mapping": "resources/mapping",
                        "font": "resources/fonts/" + font_name},
            "files": [{"path": p.relative_to(stage).as_posix(), "sha256": sha(p), "bytes": p.stat().st_size}
                      for p in sorted(stage.rglob("*")) if p.is_file()],
        }
        (stage / "release.json").write_bytes(json.dumps(manifest, ensure_ascii=False, indent=2).encode("utf-8"))
        verify(stage)
        if stage_only:
            print(json.dumps({"internalStage": str(stage), "redistributionApproved": False,
                              "note": "No ZIP produced; this stage is for local engineering tests only."},
                             ensure_ascii=False))
            return
        # Never overwrite an existing release, even when SDK inputs are identical.
        archive = stage.parent / (stage.name + ".zip")
        with zipfile.ZipFile(str(archive), "x", zipfile.ZIP_DEFLATED) as package_zip:
            for path in sorted(stage.rglob("*")):
                if path.is_file():
                    package_zip.write(str(path), path.relative_to(stage).as_posix())
        destination = project / "output/delivery" / release_id
        with operation_lock(project):
            no_links(destination)
            if destination.exists():
                raise ValueError("DELIVERY_ALREADY_EXISTS: " + str(destination))
            destination.mkdir(parents=True)
            try:
                archive.rename(destination / "toolkit.zip")
            except BaseException:
                destination.rmdir()
                raise
        shutil.rmtree(stage)
        print(json.dumps({"zip": str(destination / "toolkit.zip"), "sha256": sha(destination / "toolkit.zip"),
                          "freshMachinePassed": False}, ensure_ascii=False))
    except Exception:
        print("Package not released. Incomplete staging retained for diagnosis: " + str(stage), file=sys.stderr)
        raise


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True, type=Path)
    parser.add_argument("--sdk", required=True, type=Path)
    parser.add_argument("--stage-only", action="store_true",
                        help="Local engineering staging only; never creates a redistributable ZIP")
    args = parser.parse_args()
    package(args.inputs.resolve(), args.sdk.resolve(), args.stage_only)
