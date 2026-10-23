"""Static contract check for the self-contained Qt-native source package."""

from __future__ import annotations

import hashlib
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "baseline-sdk"
SCOPE = ROOT / "component-definitions" / "base-sdk-scope.json"
FONT_SOURCE = ROOT / "runtime" / "assets" / "fonts" / "HarmonyOSSans"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate():
    expected = {
        "Form/README.md",
        "View/ZNativeComponents.qss",
        "View/z_native_components.qrc",
        "View/fonts/HarmonyOS_Sans.ttf",
        "View/fonts/HarmonyOS_Sans_SC.ttf",
        "View/fonts/LICENSE.txt",
        "View/icons/checkbox-indeterminate.svg",
        "Include/MyDesignSystem/MyDesignSystem.h",
        "Include/MyDesignSystem/ZNativeComponents.h",
        "Include/MyDesignSystem/qt5152_msvc_compat.h",
        "SRC/ZNativeComponents.cpp",
        "cmake/CMakeLists.txt",
        "cmake/GeneratedComponentSDKConfig.cmake.in",
        "manifest/component-package.json",
        "manifest/base-sdk-scope.json",
        "CMakeLists.txt",
    }
    actual = {path.relative_to(PACKAGE).as_posix() for path in PACKAGE.rglob("*") if path.is_file()}
    assert expected <= actual, sorted(expected - actual)

    scope = json.loads(SCOPE.read_text(encoding="utf-8"))
    public_header = (PACKAGE / "Include/MyDesignSystem/ZNativeComponents.h").read_text(
        encoding="utf-8"
    )
    declared_qt_classes = re.findall(r"X\((Q[A-Za-z0-9]+), Z_[A-Za-z0-9]+\)", public_header)
    assert declared_qt_classes == scope["approvedQtClasses"]
    assert len(declared_qt_classes) == scope["validation"]["expectedNativeComponentCount"]

    qss = (PACKAGE / "View/ZNativeComponents.qss").read_text(encoding="utf-8")
    for forbidden in ("qproperty-", "--component-", "--qtlab-", "sToken", "palette.json", ".json"):
        assert forbidden not in qss, forbidden
    assert "HarmonyOS Sans" in qss and "HarmonyOS Sans SC" in qss
    assert "::indicator:indeterminate" in qss

    source = (PACKAGE / "SRC/ZNativeComponents.cpp").read_text(encoding="utf-8")
    for forbidden in ("ComponentFactory", "ViewPalette", "QssProbe", "TokenResources", "Surface.h"):
        assert forbidden not in source, forbidden
    assert "Q_INIT_RESOURCE(z_native_components)" in source

    cmake = (PACKAGE / "cmake/CMakeLists.txt").read_text(encoding="utf-8")
    for forbidden in ("ComponentFactory", "Z_FoundationComponents", "Z_DesignComponents", "runtime/component"):
        assert forbidden not in cmake, forbidden
    assert "SRC/ZNativeComponents.cpp" in cmake
    assert "View/z_native_components.qrc" in cmake

    qrc = PACKAGE / "View/z_native_components.qrc"
    for entry in ET.parse(qrc).findall(".//file"):
        path = (qrc.parent / entry.text).resolve()
        assert path.is_relative_to(PACKAGE) and path.is_file(), (entry.text, path)

    for name in ("HarmonyOS_Sans.ttf", "HarmonyOS_Sans_SC.ttf", "LICENSE.txt"):
        assert digest(PACKAGE / "View/fonts" / name) == digest(FONT_SOURCE / name), name

    manifest = json.loads((PACKAGE / "manifest/component-package.json").read_text(encoding="utf-8"))
    embedded_scope = PACKAGE / manifest["scope"]
    assert manifest["scope"] == "manifest/base-sdk-scope.json"
    assert embedded_scope.is_file()
    assert digest(embedded_scope) == digest(SCOPE)
    assert manifest["runtimeTokenJson"] is False
    assert manifest["legacyRuntime"] is False
    return {
        "status": "PASS",
        "nativeComponents": len(declared_qt_classes),
        "runtimeTokenJson": manifest["runtimeTokenJson"],
        "legacyRuntime": manifest["legacyRuntime"],
    }


if __name__ == "__main__":
    print(json.dumps(validate(), ensure_ascii=False))
