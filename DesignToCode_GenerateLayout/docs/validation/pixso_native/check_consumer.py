"""Build/run an independent page consumer using explicit existing tools only."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from content_readback import compare_content

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from acquire import checked_work_path, new_output, write_json


def run(argv, environment, log):
    result = subprocess.run(argv, env=environment, cwd=str(log.parent), capture_output=True, timeout=180)
    log.write_bytes(result.stdout + result.stderr)
    if result.returncode:
        write_json(log.parent / "result.json", {
            "status": "FAIL", "failedLog": str(log),
            "scope": "independent consumer probe; no target-page acceptance"})
        raise RuntimeError("COMMAND_FAILED: " + str(log))
    return result.stdout


def page_hashes(root):
    return {file.relative_to(root).as_posix(): hashlib.sha256(file.read_bytes()).hexdigest()
            for file in sorted(root.rglob("*")) if file.is_file()}


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("page", "sdk", "qt", "cmake", "ninja", "vsdev", "work"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--page-name", required=True)
    parser.add_argument("--vector", type=Path)
    parser.add_argument("--generation-record", type=Path,
                        help="Snapshot only generated page and compare its bound content; no SDK probe controls")
    args = parser.parse_args()
    if sys.platform != "win32":
        raise RuntimeError("WINDOWS_CONSUMER_PROBE")
    ctypes.windll.kernel32.SetErrorMode(0x8003)
    for name in ("page", "sdk", "qt", "cmake", "ninja", "vsdev"):
        setattr(args, name, getattr(args, name).resolve(strict=True))
    # VS setup affects only the child environment captured here, not the host
    # process, global PATH, registry, or installed dependencies.
    vsdev = str(args.vsdev)
    if any(char in vsdev for char in '"&|<>%\r\n'):
        raise RuntimeError("UNSAFE_TOOL_PATH")
    env_output = subprocess.run(
        'cmd.exe /d /s /c ""' + vsdev + '" -no_logo -arch=x64 >nul && set"',
        capture_output=True, timeout=30, check=True).stdout.decode("mbcs")
    environment = {key.upper(): value for key, value in os.environ.items()}
    for line in env_output.splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            environment[key.upper()] = value
    sdk_manifest = args.sdk / "manifest/component-sdk.json"
    before = hashlib.sha256(sdk_manifest.read_bytes()).hexdigest()
    page_before = page_hashes(args.page)
    inputs = {"sdkManifestSha256": before, "pageHashes": page_before,
              "page": str(args.page), "pageName": args.page_name,
              "sdk": str(args.sdk), "qt": str(args.qt), "cmake": str(args.cmake),
              "ninja": str(args.ninja), "vsdev": str(args.vsdev)}
    if args.vector:
        args.vector = args.vector.resolve(strict=True)
        inputs["vector"] = {"path": str(args.vector),
                            "sha256": hashlib.sha256(args.vector.read_bytes()).hexdigest()}
    generation = None
    if args.generation_record:
        if args.vector:
            raise ValueError("PAGE_ONLY_AND_VECTOR_PROBE_ARE_SEPARATE")
        generation_bytes = args.generation_record.read_bytes()
        generation = json.loads(generation_bytes)
        if generation.get("status") != "STAGED":
            raise ValueError("STAGED_GENERATION_REQUIRED")
        if {item["path"]: item["sha256"] for item in generation["files"]} != page_before:
            raise ValueError("PAGE_GENERATION_HASH_MISMATCH")
        if generation["metadata"]["sdk"]["manifestSha256"] != before:
            raise ValueError("GENERATION_SDK_MISMATCH")
        inputs["generationRecordSha256"] = hashlib.sha256(generation_bytes).hexdigest()
    work = checked_work_path(args.work)
    if work.exists():
        marker = work / "inputs.json"
        if not marker.is_file() or json.loads(marker.read_text(encoding="utf-8")) != inputs:
            raise RuntimeError("CONSUMER_WORK_INPUT_MISMATCH")
    else:
        new_output(work)
        write_json(work / "inputs.json", inputs)
    write_json(work / "result.json", {
        "status": "RUNNING", "scope": "independent consumer probe",
        "note": "Previous PASS is invalidated before this run begins."})
    run([str(args.cmake), "-S", str(Path(__file__).parent / "consumer"),
         "-B", str(work / "compile"), "-G", "Ninja",
         "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_MAKE_PROGRAM=" + args.ninja.as_posix(),
         "-DPAGE_ROOT=" + args.page.as_posix(), "-DPAGE_NAME=" + args.page_name,
         "-DGeneratedComponentSDK_DIR=" + (args.sdk / "cmake").as_posix(),
         "-DQt5_DIR=" + (args.qt / "lib/cmake/Qt5").as_posix()], environment, work / "configure.log")
    run([str(args.cmake), "--build", str(work / "compile"), "--parallel", "2"],
        environment, work / "build.log")
    runtime = environment.copy()
    runtime["PATH"] = str(args.qt / "bin") + os.pathsep + runtime.get("PATH", "")
    runtime["QT_PLUGIN_PATH"] = str(args.qt / "plugins")
    runtime["QT_QPA_PLATFORM"] = "offscreen"
    command = [str(work / "compile/native_page_consumer.exe"), str(work / "page.png")]
    if generation:
        viewport = generation["metadata"]["viewport"]
        ui = ET.parse(args.page / "form" / (args.page_name + ".ui"))
        contract = {"width": viewport["width"], "height": viewport["height"],
                    "objects": [{"name": item.attrib["name"], "class": item.attrib["class"]}
                                for item in ui.iter("widget")]}
        write_json(work / "snapshot-contract.json", contract)
        command += ["--page-only", str(work / "snapshot-contract.json")]
    if args.vector:
        command += [str(args.vector), str(work / "vector.png")]
    output = run(command,
                 runtime, work / "runtime.log")
    after = hashlib.sha256(sdk_manifest.read_bytes()).hexdigest()
    if before != after:
        raise RuntimeError("SDK_CHANGED_DURING_CONSUMER_CHECK")
    if page_before != page_hashes(args.page):
        raise RuntimeError("CONSUMER_MODIFIED_PAGE_INPUT")
    if args.vector and inputs["vector"]["sha256"] != hashlib.sha256(args.vector.read_bytes()).hexdigest():
        raise RuntimeError("VECTOR_CHANGED_DURING_CONSUMER_CHECK")
    result = json.loads(output.decode("utf-8"))
    result["sdkManifestSha256"] = before
    result["pageFilesUnchanged"] = True
    result["toolchain"] = {"python": sys.version.split()[0], "qt": str(args.qt),
                           "cmake": str(args.cmake), "ninja": str(args.ninja),
                           "vsdev": str(args.vsdev)}
    result["scope"] = "independent consumer probe; target design acceptance remains separate"
    if generation:
        result["scope"] = "generated page only; no additional SDK probe controls"
        result["contentReadback"] = compare_content(generation["evidence"]["componentBindings"], result)
        if (result["width"], result["height"]) != (viewport["width"], viewport["height"]):
            result["status"] = "FAIL"
            result["viewportMismatch"] = True
        if result["contentReadback"]["status"] != "PASS":
            result["status"] = "FAIL"
    write_json(work / "result.json", result)
    print(json.dumps(result, ensure_ascii=True))
    if result["status"] != "PASS":
        raise RuntimeError("GENERATED_PAGE_CHECK_FAILED")


if __name__ == "__main__":
    main()
