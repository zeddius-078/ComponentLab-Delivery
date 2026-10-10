"""Validate ComponentLab as a display-only consumer of GeneratedComponentSDK."""
import argparse
import hashlib
import json
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2]
PROJECT = SOURCE.parent
SDK = PROJECT / "build/sdk"


def validation_inputs():
    paths = [p for folder in (SOURCE / "delivery", SOURCE / "presentation")
             for p in folder.rglob("*") if p.is_file() and "__pycache__" not in p.parts]
    paths.extend((SOURCE / name) for name in
                 ("CMakeLists.txt", "GeneratedComponentDependency.cmake") )
    paths.append(SDK / "manifest/component-sdk.json")
    return {p.relative_to(PROJECT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(set(paths))}


def check():
    manifest = json.loads((SDK / "manifest/component-sdk.json").read_bytes())
    assert manifest["owner"] == "DesignToCode_GenerateComponent"
    assert manifest["componentAuthority"] and manifest["tokenAuthority"]
    assert manifest["linkage"] == "static"
    assert (SDK / "include/MyDesignSystem/MyDesignSystem.h").is_file()
    assert (SDK / "lib/GeneratedComponentSDK.lib").is_file()
    assert (SDK / "resources/tokens/default").is_dir()
    assert not list(SDK.rglob("*.exe")) and not list(SDK.rglob("*.dll"))
    demo = PROJECT / "build/demo/component_gallery"
    if demo.exists():
        assert not list(demo.rglob("*.lib")) and not list(demo.rglob("*.h"))
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
    return {"status": "PASS", "sdkManifestSha256": hashlib.sha256(
        (SDK / "manifest/component-sdk.json").read_bytes()).hexdigest(),
        "sourceRoles": ["delivery", "presentation"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--print-inputs", action="store_true")
    args = parser.parse_args()
    if args.print_inputs:
        print(json.dumps(validation_inputs(), sort_keys=True))
        return
    result = check()
    if args.snapshot:
        args.snapshot.write_text(json.dumps(validation_inputs(), indent=2, sort_keys=True),
                                 encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
