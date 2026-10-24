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
REPORT = PROJECT / "output/logs/project-check.json"
MAINTAINED_CONFIGS = {
    "pixso_native/component-mapping.json",
    "pixso_native/layout-mapping.json",
    "pixso_native/dsm/component_parsers.json",
}


def is_source_input(file):
    """Allow tool sources and named maintained mappings, not generated data."""
    allowed = {".cpp", ".h", ".ps1", ".py", ".js", ".cjs", ".cmake", ".cmd", ".sh"}
    return (
        file.suffix in allowed
        or file.name in {"CMakeLists.txt", "README.md", ".clang-format"}
        or file.relative_to(SOURCE).as_posix() in MAINTAINED_CONFIGS
    )


def normalize_for_boundary_check(content):
    # GenerateLayout 必须消费唯一权威组件 SDK；仅对这条已定义的发布路径保留白名单。
    return content.replace(
        "/ComponentLab-Delivery/DesignToCode_GenerateComponent/build/sdk/",
        "/AUTHORIZED_GENERATE_COMPONENT_SDK/",
    )


def check():
    result = {"project": PROJECT.name, "localLinks": 0, "optionalBuildLinks": 0, "resourceFiles": 0}
    result["pendingDeletion"] = []
    for directory in PROJECT.iterdir():
        if not directory.is_dir():
            continue
        if directory.name.startswith(("待删除-", "需删除-")):
            result["pendingDeletion"].append(directory.name)
            continue
        assert directory.name in {"Source", "docs", "build", "output"}, (directory, "unexpected maintained directory")
    # Directory junctions would silently restore a shared dependency after a physical split.
    for file in SOURCE.rglob("*"):
        assert not file.is_symlink() and not getattr(file, "is_junction", lambda: False)(), file
        if not file.is_file() or file == Path(__file__):
            continue
        assert is_source_input(file), (file, "non-tool file in Source")
        if file.suffix in {".cpp", ".h", ".ps1", ".py", ".js", ".cjs", ".cmd", ".qrc"} or file.name == "CMakeLists.txt":
            code = normalize_for_boundary_check(
                file.read_text(encoding="utf-8-sig").replace("\\", "/")
            )
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
        if file.is_relative_to(PROJECT / "docs/archive"):
            continue  # Frozen evidence preserves historical paths, not active navigation.
        content = file.read_text(encoding="utf-8-sig")
        assert len(re.findall(r"^\x60\x60\x60", content, re.M)) % 2 == 0, file
        for link in re.findall(r"\]\(([^)]+)\)", content):
            link = link.strip("<> ").split("#")[0]
            if not link or re.match(r"^https?://", link):
                continue
            target = (file.parent / link).resolve()
            if target.is_relative_to(PROJECT / "build") or target.is_relative_to(PROJECT / "output"):
                result["optionalBuildLinks"] += 1
                continue  # Generated examples are absent after deleting build, by design.
            assert target.is_relative_to(PROJECT) and target.exists(), (file, link)
            result["localLinks"] += 1
    # Inspect only active build descriptions; before-project-split stores explicitly retired caches.
    cache = PROJECT / "output/work/cache"
    for ninja in cache.rglob("build.ninja"):
        if "before-project-split" in ninja.relative_to(cache).parts:
            continue
        content = normalize_for_boundary_check(
            ninja.read_text(encoding="utf-8").replace("\\", "/")
        )
        for name in OTHER:
            assert "/ComponentLab-Delivery/" + name + "/" not in content, (ninja, name)
        assert "/ComponentLab-Delivery/DesignToCode/" not in content, (ninja, "retired path")
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
