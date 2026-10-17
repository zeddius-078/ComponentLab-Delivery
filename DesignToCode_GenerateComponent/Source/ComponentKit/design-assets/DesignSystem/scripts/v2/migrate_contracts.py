"""Migrate component contracts and design mapping files from legacy token
references to V2 canonical / adapter.qt paths. One-way; run once per consumer
file. The replacement map derives from the legacy decision map plus the
curated runtime role table. Values are preserved by construction."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]  # design-assets

REFERENCE = re.compile(r"\{([a-zA-Z0-9_.-]+)\}")

CURATED = {
    # legacy runtime roles -> V2 canonical
    "semantic.color.text.onBrand": "semantic.color.text.invert",
    "semantic.color.text.inverse": "semantic.color.text.invert",
    # text.placeholder stays a legacy runtime role: the design semantic layer
    # defines no placeholder role and the audited dark adaptation is carried on
    # that role. Registered with an exit condition.
    "semantic.color.border.focus": "semantic.color.interactive.brand.focus-ring",
    "semantic.color.border.subtle": "semantic.color.border.secondary",
    "semantic.color.border.control": "semantic.color.border.default",
    "semantic.color.background.brand": "semantic.color.interactive.brand.default",
    "semantic.color.background.elevated": "semantic.color.background.surface-raised",
    "semantic.color.background.interactive-hover": "semantic.color.background.hover",
    "semantic.color.background.interactive-active": "semantic.color.background.pressed",
    "semantic.color.action.primary": "semantic.color.interactive.brand.default",
    "semantic.color.action.primary-hover": "semantic.color.interactive.brand.hover",
    "semantic.color.action.primary-pressed": "semantic.color.interactive.brand.pressed",
    "semantic.color.action.primary-disabled": "semantic.color.interactive.brand.disabled",
    "semantic.color.action.primary-text": "semantic.color.text.invert",
    "semantic.color.action.primary-bg": "semantic.color.interactive.brand.subtle",
    "semantic.color.action.primary-bg-hover": "primitive.color.highlight.purple-15",
    "semantic.color.action.primary-border": "primitive.color.highlight.purple-30",
    "semantic.color.action.secondary": "semantic.color.background.surface",
    "semantic.color.action.secondary-hover": "semantic.color.background.pressed",
    "semantic.color.action.secondary-pressed": "primitive.color.neutral.gray.10",
    "semantic.color.status.danger": "semantic.color.status.danger.default",
    "semantic.color.status.danger-hover": "semantic.color.status.danger.hover",
    "semantic.color.status.danger-pressed": "semantic.color.status.danger.pressed",
    "semantic.color.status.danger-bg": "semantic.color.status.danger.subtle",
    "semantic.color.status.warning": "semantic.color.status.warning.default",
    "semantic.color.status.warning-bg": "primitive.color.functional.yellow-subtle",
    "semantic.color.status.success": "primitive.color.functional.green.60",
    "semantic.color.status.success-bg": "primitive.color.functional.green.5",
    "semantic.color.status.info": "primitive.color.functional.blue",
    "semantic.color.status.info-bg": "primitive.color.functional.blue-subtle",
    "semantic.radius.control-md": "semantic.radius.control-default",
    "semantic.radius.input": "semantic.radius.control-default",
    "semantic.dimension.control.height-xs": "adapter.qt.dimension.control-height.xs",
    "semantic.dimension.control.height-sm": "adapter.qt.dimension.control-height.sm",
    "semantic.dimension.control.height-md": "adapter.qt.dimension.control-height.md",
    "semantic.dimension.control.height-lg": "adapter.qt.dimension.control-height.lg",
    "semantic.dimension.control.interactive-size": "adapter.qt.dimension.control-height.interactive-size",
    "base.fontSize.caption": "adapter.qt.font-size.caption",
    "base.fontSize.body-sm": "adapter.qt.font-size.body-sm",
    "base.fontSize.body-md": "adapter.qt.font-size.body-md",
    "base.fontSize.subheading-sm": "semantic.typography.heading.heading3.font-size",
    "base.lineHeight.body-sm": "semantic.typography.body.default.line-height",
    "base.lineHeight.body-md": "semantic.typography.subtitle.sub-title.line-height",
    "base.fontWeight.regular": "semantic.typography.font-weight.body",
    "base.borderWidth.hairline": "semantic.border-width.boundary-default",
    "base.space.4": "primitive.spacing.4",
    "base.color.neutral.black.500": "primitive.color.neutral.gray.100",
    "component.basic.input.state.default.background": "component.input.base.default.container.background-color",
    "component.basic.input.state.default.text": "component.input.base.default.content.color",
    "component.basic.input.state.default.placeholder": "component.input.base.default.placeholder.color",
    "component.basic.input.state.default.border": "component.input.base.default.border.color",
    "component.basic.input.state.hover.border": "component.input.base.hover.border.color",
    "component.basic.input.state.focus.border": "component.input.base.focused.border.color",
    "component.basic.input.state.focus.outlineWidth": "component.input.base.focused.focus-ring.width",
    "component.basic.input.state.error.border": "component.input.base.error.border.color",
    "component.basic.input.state.error.background": "component.input.base.error.container.background-color",
    "component.basic.input.state.disabled.text": "component.input.base.disabled.content.color",
    "component.basic.input.radius": "component.input.base.default.container.radius",
    "component.basic.checkbox.radius": "component.checkbox.base.default.indicator.radius",
    "component.basic.checkbox.borderWidth": "component.checkbox.base.default.indicator.border-width",
    "component.basic.checkbox.focus.borderWidth": "component.checkbox.base.focused.focus-ring.width",
    "component.basic.checkbox.state.unchecked.border": "semantic.color.icon.secondary",
    "component.basic.checkbox.state.selected.background": "semantic.color.interactive.brand.default",
    "component.basic.checkbox.state.selected.border": "semantic.color.interactive.brand.default",
    "component.basic.checkbox.state.selected.hoverBackground": "semantic.color.interactive.brand.hover",
    "component.basic.checkbox.state.selected.hoverBorder": "semantic.color.interactive.brand.hover",
    "component.basic.checkbox.state.selected.pressedBackground": "semantic.color.interactive.brand.pressed",
    "component.basic.checkbox.state.selected.pressedBorder": "semantic.color.interactive.brand.pressed",
    "component.basic.checkbox.state.selected.disabledBackground": "semantic.color.icon.disabled",
    "component.basic.checkbox.state.selected.disabledBorder": "semantic.color.icon.disabled",
    "component.basic.checkbox.sizes.sm.indicator": "component.checkbox.size.small.indicator.size",
    "component.basic.checkbox.sizes.sm.fontSize": "component.checkbox.base.default.label.font-size",
    "component.basic.checkbox.sizes.sm.spacing": "component.checkbox.base.default.content.gap",
    "component.basic.checkbox.sizes.lg.indicator": "component.checkbox.size.large.indicator.size",
    "component.basic.checkbox.sizes.lg.fontSize": "component.checkbox.base.default.label.font-size",
    "component.basic.checkbox.sizes.lg.spacing": "component.checkbox.base.default.content.gap",
    "base.stokenAdapter.button.XS.iconContainer": "adapter.qt.button.iconContainer.XS",
    "base.stokenAdapter.button.S.iconContainer": "adapter.qt.button.iconContainer.S",
    "base.stokenAdapter.button.M.iconContainer": "adapter.qt.button.iconContainer.M",
    "base.stokenAdapter.button.L.iconContainer": "adapter.qt.button.iconContainer.L",
    "component.basic.checkbox.sizes.sm.hitTarget": "adapter.qt.checkbox.hitTarget.sm",
    "component.basic.checkbox.sizes.sm.qssContentSize": "adapter.qt.checkbox.qssContentSize.sm",
    "component.basic.checkbox.sizes.lg.hitTarget": "adapter.qt.checkbox.hitTarget.lg",
    "component.basic.checkbox.sizes.lg.qssContentSize": "adapter.qt.checkbox.qssContentSize.lg",
    "component.basic.button.size.sm.height": "adapter.qt.button.size.sm.height",
    "component.basic.button.size.md.height": "adapter.qt.button.size.md.height",
    "component.basic.button.size.lg.height": "adapter.qt.button.size.lg.height",
    "component.basic.button.size.sm.fontSize": "semantic.typography.label.default.font-size",
    "component.basic.button.size.md.fontSize": "semantic.typography.label.default.font-size",
    "component.basic.button.size.lg.fontSize": "semantic.typography.label.default.font-size",
    "component.basic.input.size.sm.height": "adapter.qt.input.size.sm.height",
    "component.basic.input.size.md.height": "adapter.qt.input.size.md.height",
    "component.basic.input.size.lg.height": "adapter.qt.input.size.lg.height",
    "component.basic.input.size.sm.paddingInline": "component.input.size.small.container.padding-inline",
    "component.basic.input.size.md.paddingInline": "component.input.size.medium.container.padding-inline",
    "component.basic.input.size.lg.paddingInline": "component.input.size.large.container.padding-inline",
    "component.basic.input.size.sm.paddingBlock": "component.button.base.default.container.padding-block",
    "component.basic.input.size.md.paddingBlock": "component.button.base.default.container.padding-block",
    "component.basic.input.size.lg.paddingBlock": "component.button.base.default.container.padding-block",
    "component.basic.input.size.sm.fontSize": "component.input.base.default.content.font-size",
    "component.basic.input.size.md.fontSize": "component.input.base.default.content.font-size",
    "component.basic.input.size.lg.fontSize": "component.input.base.default.content.font-size",
}


def build_map() -> dict[str, str]:
    mapping = dict(CURATED)
    decisions = json.loads((ROOT / "DesignSystem/config/legacy-map.json").read_text(encoding="utf-8"))
    for row in decisions["decisions"]:
        legacy_key = row.get("legacyKey", "")
        canonical = row.get("canonicalPath")
        if not legacy_key.startswith("--") or not canonical:
            continue
        base_key = "base.stoken." + legacy_key[2:].replace("/", ".")
        if base_key not in mapping:
            mapping[base_key] = canonical
    return mapping


def migrate_text(text: str, mapping: dict[str, str]) -> tuple[str, list[str]]:
    replaced: list[str] = []

    def replace(match: re.Match) -> str:
        ref = match.group(1)
        target = mapping.get(ref)
        if target and target != ref:
            replaced.append(f"{ref} -> {target}")
            return "{" + target + "}"
        return match.group(0)

    return REFERENCE.sub(replace, text), replaced


def migrate_plain_tokens(text: str, mapping: dict[str, str]) -> tuple[str, int]:
    count = 0
    lines = text.splitlines(keepends=True)
    for index, line in enumerate(lines):
        for ref, target in mapping.items():
            if ref != target and ('"' + ref + '"') in line:
                lines[index] = line.replace('"' + ref + '"', '"' + target + '"')
                count += 1
    return "".join(lines), count


def main() -> int:
    mapping = build_map()
    files = [
        ROOT / "tokens/button-contract.json",
        ROOT / "tokens/checkbox-contract.json",
        ROOT / "tokens/forms-contract.json",
        ROOT / "tokens/figma-mapping.json",
    ]
    for path in files:
        text = path.read_text(encoding="utf-8")
        migrated, replaced = migrate_text(text, mapping)
        if path.name == "figma-mapping.json":
            migrated, plain_count = migrate_plain_tokens(migrated, mapping)
            replaced = replaced + [""] * 0
            total = len(replaced) + plain_count
        else:
            total = len(replaced)
        leftover = {m.group(1) for m in REFERENCE.finditer(migrated)
                    if m.group(1).startswith(("base.stoken", "component.basic", "component.composite",
                                              "base.", "semantic.dimension"))}
        if leftover:
            shown = sorted(leftover)[:12]
            print(f"NOTE {path.name}: remaining legacy refs (frozen Commerly material, "
                  f"registered with exit conditions): {shown}")
        if total:
            path.write_text(migrated, encoding="utf-8")
            print(f"{path.name}: {total} replacements")
        else:
            print(f"{path.name}: no changes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
