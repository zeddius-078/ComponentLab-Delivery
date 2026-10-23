"""Published SDK verification shared by producers, consumers and runtime tools.

Python 3.9, standard library only. This module never consults producer sources,
repairs a package, installs dependencies, or writes into the selected SDK.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import stat
import sys


def reject_links(path):
    path = Path(path).absolute()
    for item in (path, *path.parents):
        if item.is_symlink() or (item.exists() and
                getattr(item.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
            raise ValueError("PATH_IS_LINK: " + str(item))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def verify_native_baseline(manifest, names):
    """Validate the deliberately small, Qt-native-only SDK contract.

    A baseline package is not a cut-down copy of the historical component
    runtime.  In particular it must not carry runtime Token JSON, Figma maps,
    ComponentFactory, or the component catalog.  Its visual authority is the
    package-owned View QSS resource, while token data remains a build-time
    concern of the producer.
    """
    if manifest.get("tokenAuthority") is not False or not manifest.get("visualQssAuthority"):
        raise ValueError("SDK_BASELINE_AUTHORITY_INVALID")
    if manifest.get("runtimeTokenJson") is not False or manifest.get("legacyRuntime") is not False:
        raise ValueError("SDK_BASELINE_RUNTIME_BOUNDARY_INVALID")
    scope_path = manifest.get("nativeScope", "")
    required = {
        "include/MyDesignSystem/MyDesignSystem.h",
        "include/MyDesignSystem/ZNativeComponents.h",
        "cmake/GeneratedComponentSDKConfig.cmake",
        "cmake/GeneratedComponentSDKConfigVersion.cmake",
        "View/z_native_components.qrc",
        "View/ZNativeComponents.qss",
        "View/fonts/HarmonyOS_Sans.ttf",
        "View/fonts/HarmonyOS_Sans_SC.ttf",
        "View/fonts/LICENSE.txt",
        "manifest/component-package.json",
        scope_path,
    }
    if not required.issubset(set(names)):
        raise ValueError("SDK_BASELINE_REQUIRED_FILE_MISSING")
    if any(name.startswith("resources/tokens/") or name.startswith("resources/mappings/")
           for name in names):
        raise ValueError("SDK_BASELINE_CONTAINS_RUNTIME_MIDDLEWARE")
    if any(Path(name).suffix.lower() == ".json" and name != scope_path and
           name not in ("manifest/component-sdk.json", "manifest/component-package.json")
           for name in names):
        raise ValueError("SDK_BASELINE_CONTAINS_UNDECLARED_JSON")
    scope_file = Path(scope_path)
    try:
        scope = json.loads((Path(manifest["_sdkRoot"]) / scope_file).read_text(encoding="utf-8-sig"))
    except (KeyError, OSError, json.JSONDecodeError) as error:
        raise ValueError("SDK_BASELINE_SCOPE_INVALID") from error
    approved = scope.get("approvedQtClasses", [])
    if (len(approved) != scope.get("validation", {}).get("expectedNativeComponentCount") or
            manifest.get("nativeComponentCount") != len(approved) or
            manifest.get("scopeHash") != sha(Path(manifest["_sdkRoot"]) / scope_file)):
        raise ValueError("SDK_BASELINE_SCOPE_MISMATCH")
    return required


def verify(sdk, configuration="Release"):
    sdk = Path(sdk).absolute()
    reject_links(sdk)
    manifest = json.loads((sdk / "manifest/component-sdk.json").read_text(encoding="utf-8-sig"))
    # Internal verification context only; it is removed before returning so it
    # can never be mistaken for a published manifest field.
    manifest["_sdkRoot"] = str(sdk)
    if manifest.get("schemaVersion") != 2:
        raise ValueError("SDK_SCHEMA_UNSUPPORTED")
    if (manifest.get("package"), manifest.get("owner")) != (
            "GeneratedComponentSDK", "DesignToCode_GenerateComponent"):
        raise ValueError("SDK_IDENTITY_INVALID")
    if not manifest.get("componentAuthority"):
        raise ValueError("SDK_AUTHORITY_INVALID")
    if manifest.get("linkage") not in ("static", "shared"):
        raise ValueError("SDK_LINKAGE_INVALID")
    linux_shared = manifest.get("linkage") == "shared"
    tools = manifest.get("toolchain", {})
    if linux_shared:
        expected = (("configuration", configuration), ("architecture", "x86_64"),
                    ("crt", "gnu"), ("compilerId", "GNU"))
    else:
        expected = (("configuration", configuration), ("architecture", "x64"),
                    ("crt", "MDd" if configuration == "Debug" else "MD"),
                    ("compilerId", "MSVC"))
    for key, value in expected:
        if tools.get(key) != value:
            raise ValueError("SDK_ABI_MISMATCH: " + key)
    for key, prefix in (("pythonVersion", "3.9."), ("qtVersion", "5.15.")):
        if not str(tools.get(key, "")).startswith(prefix):
            raise ValueError("SDK_TOOLCHAIN_VERSION_INVALID: " + key)
    for key in ("cmakeVersion", "compilerVersion"):
        if not tools.get(key):
            raise ValueError("SDK_TOOLCHAIN_METADATA_MISSING: " + key)
    if not re.fullmatch(r"\d+\.\d+\.\d+\.\d+", manifest.get("packageVersion", "")):
        raise ValueError("SDK_VERSION_INVALID")
    declared = manifest.get("files", [])
    names = [entry["path"] for entry in declared]
    if len(set(name.casefold() for name in names)) != len(names):
        raise ValueError("SDK_DUPLICATE_FILE")
    actual = []
    for path in sorted(sdk.rglob("*")):
        reject_links(path)
        relative = path.relative_to(sdk).as_posix()
        if not path.is_file() or relative == "manifest/component-sdk.json":
            continue
        actual.append({"path": relative, "sha256": sha(path), "bytes": path.stat().st_size})
    normalized = [{**item, "sha256": item["sha256"].lower()} for item in declared]
    if sorted(actual, key=lambda item: item["path"]) != sorted(normalized, key=lambda item: item["path"]):
        raise ValueError("SDK_FILE_INTEGRITY_MISMATCH")
    sdk_library = "lib/libGeneratedComponentSDK.so" if linux_shared else "lib/GeneratedComponentSDK.lib"
    required = {sdk_library, "include/MyDesignSystem/MyDesignSystem.h",
                "cmake/GeneratedComponentSDKConfig.cmake", "cmake/GeneratedComponentSDKConfigVersion.cmake"}
    if manifest.get("packageKind") == "qt-native-baseline":
        required.update(verify_native_baseline(manifest, names))
    else:
        if not manifest.get("tokenAuthority"):
            raise ValueError("SDK_AUTHORITY_INVALID")
        for key in ("componentMap", "nativeCapabilityCatalog", "componentSourceMap", "designComponentCatalog",
                    "v2ComponentMap", "v2ComponentRegistry", "tokenManifest"):
            required.add(manifest.get(key, ""))
        if not any(name.startswith("licenses/") for name in names):
            raise ValueError("SDK_REQUIRED_FILE_MISSING")
    if not required.issubset(set(names)):
        raise ValueError("SDK_REQUIRED_FILE_MISSING")
    if any(Path(name).suffix.lower() in (".exe", ".dll") for name in names):
        raise ValueError("SDK_CONTAINS_RUNTIME_EXECUTABLE")
    files = manifest.get("sourceFiles")
    if not files or hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode(
            "utf-8")).hexdigest() != manifest.get("sourceFingerprint"):
        raise ValueError("SDK_SOURCE_FINGERPRINT_INVALID")
    manifest.pop("_sdkRoot", None)
    return manifest


if __name__ == "__main__":
    if sys.version_info[:2] != (3, 9):
        raise SystemExit("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["verify"])
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--configuration", choices=["Release", "Debug"], default="Release")
    args = parser.parse_args()
    result = verify(args.sdk, args.configuration)
    print(json.dumps({"status": "PASS", "sdkRoot": str(args.sdk.resolve()),
                      "releaseId": result["releaseId"],
                      "manifestSha256": sha(args.sdk / "manifest/component-sdk.json")}))
