"""Read compiled Qt Tokens and prepare a bounded Figma variable transport."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4] / "ComponentKit/design-assets"


def read(relative):
    return json.loads((ROOT / relative).read_text(encoding="utf-8-sig"))


def prepare():
    runtime = read("DesignSystem/generated/runtime/tokens_qt.json")
    button = read("tokens/button-contract.json")
    fields = read("tokens/forms-contract.json")
    references = {}

    def add(value, scope, owner):
        if not isinstance(value, str) or not value.startswith("{"):
            return
        path = value[1:-1]
        item = references.setdefault(path, {"scopes": set(), "owners": set()})
        item["scopes"].update(scope)
        item["owners"].add(owner)

    for key, value in button["palette"].items():
        add(value, ["ALL_FILLS", "STROKE_COLOR"], "button.palette." + key)
    for key, value in fields["palette"].items():
        add(value, ["ALL_FILLS", "STROKE_COLOR"], "forms.palette." + key)
    add(button["shape"]["radius"], ["CORNER_RADIUS"], "button.radius")
    for metric in button["metrics"]:
        for key in ("minimumHeight", "paddingInline", "gap", "iconCanvas", "fontSize"):
            scope = ["FONT_SIZE"] if key == "fontSize" else ["GAP"] if key in ("paddingInline", "gap") else ["WIDTH_HEIGHT"]
            add(metric[key], scope, f"button.{metric['name']}.{key}")
    for key in ("radius", "labelGap", "tagPadding", "labelFontSize", "hintFontSize", "gap", "iconSize", "actionSize", "borderWidth"):
        scope = ["CORNER_RADIUS"] if key == "radius" else ["FONT_SIZE"] if "FontSize" in key else ["STROKE_FLOAT"] if key == "borderWidth" else ["WIDTH_HEIGHT"] if key in ("iconSize", "actionSize") else ["GAP"]
        add(fields["metrics"][key], scope, "forms.metrics." + key)
    for size, metrics in fields["sizes"].items():
        for key in ("height", "paddingInline", "fontSize"):
            scope = ["FONT_SIZE"] if key == "fontSize" else ["GAP"] if key == "paddingInline" else ["WIDTH_HEIGHT"]
            add(metrics[key], scope, f"forms.{size}.{key}")
    for path, scopes, owner in (
        ("component.checkbox.base.default.indicator.radius", ["CORNER_RADIUS"], "checkbox.radius"),
        ("component.checkbox.size.small.indicator.size", ["WIDTH_HEIGHT"], "checkbox.sm.indicator"),
        ("component.checkbox.size.large.indicator.size", ["WIDTH_HEIGHT"], "checkbox.lg.indicator"),
    ):
        add("{" + path + "}", scopes, owner)

    flattened = {name: {path: value for group in groups.values() for path, value in group.items()} for name, groups in runtime["profiles"].items()}
    variables = []
    for path, spec in sorted(references.items()):
        values = {name: data[path] for name, data in flattened.items()}
        kind = "COLOR" if path in runtime["profiles"][next(iter(flattened))]["color"] else "FLOAT"
        if kind == "FLOAT" and not all(isinstance(v, (int, float)) and not isinstance(v, bool) for v in values.values()):
            raise ValueError(f"Unsupported variable type: {path}")
        variables.append({"sourcePath": path, "name": path.replace(".", "/"), "type": kind, "scopes": sorted(spec["scopes"]), "owners": sorted(spec["owners"]), "values": values})
    files = ["DesignSystem/generated/runtime/tokens_qt.json", "tokens/button-contract.json", "tokens/forms-contract.json", "tokens/checkbox-contract.json"]
    font_path = button["font"]["family"][1:-1]
    fonts = {profile: values[font_path] for profile, values in flattened.items()}
    return {"schemaVersion": 1, "sourcePolicy": "compiled-qt-token-snapshot-read-only", "profiles": list(flattened), "sourceHashes": {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in files}, "font": {"sourcePath": font_path, "qtFamily": next(iter(fonts.values())), "profiles": fonts, "figmaFamily": None, "status": "AWAITING_FONT_DECISION"}, "variables": variables}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = prepare()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared {len(result['variables'])} variables across {len(result['profiles'])} Profiles: {args.output}")
