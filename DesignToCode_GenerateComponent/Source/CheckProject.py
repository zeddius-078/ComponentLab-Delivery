"""Check local documentation, resources and three-project dependency boundaries."""
import json
import argparse
import hashlib
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET
from SDKPackage import source_inventory, verify

PROJECT = Path(__file__).resolve().parent.parent
SOURCE = PROJECT / "Source"
WORK_SOURCE = PROJECT / "output/work/generated/Source"
NAMES = {"ComponentLab", "DesignToCode_GenerateComponent", "DesignToCode_GenerateLayout"}
OTHER = NAMES - {PROJECT.name}
REPORT = PROJECT / "output/logs/project-check.json"


def check_source():
    """Clean-checkout gate: maintained inputs only, no SDK or generated files."""
    files = source_inventory(PROJECT)
    for entry in PROJECT.iterdir():
        if entry.is_dir() and not entry.name.startswith("待删除-"):
            assert entry.name in {"Source", "docs", "build", "output"}, (entry, "unexpected root directory")
    for entry in SOURCE.rglob("*"):
        assert entry.name not in {"generated", "__pycache__"}, (entry, "generated files in Source")
        assert entry.suffix not in {".pyc", ".obj", ".exe", ".dll", ".lib", ".pdb"}, entry
        if entry.is_file() and entry.suffix == ".json":
            json.loads(entry.read_text(encoding="utf-8-sig"))
    native = json.loads((SOURCE / "ComponentKit/component-definitions/native-components.json").read_text(encoding="utf-8"))["components"]
    assert native
    for field in ("qtClass", "sdkClass"):
        assert len({item[field] for item in native}) == len(native), field
    for item in native:
        assert item["sdkClass"] == "Z_" + item["qtClass"][1:], item
    for path in SOURCE.rglob("*"):
        if path.is_file() and path.suffix in {".py", ".ps1", ".cpp", ".h", ".cmake", ".qrc"}:
            text = path.read_text(encoding="utf-8-sig").replace("\\", "/")
            for sibling in OTHER:
                assert "/" + sibling + "/" not in text, (path, "sibling source dependency")
    return {"project": PROJECT.name, "mode": "source-only", "status": "PASS",
            "maintainedInputs": len(files), "nativeComponentContracts": len(native),
            "sdkRequired": False}


