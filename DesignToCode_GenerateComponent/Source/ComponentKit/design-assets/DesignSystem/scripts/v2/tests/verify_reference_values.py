"""Verify the four imported component families resolve to the exact values of
the reference files in DesignSystem/imports/reference/."""
import json
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SCRIPTS))

from v2.model import TokenModel
from v2.resolver import Resolver
from v2.values import qt_value

ROOT = SCRIPTS.parents[1]
SOURCE = ROOT / "DesignSystem/source"
REF = ROOT / "DesignSystem/imports/reference"


def load_reference(name):
    doc = json.loads((REF / f"{name}.json").read_text(encoding="utf-8"))
    comp = doc[name]
    sem = doc.get("semantic", {})
    prim = doc.get("primitive", {})
    # flatten the three layers into a reference namespace
    flat = {}

    def walk(node, prefix):
        for key, value in node.items():
            path = f"{prefix}/{key}" if prefix else key
            if isinstance(value, dict) and "$value" in value:
                flat[path] = value["$value"]
            elif isinstance(value, dict):
                walk(value, path)

    walk({"semantic": sem, "primitive": prim}, "")
    # component references use bare semantic/primitive paths
    walk(sem, "")
    walk(prim, "")
    walk(comp, "comp")
    return comp, flat


def resolve_ref(path, flat, seen=()):
    value = flat.get(path)
    if value is None:
        raise KeyError(f"unresolved reference: {path}")
    if isinstance(value, str) and value.startswith("{") and value.endswith("}"):
        target = value[1:-1]
        if target in seen:
            raise ValueError(f"cycle: {path}")
        return resolve_ref(target, flat, (*seen, path))
    return value


def normalize(value):
    if isinstance(value, dict) and "colorSpace" in value:
        r, g, b = (int(round(c * 255)) for c in value["components"][:3])
        alpha = value.get("alpha", 1)
        if alpha >= 1:
            return f"#{r:02X}{g:02X}{b:02X}"
        return f"rgba({r}, {g}, {b}, {round(alpha, 6)})"
    if isinstance(value, str) and value.startswith("rgba("):
        parts = value[5:-1].replace(" ", "").split(",")
        r, g, b = (int(float(x)) for x in parts[:3])
        alpha = float(parts[3])
        if alpha >= 1:
            return f"#{r:02X}{g:02X}{b:02X}"
        return f"rgba({r}, {g}, {b}, {round(alpha, 6)})"
    if isinstance(value, float):
        return round(value, 6)
    return value


