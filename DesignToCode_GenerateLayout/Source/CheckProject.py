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
