"""Validate ComponentLab as a display-only consumer of GeneratedComponentSDK."""
import argparse
import hashlib
import json
import os
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
PROJECT = SOURCE.parent
SDK = Path(os.environ.get(
    "GENERATED_COMPONENT_SDK_ROOT",
    PROJECT.parent / "DesignToCode_GenerateComponent" / "build" / "sdk",
)).resolve()


def validation_inputs():
    paths = [p for folder in (SOURCE / "delivery", SOURCE / "presentation")
             for p in folder.rglob("*") if p.is_file() and "__pycache__" not in p.parts]
    paths.extend(p for p in SOURCE.iterdir() if p.is_file()
                 and (p.suffix in {".ps1", ".cmake"} or p.name == "CMakeLists.txt"))
    paths.append(SDK / "manifest/component-sdk.json")
    result = {}
    for path in sorted(set(paths)):
        key = (path.relative_to(PROJECT).as_posix() if path.is_relative_to(PROJECT)
               else "@generated-component-sdk/" + path.relative_to(SDK).as_posix())
        result[key] = hashlib.sha256(path.read_bytes()).hexdigest()
    return result


def check_baseline_sdk(manifest, dist):
    """Validate the published Qt-native baseline without assuming V2 Tokens.

    The baseline is intentionally a direct C++/QSS package.  Figma maps,
    palette JSON and the former ComponentFactory are legacy inputs, not a
    runtime dependency of a portable native-control consumer.
    """
    assert manifest["publicationState"] == "baseline-native"
    assert manifest["componentAuthority"]
    assert not manifest["tokenAuthority"]
    assert manifest["visualQssAuthority"]
    assert not manifest["runtimeTokenJson"]
    assert not manifest["legacyRuntime"]
    assert manifest["visualResourceDelivery"] == "static-qrc"
    assert manifest["linkage"] == "static"
    assert manifest["cmakeTarget"] == "GeneratedComponentSDK::Components"
    assert manifest["nativeComponentCount"] == 58
    assert manifest.get("compositeComponentCount") == 1
    for relative in (
        "include/MyDesignSystem/MyDesignSystem.h",
        "include/MyDesignSystem/ZNativeComponents.h",
        "include/MyDesignSystem/ZInputField.h",
        "lib/GeneratedComponentSDK.lib",
        "manifest/base-sdk-scope.json",
        "cmake/GeneratedComponentSDKConfig.cmake",
    ):
        assert (SDK / relative).is_file(), relative
    assert not (SDK / "resources").exists()
    assert not (SDK / "View").exists()
    assert not (SDK / "Form").exists()
    assert not list(SDK.rglob("*.exe")) and not list(SDK.rglob("*.dll"))
    scope = json.loads((SDK / manifest["nativeScope"]).read_bytes())
    native_classes = scope["approvedQtClasses"]
    assert len(native_classes) == manifest["nativeComponentCount"]
    assert all(item.startswith("Q") for item in native_classes)

    cmake = (SOURCE / "CMakeLists.txt").read_text(encoding="utf-8-sig")
    baseline_branch = cmake.split("if(COMPONENTLAB_BASELINE_SDK)", 1)[1].split(
        "return()", 1)[0]
    for forbidden in ("ComponentFactory", "TokenResources", "resources/mappings",
                      "resources/tokens", "figma-component-map"):
        assert forbidden not in baseline_branch, forbidden
    assert "GeneratedComponentSDK::Components" in baseline_branch
    assert "componentStyleSheet" in (SOURCE / "delivery/baseline/BaselineComponentLab.cpp").read_text(
        encoding="utf-8-sig")

    # A stable gallery can legitimately belong to the preceding legacy SDK
    # release until this baseline build is promoted.  Validate a package only
    # when the caller explicitly supplies its staged or released directory.
    demo = Path(dist).resolve() if dist else None
    if demo and demo.exists():
        for relative in ("component_lab.exe", "Run.cmd", "manifest/component-sdk.json"):
            assert (demo / relative).is_file(), relative
        assert not list(demo.rglob("*.lib")) and not list(demo.rglob("*.h"))
        assert not (demo / "resources").exists()
        assert not (demo / "TokenSets").exists()
    return {
        "status": "PASS",
        "sdkAuthority": str(SDK),
        "sdkMode": "direct-reference",
        "packageKind": "qt-native-baseline",
        "nativeComponentCount": manifest["nativeComponentCount"],
        "runtimeTokenJson": False,
        "sdkManifestSha256": hashlib.sha256(
            (SDK / "manifest/component-sdk.json").read_bytes()).hexdigest(),
        "sourceRoles": ["delivery/baseline"],
    }


