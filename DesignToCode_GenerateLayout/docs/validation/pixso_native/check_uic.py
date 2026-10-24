"""Independent Qt 5.15.2 syntax probe; not target-page acceptance."""
import argparse
import ctypes
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from page_output import render_page, write_page


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--uic", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if sys.platform == "win32":
        # Process-local error mode prevents system loader dialogs from hanging
        # unattended probes. It does not change global environment/settings.
        ctypes.windll.kernel32.SetErrorMode(0x8003)
    version = subprocess.run([str(args.uic.resolve()), "-v"], capture_output=True, timeout=15)
    version_text = (version.stdout + version.stderr).decode("utf-8", errors="replace").strip()
    if version.returncode or version_text != "uic 5.15.2":
        raise RuntimeError("QT5152_UIC_REQUIRED: " + version_text)
    def container(name, direction, children):
        return {"kind": "container", "name": name,
                "source": {"file": "nested-layout-synthetic-fixture"},
                "css": {"display": "flex", "flex-direction": direction,
                        "gap": 4, "padding-left": 1, "padding-top": 2,
                        "padding-right": 3, "padding-bottom": 4},
                "children": children}
    root = container("root", "row", [
        container("middle", "column", [container("inner", "row", [])])])
    root["css"].update({"width": "100%", "height": "480px",
                       "box-sizing": "border-box", "overflow": "hidden"})
    root["viewport"] = {"width": 640, "height": 480, "source": "synthetic probe viewport"}
    root["children"][0]["css"].update({
        "width": "320px", "height": "100%",
        "box-sizing": "border-box", "flex-shrink": "0"})
    growth = container("growth", "column", [])
    growth["css"].update({"width": "auto", "flex-grow": "1", "flex-shrink": "0"})
    root["children"].append(growth)
    body = container("fillBody", "row", root["children"])
    body["css"].update({"width": "100%", "height": "100%", "box-sizing": "border-box"})
    wrapper = container("fillWrapper", "column", [body])
    wrapper["css"].update({"width": "100%", "height": "100%", "box-sizing": "border-box",
                           "padding-left": 10, "padding-top": 10,
                           "padding-right": 10, "padding-bottom": 10})
    root["css"]["flex-direction"] = "column"
    root["children"] = [wrapper]
    mapping = {"schemaVersion": 1, "sdkRelease": "synthetic-no-components",
               "profiles": ["standard-light"], "components": []}
    mapping["pageHost"] = {"class": "designer_widgets::ThemeHost",
                           "header": "MyDesignSystem/DesignerWidgets.h",
                           "constructor": "profile-parent"}
    files = render_page("NestedLayoutProbe", root, mapping,
                        "synthetic-no-components", "standard-light",
                        page_styles={"root": {"background-color": "rgba(17, 34, 51, 1)"}})
    output = write_page(args.output, files)
    checked = subprocess.run([str(args.uic.resolve()),
                              str(output / "form/NestedLayoutProbe.ui")],
                             capture_output=True, timeout=15)
    if checked.returncode:
        raise RuntimeError("UIC_FAILED: " + checked.stderr.decode("utf-8", errors="replace"))
    # uic output stays outside the five-file page unit.
    print(json.dumps({"status": "通过", "scope": "synthetic nested layout syntax only",
                      "uicVersion": version_text, "page": str(output),
                      "generatedHeaderBytes": len(checked.stdout),
                      "targetPageAcceptance": "未执行"}, ensure_ascii=False))


if __name__ == "__main__":
    main()