def check():
    check_source()
    result = {"project": PROJECT.name, "localLinks": 0, "resourceFiles": 0}
    result["pendingDeletion"] = []
    for entry in PROJECT.iterdir():
        if entry.is_dir() and entry.name.startswith("待删除-"):
            result["pendingDeletion"].append(entry.name)
        elif entry.is_dir():
            assert entry.name in {"Source", "docs", "build", "output"}, (entry, "unexpected root directory")
    for entry in SOURCE.rglob("*"):
        assert entry.name not in {"generated", "__pycache__"}, (entry, "generated files in Source")
        assert entry.suffix not in {".pyc", ".obj", ".exe", ".dll", ".lib", ".pdb"}, (entry, "build artifact in Source")
    # The component Token chain is an owned production path and must survive directory cleanup.
    token_chain = [
        SOURCE / "ComponentKit/design-assets/tokens/checkbox-contract.json",
        SOURCE / "ComponentKit/design-assets/tools/export_tokens.py",
        SOURCE / "ComponentKit/design-assets/tools/checkbox_token_compiler.cpp",
        WORK_SOURCE / "ComponentKit/design-assets/generated",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/primitive.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/profile.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/semantic.json",
        WORK_SOURCE / "ComponentKit/design-assets/DesignSystem/generated/component.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/component-recipes",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/product/application-layout.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/dictionary.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/source/component-registry.json",
        WORK_SOURCE / "ComponentKit/design-assets/DesignSystem/config/figma-component-map-v2.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/config/figma-audit-v2.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/config/reference-mapping.json",
        WORK_SOURCE / "ComponentKit/design-assets/DesignSystem/config/manifest.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/schema/v2-token-schema.json",
        SOURCE / "ComponentKit/design-assets/DesignSystem/scripts/v2/compile.py",
    ]
    for path in token_chain:
        assert path.exists(), ("missing component Token chain input", path)
    native_contract = json.loads((
        SOURCE / "ComponentKit/component-definitions/native-components.json"
    ).read_text(encoding="utf-8"))
    native_components = native_contract["components"]
    assert native_components, "native component registry must not be empty"
    assert len({item["qtClass"] for item in native_components}) == len(native_components)
    assert len({item["sdkClass"] for item in native_components}) == len(native_components)
    source_map = json.loads((
        WORK_SOURCE / "ComponentKit/component-definitions/generated-component-source-map.json"
    ).read_text(encoding="utf-8"))
    assert len(source_map["components"]) == len(native_components)
    design_catalog = json.loads((
        WORK_SOURCE / "ComponentKit/component-definitions/generated-design-component-catalog.json"
    ).read_text(encoding="utf-8"))
    assert design_catalog["figmaFileKey"] == "w92BKW1EcutKmTXkwzNHbf"
    entries = design_catalog["entries"]
    components = design_catalog["components"]
    statistics = design_catalog["statistics"]
    assert design_catalog["canonicalLibraryEntryCount"] == len(entries)
    assert design_catalog["componentCount"] == len(components)
    assert statistics["componentIdentityCount"] == len(components)
    assert statistics["canonicalLibraryEntryCount"] == len(entries)
    assert statistics["widgetCount"] + statistics["compositeCount"] == len(components)
    assert len(entries) == (
        len(components) + statistics["layoutHelperCount"] + statistics["assetEntryCount"]
    )
    assert design_catalog["instantiableCount"] == sum(
        item["instantiable"] for item in components
    )
    assert design_catalog["contractOnlyCount"] == sum(
        not item["instantiable"] for item in components
    )
    assert statistics["registeredRuntimeComponentCount"] == sum(
        item["engineeringStatus"] == "REGISTERED" for item in entries
    )
    assert statistics["engineeringGapCount"] == sum(
        item["engineeringStatus"] == "UNMAPPED" for item in entries
    )
    assert design_catalog["figmaSource"]["pageCount"] == len(design_catalog["figmaPages"])
    assert design_catalog["figmaSource"]["componentSetCount"] == sum(
        len(page["componentSets"]) for page in design_catalog["figmaPages"]
    )
    assert design_catalog["figmaSource"]["variantCount"] == sum(
        len(component_set["variants"])
        for page in design_catalog["figmaPages"]
        for component_set in page["componentSets"]
    )
    component_set_governance = design_catalog["componentSetGovernance"]
    canonical_set_ids = {
        component_set_id
        for entry in entries
        for component_set_id in entry["componentSetId"].split(",")
        if component_set_id
    }
    classified_set_ids = {
        item["componentSetId"]
        for item in component_set_governance["unreferencedSets"]
    }
    inventory_set_ids = {
        component_set["id"]
        for page in design_catalog["figmaPages"]
        for component_set in page["componentSets"]
    }
    assert not canonical_set_ids & classified_set_ids
    assert inventory_set_ids == canonical_set_ids | classified_set_ids
    assert statistics["figmaComponentSetCount"] == len(inventory_set_ids)
    assert statistics["canonicalReferencedComponentSetCount"] == len(canonical_set_ids)
    assert sum(
        statistics[key]
        for key in (
            "internalComponentSetCount",
            "proposalComponentSetCount",
            "residueComponentSetCount",
        )
    ) == len(classified_set_ids)
    assert len({item["canonicalId"] for item in entries}) == len(entries)
    sdk_classes = [item["sdkClass"] for item in components if item["sdkClass"]]
    assert len(set(sdk_classes)) == len(sdk_classes)
    native_source = WORK_SOURCE / "ComponentKit/runtime/component/native"
    native_styles = WORK_SOURCE / "ComponentKit/design-assets/generated/native"
    for component in native_components:
        qt_class = component["qtClass"]
        sdk_class = component["sdkClass"]
        assert sdk_class == "Z_" + qt_class[1:], (qt_class, sdk_class)
        if component["implementation"] == "generated":
            assert (native_source / f"{sdk_class}.h").is_file(), sdk_class
            assert (native_source / f"{sdk_class}.cpp").is_file(), sdk_class
        else:
            assert (SOURCE / f"ComponentKit/runtime/component/{sdk_class}.h").is_file(), sdk_class
            assert (SOURCE / f"ComponentKit/runtime/component/{sdk_class}.cpp").is_file(), sdk_class
        for profile in ("standard-light", "wide-light", "standard-dark", "wide-dark"):
            assert (native_styles / profile / f"{sdk_class}.qss").is_file(), (profile, sdk_class)
    sdk = PROJECT / "build/sdk"
    # Exactly one current SDK authority; build/.state holds one recovery version.
    # `sdk-debug` is a local, non-published Debug CRT companion used by the
    # consumer tests.  It must not be mistaken for a second published SDK;
    # only the exact `build/sdk` directory is the delivery artifact.
    sdk_outputs = [path for path in (PROJECT / "build").iterdir()
                   if path.is_dir() and path.name.lower() == "sdk"]
    assert sdk_outputs == [sdk], ("unexpected SDK output", sdk_outputs)
    manifest = verify(sdk, configuration="Release", project=PROJECT)
    assert manifest["owner"] == PROJECT.name
    assert manifest["tokenAuthority"] and manifest["componentAuthority"]
    assert manifest["designComponentCatalog"] == "resources/mappings/design-component-catalog.json"
    assert (sdk / "include/MyDesignSystem/MyDesignSystem.h").is_file()
    assert (sdk / "lib/GeneratedComponentSDK.lib").is_file()
    assert (sdk / "resources/tokens/default").is_dir()
    published_catalog = json.loads((
        sdk / manifest["designComponentCatalog"]
    ).read_text(encoding="utf-8"))
    assert published_catalog == design_catalog
    # ComponentLab Debug consumes the local sdk-debug companion.  Its V2 map
    # must be byte-identical to the published map, otherwise the gallery can
    # display stale bindings while the Release package is already corrected.
    v2_map = sdk / "resources/mappings/figma-component-map-v2.json"
    source_v2_map = WORK_SOURCE / "ComponentKit/design-assets/DesignSystem/config/figma-component-map-v2.json"
    expected_hash = hashlib.sha256(source_v2_map.read_bytes()).hexdigest()
    assert hashlib.sha256(v2_map.read_bytes()).hexdigest() == expected_hash
    v2_document = json.loads(v2_map.read_text(encoding="utf-8"))
    button = next(item for item in v2_document["components"] if item["componentId"] == "button")
    button_padding = [
        item for item in button["bindings"]
        if item["variable"].endswith("slot/container/padding-block")
    ]
    assert len(button_padding) == 3
    assert all(item["value"] == "semantic.layout.padding.block.none"
               and item["status"] == "specified" for item in button_padding)
    debug_sdk = PROJECT / "build/sdk-debug"
    if debug_sdk.exists():
        try:
            verify(debug_sdk, configuration="Debug", project=PROJECT)
            result["debugCompanion"] = "verified"
        except (ValueError, OSError, KeyError) as error:
            # An independently built Debug package cannot be repaired by copying
            # two Release JSON files. Release acceptance does not certify Debug.
            result["debugCompanion"] = "unverified: " + str(error)
    assert not list(sdk.rglob("*.exe")) and not list(sdk.rglob("*.dll"))
    assert not (PROJECT / "build/dist").exists()
    consumer_test = PROJECT / "docs/validation/SDKConsumerTest"
    assert (consumer_test / "main.cpp").is_file()
    assert "#include <MyDesignSystem/MyDesignSystem.h>" in (consumer_test / "main.cpp").read_text(encoding="utf-8")
    assert "GeneratedComponentSDK::Components" in (consumer_test / "CMakeLists.txt").read_text(encoding="utf-8")
    for demo in (PROJECT / "output/delivery").glob("mapping-workbench-*"):
        assert not list(demo.rglob("*.lib")) and not list(demo.rglob("*.h"))
        assert not (demo / "cmake").exists()
    # Directory junctions would silently restore a shared dependency after a physical split.
    for file in SOURCE.rglob("*"):
        assert not file.is_symlink() and not getattr(file, "is_junction", lambda: False)(), file
        if not file.is_file() or file == Path(__file__):
            continue
        if file.suffix in {".cpp", ".h", ".ps1", ".py", ".cjs", ".cmd", ".qrc"} or file.name == "CMakeLists.txt":
            code = file.read_text(encoding="utf-8-sig").replace("\\", "/")
            # Names in design evidence are valid. Only file-path references are dependencies.
            for name in OTHER:
                assert "/" + name + "/" not in code, (file, "sibling project path", name)
            assert "/ComponentLab-Delivery/DesignToCode/" not in code, (file, "retired path")
    # QRC inputs must resolve inside this project's maintained source.
    for qrc in WORK_SOURCE.rglob("*.qrc"):
        for entry in ET.parse(qrc).findall(".//file"):
            target = (qrc.parent / entry.text).resolve()
            assert target.is_relative_to(WORK_SOURCE) and target.is_file(), (qrc, target)
            result["resourceFiles"] += 1
    # Navigation must remain usable after copying just this project.
    for file in [PROJECT / "README.md", *(PROJECT / "docs").rglob("*.md"), *SOURCE.rglob("*.md")]:
        if file.is_relative_to(PROJECT / "docs/archive"):
            continue  # Historical paths are retained as evidence, not current navigation.
        content = file.read_text(encoding="utf-8-sig")
        assert len(re.findall(r"^\x60\x60\x60", content, re.M)) % 2 == 0, file
        for link in re.findall(r"\]\(([^)]+)\)", content):
            link = link.strip("<> ").split("#")[0]
            if not link or re.match(r"^https?://", link):
                continue
            target = (file.parent / link).resolve()
            if target.is_relative_to(PROJECT / "build") or target.is_relative_to(PROJECT / "output"):
                continue  # Optional generated links need not exist in a clean checkout.
            assert target.is_relative_to(PROJECT) and target.exists(), (file, link)
            result["localLinks"] += 1
    # Inspect only active build descriptions; before-project-split stores explicitly retired caches.
    cache = PROJECT / "output/work/cache"
    for ninja in cache.rglob("build.ninja"):
        if "before-project-split" in ninja.relative_to(cache).parts:
            continue
        content = ninja.read_text(encoding="utf-8").replace("\\", "/")
        for name in OTHER:
            assert "/ComponentLab-Delivery/" + name + "/" not in content, (ninja, name)
        assert "/ComponentLab-Delivery/DesignToCode/" not in content, (ninja, "retired path")
    result["tokenChainFiles"] = len(token_chain)
    result["nativeComponentContracts"] = len(native_components)
    result["canonicalLibraryEntries"] = design_catalog["canonicalLibraryEntryCount"]
    result["componentIdentities"] = design_catalog["componentCount"]
    result["registeredRuntimeComponents"] = statistics["registeredRuntimeComponentCount"]
    result["layoutHelpers"] = statistics["layoutHelperCount"]
    result["assetEntries"] = statistics["assetEntryCount"]
    result["engineeringGaps"] = statistics["engineeringGapCount"]
    result["sdkAuthority"] = "build/sdk"
    result["sdkMode"] = "sole-publisher"
    result["sdkConsumerTest"] = "docs/validation/SDKConsumerTest"
    result["status"] = "PASS"
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-only", action="store_true",
                        help="Validate maintained source without requiring build outputs.")
    args = parser.parse_args()
    if sys.version_info[:2] != (3, 9):
        parser.error("Python 3.9 is required; do not change the installed environment.")
    if args.source_only:
        REPORT = PROJECT / "output/logs/project-source-check.json"
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    try:
        result = check_source() if args.source_only else check()
    except Exception as error:
        result = {"project": PROJECT.name, "status": "FAIL", "error": str(error)}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))
    sys.exit(0 if result["status"] == "PASS" else 1)