def check(dist=None):
    # ComponentLab directly consumes the workspace authority and must never publish or cache an SDK copy.
    assert not (PROJECT / "build/sdk").exists(), "ComponentLab must not contain build/sdk"
    assert not (SOURCE / "Import-GeneratedComponentSDK.ps1").exists(), "Copy-based SDK import is retired"
    assert (SOURCE / "Verify-GeneratedComponentSDK.ps1").is_file()
    manifest = json.loads((SDK / "manifest/component-sdk.json").read_bytes())
    assert manifest["owner"] == "DesignToCode_GenerateComponent"
    if manifest.get("packageKind") == "qt-native-baseline":
        return check_baseline_sdk(manifest, dist)
    assert manifest["componentAuthority"] and manifest["tokenAuthority"]
    assert manifest["linkage"] in ("static", "shared")
    assert manifest["designComponentCatalog"] == "resources/mappings/design-component-catalog.json"
    catalog = json.loads((SDK / manifest["designComponentCatalog"]).read_bytes())
    assert catalog["figmaFileKey"] == "w92BKW1EcutKmTXkwzNHbf"
    entries = catalog["entries"]
    components = catalog["components"]
    statistics = catalog["statistics"]
    assert catalog["canonicalLibraryEntryCount"] == len(entries)
    assert catalog["componentCount"] == len(components)
    assert statistics["componentIdentityCount"] == len(components)
    assert len(entries) == (
        len(components) + statistics["layoutHelperCount"] + statistics["assetEntryCount"]
    )
    assert catalog["instantiableCount"] == sum(item["instantiable"] for item in components)
    assert catalog["contractOnlyCount"] == sum(not item["instantiable"] for item in components)
    assert catalog["figmaSource"]["pageCount"] == len(catalog["figmaPages"])
    assert catalog["figmaSource"]["componentSetCount"] == sum(
        len(page["componentSets"]) for page in catalog["figmaPages"]
    )
    assert catalog["figmaSource"]["variantCount"] == sum(
        len(component_set["variants"])
        for page in catalog["figmaPages"]
        for component_set in page["componentSets"]
    )
    assert (SDK / "include/MyDesignSystem/MyDesignSystem.h").is_file()
    if os.name == "nt":
        assert (SDK / "lib/GeneratedComponentSDK.lib").is_file()
    else:
        assert (SDK / "lib/libGeneratedComponentSDK.so").is_file()
    assert (SDK / "resources/tokens/default").is_dir()
    assert not list(SDK.rglob("*.exe")) and not list(SDK.rglob("*.dll"))
    demo = Path(dist).resolve() if dist else PROJECT / "build/tools/gallery"
    if demo.exists():
        assert not list(demo.rglob("*.lib")) and not list(demo.rglob("*.h"))
        # The gallery consumes the packaged V2 map first.  If a demo package is
        # present, fail fast instead of allowing a stale map to masquerade as a
        # current SDK build.
        sdk_map = SDK / "resources/mappings/figma-component-map-v2.json"
        demo_map = demo / "resources/mappings/figma-component-map-v2.json"
        if demo_map.exists():
            assert hashlib.sha256(demo_map.read_bytes()).digest() == hashlib.sha256(
                sdk_map.read_bytes()).digest(), "demo V2 component map is stale"
    v2_map = json.loads((SDK / "resources/mappings/figma-component-map-v2.json").read_bytes())
    assert v2_map["schemaVersion"] == "2.0"
    button = next(item for item in v2_map["components"] if item["componentId"] == "button")
    primary = next(item for item in button["bindings"] if item["variable"] ==
                   "button/base/intent/primary/slot/container/background-color")
    # Regression guard for the issue where ComponentLab displayed an old token map.
    assert primary["canonicalPath"] == "component.button.base.intent.primary.slot.container.background-color"
    assert primary["canonicalValue"] == "#6235F7"
    assert not (PROJECT / "build/dist").exists()
    assert not (PROJECT / "build/dependencies").exists()
    assert not (SOURCE / "runtime").exists()
    assert not (SOURCE / "design-assets").exists()
    names = {p.name for p in SOURCE.rglob("*") if p.is_file()}
    for forbidden in ("Z_PushButton.cpp", "Z_CheckBox.cpp", "Z_InputField.cpp",
                      "Z_Select.cpp", "token_compiler.cpp", "export_tokens.py"):
        assert forbidden not in names, forbidden
    cmake = (SOURCE / "CMakeLists.txt").read_text(encoding="utf-8-sig")
    assert "GeneratedComponentSDK::Components" in cmake
    assert "COMPONENTLAB_BUILD_GENERATORS" not in cmake
    return {"status": "PASS", "sdkAuthority": str(SDK), "sdkMode": "direct-reference",
        "sdkManifestSha256": hashlib.sha256(
        (SDK / "manifest/component-sdk.json").read_bytes()).hexdigest(),
        "sourceRoles": ["delivery", "presentation"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--print-inputs", action="store_true")
    parser.add_argument("--dist", type=Path, help="Explicit staged or stable gallery to inspect")
    args = parser.parse_args()
    if args.print_inputs:
        print(json.dumps(validation_inputs(), sort_keys=True))
        return
    result = check(args.dist)
    if args.snapshot:
        allowed = PROJECT / "output/logs"
        if not args.snapshot.resolve().is_relative_to(allowed):
            raise ValueError("SNAPSHOT_MUST_BE_IN_OUTPUT_LOGS")
        args.snapshot.parent.mkdir(parents=True, exist_ok=True)
        args.snapshot.write_text(json.dumps(validation_inputs(), indent=2, sort_keys=True),
                                 encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