CANONICAL_MAP = {
    "radio": {
        "content.fontSize.md": "component.radio.base.default.content.font-size",
        "content.fontWeight.regular": "component.radio.base.default.content.font-weight",
        "content.lineHeight.md": "component.radio.base.default.content.line-height",
        "color.text.default": "component.radio.base.default.label.color",
        "color.text.disabled": "component.radio.base.disabled.label.color",
        "color.on.bgDefault": "component.radio.base.selected.indicator.color",
        "color.on.bgDisable": "component.radio.base.selected-disabled.indicator.color",
        "color.on.bgPressed": "component.radio.base.selected-pressed.indicator.color",
        "color.on.bgHover": "component.radio.base.selected-hover.indicator.color",
        "color.on.bgInvert": "component.radio.base.selected.glyph.color",
        "color.off.bgDefault": "component.radio.base.default.indicator.background-color",
        "color.off.bgDisable": "component.radio.base.disabled.indicator.background-color",
        "color.off.bgPressed": "component.radio.base.pressed.indicator.background-color",
        "color.off.bgHover": "component.radio.base.hover.indicator.background-color",
        "color.off.Border": "component.radio.base.selected.indicator.border-color",
        "size.sm": "component.radio.size.sm.indicator.size",
        "size.md": "component.radio.size.md.indicator.size",
        "border.default": "component.radio.base.default.indicator.border-width",
        "padding.sm": "component.radio.size.sm.container.padding",
        "padding.md": "component.radio.size.md.container.padding",
    },
    "menu": {
        "padding.md": "component.menu.base.default.container.padding-md",
        "padding.lg": "component.menu.base.default.container.padding-lg",
        "padding.2xl": "component.menu.base.default.container.padding-2xl",
        "padding.3xl": "component.menu.base.default.container.padding-3xl",
        "spacing.sm": "component.menu.base.default.content.gap-sm",
        "spacing.md": "component.menu.base.default.content.gap-md",
        "radius.default": "component.menu.base.default.container.radius",
        "radius.none": "component.menu.base.none.container.radius",
        "border.default": "component.menu.base.default.border.width",
        "content.fontSize.default": "component.menu.base.default.content.font-size",
        "content.fontWeight.regular": "component.menu.base.default.content.font-weight",
        "content.fontWeight.bold": "component.menu.base.emphasis.content.font-weight",
        "content.iconSize.onlyIcon-sm": "component.menu.size.sm.icon.size",
        "content.iconSize.onlyIcon-md": "component.menu.size.md.icon.size",
        "content.height.sm": "component.menu.size.sm.container.height",
        "content.height.md": "component.menu.size.md.container.height",
        "content.lineHeight.default": "component.menu.base.default.content.line-height",
        "color.bg.defaultColor1-primary": "component.menu.base.default.container.gradient-stop-01",
        "color.bg.defaultColor2-primary": "component.menu.base.default.container.gradient-stop-02",
        "color.bg.selected-primary": "component.menu.base.selected.container.background-color",
        "color.bg.Pressed-primary": "component.menu.base.pressed.container.background-color",
        "color.bg.defaultColor-secondary": "component.menu.base.default.container.background-color",
        "color.bg.selected-secondary": "component.menu.base.selected.item.background-color",
        "color.bg.hover-secondary": "component.menu.base.hover.container.background-color",
        "color.bg.disabled": "component.menu.base.disabled.container.background-color",
        "color.content.default": "component.menu.base.default.content.color",
        "color.content.selected": "component.menu.base.selected.content.color",
        "color.content.disabled": "component.menu.base.disabled.content.color",
        "color.border.default": "component.menu.base.default.border.color",
    },
    "tree": {
        "color.bg.primary": "component.tree.base.default.container.background-color",
        "color.bg.hover": "component.tree.base.hover.item.background-color",
        "color.bg.select": "component.tree.base.selected.item.background-color",
        "color.Border.default": "component.tree.base.default.border.color",
        "color.text.default": "component.tree.base.default.content.color",
        "content.fontSize.text": "component.tree.base.default.content.font-size",
        "content.fontWeight.regular": "component.tree.base.default.content.font-weight",
        "content.lineHeight.md": "component.tree.base.default.content.line-height",
        "content.iconSize.default": "component.tree.base.default.icon.size",
        "content.height.md": "component.tree.base.default.item.height",
        "padding.default": "component.tree.base.default.container.padding",
        "border.default": "component.tree.base.default.border.width",
    },
    "table": {
        "color.bg.header": "component.table.base.default.header.background-color",
        "color.bg.default": "component.table.base.default.container.background-color",
        "color.bg.zebraPrint": "component.table.base.zebra.container.background-color",
        "color.bg.Hover": "component.table.base.hover.item.background-color",
        "color.bg.disable": "component.table.base.disabled.container.background-color",
        "color.bg.pressed": "component.table.base.pressed.item.background-color",
        "color.bg.alarm": "component.table.base.alarm.item.background-color",
        "color.bg.alarmHover": "component.table.base.alarm-hover.item.background-color",
        "color.bg.alarmSelect": "component.table.base.alarm-select.item.background-color",
        "color.bg.alarmSoft": "component.table.base.alarm-soft.item.background-color",
        "color.bg.alarmSoftHover": "component.table.base.alarm-soft-hover.item.background-color",
        "color.bg.alarmSoftSelect": "component.table.base.alarm-soft-select.item.background-color",
        "color.bg.error": "component.table.base.error.item.background-color",
        "color.bg.errorHover": "component.table.base.error-hover.item.background-color",
        "color.bg.errorSelect": "component.table.base.error-select.item.background-color",
        "color.bg.errorSoft": "component.table.base.error-soft.item.background-color",
        "color.bg.errorSoftHover": "component.table.base.error-soft-hover.item.background-color",
        "color.bg.errorSoftSelect": "component.table.base.error-soft-select.item.background-color",
        "color.bg.warning": "component.table.base.warning.item.background-color",
        "color.bg.warningHover": "component.table.base.warning-hover.item.background-color",
        "color.bg.warningSelect": "component.table.base.warning-select.item.background-color",
        "color.bg.warningSoft": "component.table.base.warning-soft.item.background-color",
        "color.bg.warningSoftHover": "component.table.base.warning-soft-hover.item.background-color",
        "color.bg.warningSoftSelect": "component.table.base.warning-soft-select.item.background-color",
        "color.border.default": "component.table.base.default.border.color",
        "color.border.select": "component.table.base.selected.border.color",
        "color.border.alarm": "component.table.base.alarm.border.color",
        "color.border.warning": "component.table.base.warning.border.color",
        "color.text.default": "component.table.base.default.content.color",
        "color.text.invert": "component.table.base.invert.content.color",
        "color.text.disable": "component.table.base.disabled.content.color",
        "color.text.alarm": "component.table.base.alarm.content.color",
        "color.text.warning": "component.table.base.warning.content.color",
        "color.text.error": "component.table.base.error.content.color",
        "color.text.revise": "component.table.base.revise.content.color",
        "color.text.disabled": "component.table.base.disabled.content.color",
        "color.text.unit": "component.table.base.unit.content.color",
        "content.height.sm": "component.table.size.sm.content.height",
        "content.height.md": "component.table.size.md.content.height",
        "content.height.lg": "component.table.size.lg.content.height",
        "content.height.xl": "component.table.size.xl.content.height",
        "content.iconSize.sm": "component.table.base.default.icon.size",
        "content.fontSize.md": "component.table.base.default.content.font-size",
        "content.fontWeight.md": "component.table.base.default.content.font-weight",
        "content.fontWeight.lg": "component.table.base.emphasis.content.font-weight",
        "content.lineHeight.xs": "component.table.base.default.content.line-height-xs",
        "content.lineHeight.sm": "component.table.base.default.content.line-height-sm",
        "content.lineHeight.md": "component.table.base.default.content.line-height-md",
        "padding.horizontal": "component.table.base.default.container.padding-inline",
        "padding.vertical": "component.table.base.default.container.padding-block",
        "size.sm": "component.table.size.sm.item.height",
        "size.md": "component.table.size.md.item.height",
        "size.lg": "component.table.size.lg.item.height",
        "spacing.default": "component.table.base.default.content.gap",
        "borderWidth.md": "component.table.base.default.border.width",
        "borderWidth.lg": "component.table.base.strong.border.width",
    },
}


def main():
    model = TokenModel(SOURCE)
    context = model.context("standard", "light")
    resolver = Resolver(model, context)
    total = 0
    failures = []
    for family, mapping in CANONICAL_MAP.items():
        comp, flat = load_reference(family)
        for ref_key, canonical in mapping.items():
            expected = resolve_ref("comp/" + ref_key.replace(".", "/"), flat)
            canonical_resolved = resolver.resolve(canonical)
            actual = qt_value(canonical_resolved)
            total += 1
            if normalize(expected) != normalize(actual):
                failures.append((family, ref_key, expected, actual))
    print(f"checked {total} reference property bindings")
    if failures:
        for family, ref_key, expected, actual in failures:
            print(f"MISMATCH {family}.{ref_key}: expected {expected} got {actual}")
        sys.exit(1)
    print("ALL VALUES MATCH THE REFERENCE FILES EXACTLY")


if __name__ == "__main__":
    main()
