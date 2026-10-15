"""Check local documentation, resources and three-project dependency boundaries."""
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

PROJECT = Path(__file__).resolve().parent.parent
SOURCE = PROJECT / "Source"
NAMES = {"ComponentLab", "DesignToCode_GenerateComponent", "DesignToCode_GenerateLayout"}
OTHER = NAMES - {PROJECT.name}
REPORT = PROJECT / "build/reports/project-check.json"


def check():
    result = {"project": PROJECT.name, "localLinks": 0, "resourceFiles": 0}
    # The component Token chain is an owned production path and must survive directory cleanup.
    token_chain = [
        SOURCE / "ComponentKit/design-assets/tokens/button-contract.json",
        SOURCE / "ComponentKit/design-assets/tokens/checkbox-contract.json",
        SOURCE / "ComponentKit/design-assets/tokens/forms-contract.json",
        SOURCE / "ComponentKit/design-assets/tools/export_tokens.py",
        SOURCE / "ComponentKit/design-assets/tools/token_compiler.cpp",
        SOURCE / "ComponentKit/design-assets/tools/checkbox_token_compiler.cpp",
        SOURCE / "ComponentKit/design-assets/tools/forms_token_compiler.cpp",
        SOURCE / "ComponentKit/design-assets/generated",
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
        SOURCE / "ComponentKit/component-definitions/generated-component-source-map.json"
    ).read_text(encoding="utf-8"))
    assert len(source_map["components"]) == len(native_components)
    design_catalog = json.loads((
        SOURCE / "ComponentKit/component-definitions/generated-design-component-catalog.json"
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
    native_source = SOURCE / "ComponentKit/runtime/component/native"
    native_styles = SOURCE / "ComponentKit/design-assets/generated/native"
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
        for profile in ("light-comfortable", "light-compact", "dark-comfortable", "dark-compact"):
            assert (native_styles / profile / f"{sdk_class}.qss").is_file(), (profile, sdk_class)
    sdk = PROJECT / "build/sdk"
    # Exactly one first-level SDK output is allowed; build/cache remains disposable compiler state.
    sdk_outputs = [path for path in (PROJECT / "build").iterdir()
                   if path.is_dir() and "sdk" in path.name.lower()]
    assert sdk_outputs == [sdk], ("unexpected SDK output", sdk_outputs)
    manifest = json.loads((sdk / "manifest/component-sdk.json").read_bytes())
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
    assert not list(sdk.rglob("*.exe")) and not list(sdk.rglob("*.dll"))
    assert not (PROJECT / "build/dist").exists()
    consumer_test = SOURCE / "SDKConsumerTest"
    assert (consumer_test / "main.cpp").is_file()
    assert "#include <MyDesignSystem/MyDesignSystem.h>" in (consumer_test / "main.cpp").read_text(encoding="utf-8")
    assert "GeneratedComponentSDK::Components" in (consumer_test / "CMakeLists.txt").read_text(encoding="utf-8")
    demo = PROJECT / "build/demo/component_mapping_workbench"
    if demo.exists():
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
    for qrc in SOURCE.rglob("*.qrc"):
        for entry in ET.parse(qrc).findall(".//file"):
            target = (qrc.parent / entry.text).resolve()
            assert target.is_relative_to(SOURCE) and target.is_file(), (qrc, target)
            result["resourceFiles"] += 1
    # Navigation must remain usable after copying just this project.
    for file in [PROJECT / "README.md", *(PROJECT / "docs").rglob("*.md"), *SOURCE.rglob("*.md")]:
        content = file.read_text(encoding="utf-8-sig")
        assert len(re.findall(r"^\x60\x60\x60", content, re.M)) % 2 == 0, file
        for link in re.findall(r"\]\(([^)]+)\)", content):
            link = link.strip("<> ").split("#")[0]
            if not link or re.match(r"^https?://", link):
                continue
            target = (file.parent / link).resolve()
            assert target.is_relative_to(PROJECT) and target.exists(), (file, link)
            result["localLinks"] += 1
    # Inspect only active build descriptions; before-project-split stores explicitly retired caches.
    cache = PROJECT / "build/cache"
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
    result["sdkConsumerTest"] = "Source/SDKConsumerTest"
    result["status"] = "PASS"
    return result


if __name__ == "__main__":
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    try:
        result = check()
    except Exception as error:
        result = {"project": PROJECT.name, "status": "FAIL", "error": str(error)}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))
    sys.exit(0 if result["status"] == "PASS" else 1)
