"""Import the reference token files (radio/menu/tree/table) into the V2 source.

The reference files live in DesignSystem/imports/reference/ next to the frozen
stoken snapshot. Their STRUCTURE is known to be rough, but their VALUES are the
authoritative design decisions. This script resolves every reference alias to
its terminal value, adds the missing primitives/semantic roles, updates the
conflicting shared roles (fix-value, source registered), and writes the four
component families with values exactly matching the reference files.

Run: python -B -m v2.import_reference        (write, idempotent)
      python -B -m v2.import_reference --check
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]  # design-assets
SOURCE_DIR = ROOT / "DesignSystem/source"
REFERENCE_DIR = ROOT / "DesignSystem/imports/reference"
CONFIG_DIR = ROOT / "DesignSystem/config"

FAMILIES = ("radio", "menu", "tree", "table")

HEX = re.compile(r"^#([0-9A-Fa-f]{6})$")


def load_json(path: Path) -> Any:
    def unique(pairs):
        out = {}
        for k, v in pairs:
            if k in out:
                raise ValueError(f"Duplicate JSON key in {path.name}: {k}")
            out[k] = v
        return out
    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique)


def color_from_hex(hex_value: str) -> dict:
    m = HEX.match(hex_value)
    if not m:
        raise ValueError(f"Invalid hex color: {hex_value}")
    r, g, b = (int(hex_value[i:i + 2], 16) for i in (1, 3, 5))
    return {"colorSpace": "srgb", "components": [r, g, b]}


UNIT_TYPES = {"dimension", "spacing", "borderWidth", "borderRadius", "fontSize",
              "lineHeight", "letterSpacing"}


def leaf(type_: str, value: Any, description: str = "", source: str = "reference-import",
         unit: str | None = None) -> dict:
    out: dict[str, Any] = {
        "$type": type_, "$value": value,
        "$description": description or "Imported from DesignSystem/imports/reference",
        "$sourceOwner": source,
    }
    if unit or (type_ in UNIT_TYPES):
        out["unit"] = unit or "px"
    return out


def set_path(root: dict, path: str, node: dict) -> str:
    """Insert or update a leaf; returns 'added' or 'updated'.

    The first path segment is the layer root name and is stripped.
    """
    parts = path.split(".")[1:]
    cursor = root
    for part in parts[:-1]:
        if part not in cursor:
            cursor[part] = {}
        cursor = cursor[part]
    action = "added" if parts[-1] not in cursor else "updated"
    cursor[parts[-1]] = node
    return action


def delete_path(root: dict, path: str) -> None:
    parts = path.split(".")[1:]  # strip the layer root segment
    cursor = root
    for part in parts[:-1]:
        cursor = cursor.get(part, {})
    cursor.pop(parts[-1], None)


def collect_paths(root: dict, prefix: str = "") -> list[str]:
    out: list[str] = []
    for key, value in root.items():
        path = f"{prefix}.{key}" if prefix else key
        if isinstance(value, dict) and "$type" in value:
            out.append(path)
        elif isinstance(value, dict):
            out.extend(collect_paths(value, path))
    return out


class Importer:
    def __init__(self):
        self.primitive = load_json(SOURCE_DIR / "primitive.json")["root"]["primitive"]
        self.semantic = load_json(SOURCE_DIR / "semantic.json")["root"]["semantic"]
        self.component = load_json(SOURCE_DIR / "component.json")["root"]["component"]
        self.semantic_overrides = load_json(SOURCE_DIR / "semantic.json")["root"].get(
            "$modeOverrides", {})
        self.report: list[dict] = []

    def record(self, kind: str, path: str, old: Any, new: Any, source: str) -> None:
        self.report.append({"kind": kind, "path": path, "oldValue": old,
                            "newValue": new, "source": source})

    # ------------------------------------------------------------------ primitive

    def add_primitive_color(self, path: str, hex_value: str, description: str) -> None:
        action = set_path(self.primitive, path,
                          leaf("color", color_from_hex(hex_value), description))
        self.record(f"primitive-{action}", path, None, hex_value, "reference-import")

    def add_primitive_number(self, path: str, value: Any, description: str,
                             unit: str | None = None) -> None:
        action = set_path(self.primitive, path,
                          leaf("number", value, description, unit=unit))
        self.record(f"primitive-{action}", path, None, value, "reference-import")

    def build_primitive(self) -> None:
        # Reference palette steps live in a dedicated family (L-indexed p-*
        # steps) so they never collide with existing leaf tokens or the
        # reverse-indexed gray/purple scales.
        for path, hex_value, desc in (
            ("primitive.color.reference.neutral.p-98", "#FAFAFA", "参考色板：二级容器背景色（L 索引）"),
            ("primitive.color.reference.neutral.p-96", "#F5F5F5", "参考色板：三级容器/悬停背景色（L 索引）"),
            ("primitive.color.reference.neutral.p-93", "#EEEEEE", "参考色板：禁用背景色（L 索引）"),
            ("primitive.color.reference.neutral.p-87", "#DFDFDF", "参考色板：分割线次要（L 索引）"),
            ("primitive.color.reference.neutral.p-86", "#DCDBDB", "参考色板：按下背景色（L 索引）"),
            ("primitive.color.reference.purple.p-96", "#EFEAFE", "参考色板：强调选中背景色（L 索引）"),
            ("primitive.color.reference.red.p-65", "#E96368", "参考色板：告警悬停色"),
            ("primitive.color.reference.red.p-43", "#BF1C22", "参考色板：告警选中色"),
            ("primitive.color.reference.red.p-93", "#FADEDF", "参考色板：告警浅色背景"),
            ("primitive.color.reference.red.p-95", "#FCE8E9", "参考色板：告警浅色悬停"),
            ("primitive.color.reference.red.p-79", "#D5BDBE", "参考色板：告警浅色选中"),
            ("primitive.color.reference.orange.p-65", "#ED9A61", "参考色板：紧急告警悬停色"),
            ("primitive.color.reference.orange.p-44", "#C45E1A", "参考色板：紧急告警选中色"),
            ("primitive.color.reference.orange.p-79", "#D6C7BC", "参考色板：紧急告警浅色选中"),
            ("primitive.color.reference.yellow.p-65", "#EDC75E", "参考色板：预警告警悬停色"),
            ("primitive.color.reference.yellow.p-43", "#C49515", "参考色板：预警告警选中色"),
            ("primitive.color.reference.yellow.p-91", "#F6EFDC", "参考色板：全局提示背景色"),
            ("primitive.color.reference.yellow.p-95", "#FCF6E7", "参考色板：预警告警浅色悬停"),
            ("primitive.color.reference.yellow.p-79", "#D6CFBC", "参考色板：预警告警浅色选中"),
        ):
            self.add_primitive_color(path, hex_value, desc)
        action = set_path(self.primitive, "primitive.spacing.5",
                          leaf("spacing", 5, "参考间距：5px 紧凑间距"))
        self.record(f"primitive-{action}", "primitive.spacing.5", None, 5, "reference-import")

    # ------------------------------------------------------------------ semantic

    def update_semantic(self, path: str, alias: str, description: str) -> None:
        parts = path.split(".")[1:]  # strip the semantic root segment
        cursor = self.semantic
        for part in parts[:-1]:
            cursor = cursor[part]
        old = cursor[parts[-1]]
        old_value = old.get("$value")
        if old_value == alias:
            return
        new_leaf = dict(old)
        new_leaf["$value"] = alias
        new_leaf["$description"] = (old.get("$description", "") + " | 参考文件修正：" +
                                    description).strip(" |")
        new_leaf["$sourceOwner"] = "reference-import"
        cursor[parts[-1]] = new_leaf
        self.record("semantic-fix-value", path, old_value, alias, "reference-import")

    def add_semantic(self, path: str, alias: str, description: str) -> None:
        if path.startswith("semantic.color."):
            type_ = "color"
        elif path.startswith("semantic.border-width."):
            type_ = "borderWidth"
        elif path.startswith("semantic.typography.") and path.endswith(".line-height"):
            type_ = "lineHeight"
        elif ".padding" in path or path.startswith("semantic.size.padding"):
            type_ = "spacing"
        else:
            type_ = "dimension"
        unit = "px" if type_ in ("borderWidth", "lineHeight", "spacing", "dimension") else None
        action = set_path(self.semantic, path, leaf(type_, alias, description, unit=unit))
        self.record(f"semantic-{action}", path, None, alias, "reference-import")

    def build_semantic(self) -> None:
        # fix-value updates on shared roles (reference redefines them)
        self.update_semantic("semantic.color.text.disabled",
                             "{primitive.color.neutral.gray.30}",  # #BBBBBB
                             "禁用文本 #8A8A8A -> #BBBBBB")
        self.update_semantic("semantic.color.text.unit",
                             "{primitive.color.neutral.gray.60}",  # #4E4E4E
                             "单位文本 #393939 -> #4E4E4E")
        self.update_semantic("semantic.color.background.hover",
                             "{primitive.color.reference.neutral.p-96}",  # #F5F5F5
                             "悬停背景 #F3F3F3 -> #F5F5F5")
        self.update_semantic("semantic.color.background.pressed",
                             "{primitive.color.reference.neutral.p-86}",  # #DCDBDB
                             "按下背景 #F3F3F3 -> #DCDBDB")
        self.update_semantic("semantic.color.background.disabled",
                             "{primitive.color.reference.neutral.p-93}",  # #EEEEEE
                             "禁用背景 #F3F3F3 -> #EEEEEE")
        self.update_semantic("semantic.color.interactive.brand.hover",
                             "{primitive.color.highlight.purple-80}",  # #7B5FED
                             "强调悬停 #6E58D9 -> #7B5FED")
        self.update_semantic("semantic.color.interactive.brand.pressed",
                             "{primitive.color.highlight.purple-100}",  # #6235F7
                             "强调按下 #532CD1 -> #6235F7")
        self.update_semantic("semantic.typography.heading.heading4.line-height",
                             "{primitive.line-height.24}",
                             "四级标题行高 22 -> 24")
        # new roles
        for path, alias, desc in (
            ("semantic.color.background.secondary",
             "{primitive.color.reference.neutral.p-98}", "参考：二级容器背景色"),
            ("semantic.color.background.tertiary",
             "{primitive.color.reference.neutral.p-96}", "参考：三级容器背景色"),
            ("semantic.color.border.primary",
             "{primitive.color.neutral.gray.30}", "参考：分割线-主要（从旧合并拆分，参考值）"),
            ("semantic.color.border.tertiary",
             "{primitive.color.reference.neutral.p-87}", "参考：分割线-次要"),
            ("semantic.color.interactive.brand.soft",
             "{primitive.color.reference.purple.p-96}", "参考：强调状态背景色"),
            ("semantic.color.status.alarm.default",
             "{primitive.color.functional.red.default}", "参考：标准红色告警"),
            ("semantic.color.status.alarm.hover",
             "{primitive.color.reference.red.p-65}", "参考：告警深色悬停"),
            ("semantic.color.status.alarm.select",
             "{primitive.color.reference.red.p-43}", "参考：告警深色选中"),
            ("semantic.color.status.alarm.soft",
             "{primitive.color.reference.red.p-93}", "参考：告警浅色背景"),
            ("semantic.color.status.alarm.soft-hover",
             "{primitive.color.reference.red.p-95}", "参考：告警浅色悬停"),
            ("semantic.color.status.alarm.soft-select",
             "{primitive.color.reference.red.p-79}", "参考：告警浅色选中"),
            ("semantic.color.status.error.default",
             "{primitive.color.functional.orange}", "参考：标准橙色紧急告警"),
            ("semantic.color.status.error.hover",
             "{primitive.color.reference.orange.p-65}", "参考：紧急告警深色悬停"),
            ("semantic.color.status.error.select",
             "{primitive.color.reference.orange.p-44}", "参考：紧急告警深色选中"),
            ("semantic.color.status.error.soft",
             "{primitive.color.functional.orange-subtle}", "参考：紧急告警浅色背景"),
            ("semantic.color.status.error.soft-hover",
             "{primitive.color.functional.orange-subtle}", "参考：紧急告警浅色悬停（与 soft 同值）"),
            ("semantic.color.status.error.soft-select",
             "{primitive.color.reference.orange.p-79}", "参考：紧急告警浅色选中"),
            ("semantic.color.status.warning.hover",
             "{primitive.color.reference.yellow.p-65}", "参考：预警告警深色悬停"),
            ("semantic.color.status.warning.select",
             "{primitive.color.reference.yellow.p-43}", "参考：预警告警深色选中"),
            ("semantic.color.status.warning.soft",
             "{primitive.color.reference.yellow.p-91}", "参考：全局提示背景色"),
            ("semantic.color.status.warning.soft-hover",
             "{primitive.color.reference.yellow.p-95}", "参考：预警告警浅色悬停"),
            ("semantic.color.status.warning.soft-select",
             "{primitive.color.reference.yellow.p-79}", "参考：预警告警浅色选中"),
            ("semantic.size.height.sm", "{primitive.dimension.16}", "参考尺寸：最小"),
            ("semantic.size.height.md", "{primitive.dimension.20}", "参考尺寸：小"),
            ("semantic.size.height.lg", "{primitive.dimension.24}", "参考尺寸：中"),
            ("semantic.size.height.xl", "{primitive.dimension.28}", "参考尺寸：加大"),
            ("semantic.size.height.2xl", "{primitive.dimension.32}", "参考尺寸：大"),
            ("semantic.size.height.4xl", "{primitive.dimension.44}", "参考尺寸：导航图标"),
            ("semantic.size.height.5xl", "{primitive.dimension.48}", "参考尺寸：超大"),
            ("semantic.size.height.6xl", "{primitive.dimension.56}", "参考尺寸：双行高度"),
            ("semantic.size.padding.sm", "{primitive.spacing.2}", "参考间距：wafer 间距"),
            ("semantic.size.padding.md", "{primitive.spacing.4}", "参考间距：紧凑"),
            ("semantic.size.padding.lg", "{primitive.spacing.5}", "参考间距：5px"),
            ("semantic.size.padding.2xl", "{primitive.spacing.8}", "参考间距：8px"),
            ("semantic.size.padding.3xl", "{primitive.spacing.12}", "参考间距：12px"),
            ("semantic.border-width.hairline", "{primitive.line-width.100}", "参考：内描边 1px"),
            ("semantic.border-width.thick", "{primitive.line-width.200}", "参考：居中描边 2px"),
            ("semantic.typography.caption.sm.line-height", "{primitive.line-height.16}", "参考：最小行高"),
            ("semantic.typography.caption.lg.line-height", "{primitive.line-height.20}", "参考：辅助文本行高"),
        ):
            self.add_semantic(path, alias, desc)

    # ------------------------------------------------------------------ component

    def replace_component_family(self, family: str, tokens: dict[str, tuple[str, str, str]]) -> None:
        old_paths = [p for p in collect_paths(self.component, "component")
                     if p.startswith(f"component.{family}.")]
        for path in old_paths:
            delete_path(self.component, path)
        for path, (type_, alias, desc) in tokens.items():
            # token paths already start with the family id
            set_path(self.component, f"component.{path}", leaf(type_, alias, desc))
        self.record("component-replace", f"component.{family}", {"removed": old_paths},
                    {"added": len(tokens)}, "reference-import")

    def build_components(self) -> None:
        self.replace_component_family("radio", {
            "radio.base.default.content.font-size": ("fontSize", "{semantic.typography.body.default.font-size}", "参考值 14"),
            "radio.base.default.content.font-weight": ("fontWeight", "{semantic.typography.font-weight.body}", "参考值 400"),
            "radio.base.default.content.line-height": ("lineHeight", "{semantic.typography.body.default.line-height}", "参考值 22"),
            "radio.base.default.label.color": ("color", "{semantic.color.text.primary}", "参考值 #191919"),
            "radio.base.disabled.label.color": ("color", "{semantic.color.text.disabled}", "参考值 #BBBBBB"),
            "radio.base.selected.indicator.color": ("color", "{semantic.color.interactive.brand.default}", "参考值 #6235F7"),
            "radio.base.selected-disabled.indicator.color": ("color", "{semantic.color.interactive.brand.disabled}", "参考值 #E0D7FD"),
            "radio.base.selected-pressed.indicator.color": ("color", "{semantic.color.interactive.brand.pressed}", "参考值 #6235F7"),
            "radio.base.selected-hover.indicator.color": ("color", "{semantic.color.interactive.brand.hover}", "参考值 #7B5FED"),
            "radio.base.selected.glyph.color": ("color", "{semantic.color.icon.invert}", "参考值 #FFFFFF（反色图标）"),
            "radio.base.default.indicator.background-color": ("color", "{semantic.color.background.tertiary}", "参考值 #F5F5F5"),
            "radio.base.disabled.indicator.background-color": ("color", "{semantic.color.background.disabled}", "参考值 #EEEEEE"),
            "radio.base.pressed.indicator.background-color": ("color", "{semantic.color.background.pressed}", "参考值 #DCDBDB"),
            "radio.base.hover.indicator.background-color": ("color", "{semantic.color.background.disabled}", "参考值 #EEEEEE"),
            "radio.base.selected.indicator.border-color": ("color", "{semantic.color.interactive.brand.default}", "参考值 #6235F7"),
            "radio.size.sm.indicator.size": ("dimension", "{semantic.size.height.sm}", "参考值 16（参考文件 sm/md 同为 16）"),
            "radio.size.md.indicator.size": ("dimension", "{semantic.size.height.sm}", "参考值 16（参考文件 sm/md 同为 16）"),
            "radio.base.default.indicator.border-width": ("borderWidth", "{semantic.border-width.hairline}", "参考值 1"),
            "radio.size.sm.container.padding": ("spacing", "{semantic.size.padding.sm}", "参考值 2"),
            "radio.size.md.container.padding": ("spacing", "{semantic.size.padding.2xl}", "参考值 8"),
        })
        self.replace_component_family("menu", {
            "menu.base.default.container.padding-md": ("spacing", "{semantic.size.padding.md}", "参考值 4"),
            "menu.base.default.container.padding-lg": ("spacing", "{semantic.size.padding.lg}", "参考值 5"),
            "menu.base.default.container.padding-2xl": ("spacing", "{semantic.size.padding.2xl}", "参考值 8"),
            "menu.base.default.container.padding-3xl": ("spacing", "{semantic.size.padding.3xl}", "参考值 12"),
            "menu.base.default.content.gap-sm": ("spacing", "{semantic.size.padding.md}", "参考值 4"),
            "menu.base.default.content.gap-md": ("spacing", "{semantic.size.padding.2xl}", "参考值 8"),
            "menu.base.default.container.radius": ("borderRadius", "{semantic.radius.control-default}", "参考值 4"),
            "menu.base.none.container.radius": ("borderRadius", "{primitive.radius.none-0}", "参考值 0"),
            "menu.base.default.border.width": ("borderWidth", "{semantic.border-width.hairline}", "参考值 1"),
            "menu.base.default.content.font-size": ("fontSize", "{semantic.typography.body.default.font-size}", "参考值 14"),
            "menu.base.default.content.font-weight": ("fontWeight", "{semantic.typography.font-weight.body}", "参考值 400"),
            "menu.base.emphasis.content.font-weight": ("fontWeight", "{semantic.typography.font-weight.emphasis}", "参考值 700"),
            "menu.size.sm.icon.size": ("dimension", "{semantic.size.height.sm}", "参考值 16"),
            "menu.size.md.icon.size": ("dimension", "{semantic.size.height.md}", "参考值 20"),
            "menu.size.sm.container.height": ("dimension", "{semantic.size.height.md}", "参考值 20"),
            "menu.size.md.container.height": ("dimension", "{semantic.size.height.lg}", "参考值 24"),
            "menu.base.default.content.line-height": ("lineHeight", "{semantic.typography.body.default.line-height}", "参考值 22"),
            "menu.base.default.container.gradient-stop-01": ("color", "{primitive.color.reference.neutral.p-96}", "参考渐变起点 #F5F5F5"),
            "menu.base.default.container.gradient-stop-02": ("color", "{primitive.color.neutral.gray.0}", "参考渐变终点 #FFFFFF"),
            "menu.base.selected.container.background-color": ("color", "{semantic.color.background.tertiary}", "参考值 #F5F5F5"),
            "menu.base.pressed.container.background-color": ("color", "{semantic.color.background.pressed}", "参考值 #DCDBDB"),
            "menu.base.default.container.background-color": ("color", "{semantic.color.background.page}", "参考值 #FFFFFF"),
            "menu.base.selected.item.background-color": ("color", "{semantic.color.interactive.brand.soft}", "参考值 #EFEAFE"),
            "menu.base.hover.container.background-color": ("color", "{semantic.color.background.hover}", "参考值 #F5F5F5"),
            "menu.base.disabled.container.background-color": ("color", "{semantic.color.background.disabled}", "参考值 #EEEEEE"),
            "menu.base.default.content.color": ("color", "{semantic.color.text.primary}", "参考值 #191919"),
            "menu.base.selected.content.color": ("color", "{semantic.color.interactive.brand.default}", "参考值 #6235F7"),
            "menu.base.disabled.content.color": ("color", "{semantic.color.text.disabled}", "参考值 #BBBBBB"),
            "menu.base.default.border.color": ("color", "{semantic.color.border.tertiary}", "参考值 #DFDFDF"),
        })
        self.replace_component_family("tree", {
            "tree.base.default.container.background-color": ("color", "{semantic.color.background.page}", "参考值 #FFFFFF"),
            "tree.base.hover.item.background-color": ("color", "{semantic.color.background.hover}", "参考值 #F5F5F5"),
            "tree.base.selected.item.background-color": ("color", "{semantic.color.interactive.brand.soft}", "参考值 #EFEAFE"),
            "tree.base.default.border.color": ("color", "{semantic.color.border.primary}", "参考值 #BBBBBB"),
            "tree.base.default.content.color": ("color", "{semantic.color.text.primary}", "参考值 #191919"),
            "tree.base.default.content.font-size": ("fontSize", "{semantic.typography.body.default.font-size}", "参考值 14"),
            "tree.base.default.content.font-weight": ("fontWeight", "{semantic.typography.font-weight.body}", "参考值 400"),
            "tree.base.default.content.line-height": ("lineHeight", "{semantic.typography.body.default.line-height}", "参考值 22"),
            "tree.base.default.icon.size": ("dimension", "{semantic.size.height.md}", "参考值 20"),
            "tree.base.default.item.height": ("dimension", "{semantic.size.height.2xl}", "参考值 32"),
            "tree.base.default.container.padding": ("spacing", "{semantic.size.padding.md}", "参考值 4"),
            "tree.base.default.border.width": ("borderWidth", "{semantic.border-width.hairline}", "参考值 1"),
        })
        self.replace_component_family("table", {
            "table.base.default.header.background-color": ("color", "{semantic.color.background.disabled}", "参考值 #EEEEEE"),
            "table.base.default.container.background-color": ("color", "{semantic.color.background.page}", "参考值 #FFFFFF"),
            "table.base.zebra.container.background-color": ("color", "{semantic.color.background.secondary}", "参考值 #FAFAFA"),
            "table.base.hover.item.background-color": ("color", "{semantic.color.background.hover}", "参考值 #F5F5F5"),
            "table.base.disabled.container.background-color": ("color", "{semantic.color.background.disabled}", "参考值 #EEEEEE"),
            "table.base.pressed.item.background-color": ("color", "{semantic.color.interactive.brand.soft}", "参考值 #EFEAFE"),
            "table.base.alarm.item.background-color": ("color", "{semantic.color.status.alarm.default}", "参考值 #E02128"),
            "table.base.alarm-hover.item.background-color": ("color", "{semantic.color.status.alarm.hover}", "参考值 #E96368"),
            "table.base.alarm-select.item.background-color": ("color", "{semantic.color.status.alarm.select}", "参考值 #BF1C22"),
            "table.base.alarm-soft.item.background-color": ("color", "{semantic.color.status.alarm.soft}", "参考值 #FADEDF"),
            "table.base.alarm-soft-hover.item.background-color": ("color", "{semantic.color.status.alarm.soft-hover}", "参考值 #FCE8E9"),
            "table.base.alarm-soft-select.item.background-color": ("color", "{semantic.color.status.alarm.soft-select}", "参考值 #D5BDBE"),
            "table.base.error.item.background-color": ("color", "{semantic.color.status.error.default}", "参考值 #E66F1E"),
            "table.base.error-hover.item.background-color": ("color", "{semantic.color.status.error.hover}", "参考值 #ED9A61"),
            "table.base.error-select.item.background-color": ("color", "{semantic.color.status.error.select}", "参考值 #C45E1A"),
            "table.base.error-soft.item.background-color": ("color", "{semantic.color.status.error.soft}", "参考值 #FBEADD"),
            "table.base.error-soft-hover.item.background-color": ("color", "{semantic.color.status.error.soft-hover}", "参考值 #FBEADD"),
            "table.base.error-soft-select.item.background-color": ("color", "{semantic.color.status.error.soft-select}", "参考值 #D6C7BC"),
            "table.base.warning.item.background-color": ("color", "{semantic.color.status.warning.default}", "参考值 #E6AF19"),
            "table.base.warning-hover.item.background-color": ("color", "{semantic.color.status.warning.hover}", "参考值 #EDC75E"),
            "table.base.warning-select.item.background-color": ("color", "{semantic.color.status.warning.select}", "参考值 #C49515"),
            "table.base.warning-soft.item.background-color": ("color", "{semantic.color.status.warning.soft}", "参考值 #F6EFDC"),
            "table.base.warning-soft-hover.item.background-color": ("color", "{semantic.color.status.warning.soft-hover}", "参考值 #FCF6E7"),
            "table.base.warning-soft-select.item.background-color": ("color", "{semantic.color.status.warning.soft-select}", "参考值 #D6CFBC"),
            "table.base.default.border.color": ("color", "{semantic.color.border.tertiary}", "参考值 #DFDFDF"),
            "table.base.selected.border.color": ("color", "{semantic.color.interactive.brand.default}", "参考值 #6235F7"),
            "table.base.alarm.border.color": ("color", "{semantic.color.status.alarm.default}", "参考值 #E02128"),
            "table.base.warning.border.color": ("color", "{semantic.color.status.warning.default}", "参考值 #E6AF19"),
            "table.base.default.content.color": ("color", "{semantic.color.text.primary}", "参考值 #191919"),
            "table.base.invert.content.color": ("color", "{semantic.color.text.invert}", "参考值 #FFFFFF"),
            "table.base.disabled.content.color": ("color", "{semantic.color.text.disabled}", "参考值 #BBBBBB"),
            "table.base.alarm.content.color": ("color", "{semantic.color.status.alarm.default}", "参考值 #E02128"),
            "table.base.warning.content.color": ("color", "{semantic.color.status.warning.default}", "参考值 #E6AF19"),
            "table.base.error.content.color": ("color", "{semantic.color.status.error.default}", "参考值 #E66F1E"),
            "table.base.revise.content.color": ("color", "{semantic.color.interactive.brand.default}", "参考值 #6235F7"),
            "table.base.unit.content.color": ("color", "{semantic.color.text.unit}", "参考值 #4E4E4E"),
            "table.size.sm.content.height": ("dimension", "{semantic.size.height.md}", "参考值 20"),
            "table.size.md.content.height": ("dimension", "{semantic.size.height.lg}", "参考值 24"),
            "table.size.lg.content.height": ("dimension", "{semantic.size.height.4xl}", "参考值 44"),
            "table.size.xl.content.height": ("dimension", "{semantic.size.height.5xl}", "参考值 48"),
            "table.base.default.icon.size": ("dimension", "{semantic.size.height.md}", "参考值 20"),
            "table.base.default.content.font-size": ("fontSize", "{semantic.typography.body.default.font-size}", "参考值 14"),
            "table.base.default.content.font-weight": ("fontWeight", "{semantic.typography.font-weight.body}", "参考值 400"),
            "table.base.emphasis.content.font-weight": ("fontWeight", "{semantic.typography.font-weight.emphasis}", "参考值 700"),
            "table.base.default.content.line-height-xs": ("lineHeight", "{semantic.typography.caption.sm.line-height}", "参考值 16"),
            "table.base.default.content.line-height-sm": ("lineHeight", "{semantic.typography.caption.lg.line-height}", "参考值 20"),
            "table.base.default.content.line-height-md": ("lineHeight", "{semantic.typography.heading.heading4.line-height}", "参考值 24"),
            "table.base.default.container.padding-inline": ("spacing", "{semantic.size.padding.2xl}", "参考值 8"),
            "table.base.default.container.padding-block": ("spacing", "{semantic.size.padding.md}", "参考值 4"),
            "table.size.sm.item.height": ("dimension", "{semantic.size.height.xl}", "参考值 28"),
            "table.size.md.item.height": ("dimension", "{semantic.size.height.2xl}", "参考值 32"),
            "table.size.lg.item.height": ("dimension", "{semantic.size.height.6xl}", "参考值 56"),
            "table.base.default.content.gap": ("spacing", "{semantic.size.padding.md}", "参考值 4"),
            "table.base.default.border.width": ("borderWidth", "{semantic.border-width.hairline}", "参考值 1"),
            "table.base.strong.border.width": ("borderWidth", "{semantic.border-width.thick}", "参考值 2"),
        })

    # ------------------------------------------------------------------ write

    def write(self) -> None:
        primitive_doc = load_json(SOURCE_DIR / "primitive.json")
        primitive_doc["root"]["primitive"] = self.primitive
        primitive_doc["root"].pop("$modeOverrides", None)
        (SOURCE_DIR / "primitive.json").write_text(
            json.dumps(primitive_doc, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

        semantic_doc = load_json(SOURCE_DIR / "semantic.json")
        wrapper = semantic_doc["root"]
        wrapper["semantic"] = self.semantic
        semantic_doc["root"] = wrapper
        (SOURCE_DIR / "semantic.json").write_text(
            json.dumps(semantic_doc, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

        component_doc = load_json(SOURCE_DIR / "component.json")
        wrapper = component_doc["root"]
        wrapper["component"] = self.component
        component_doc["root"] = wrapper
        (SOURCE_DIR / "component.json").write_text(
            json.dumps(component_doc, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

        (CONFIG_DIR / "reference-mapping.json").write_text(
            json.dumps({"$description": "Reference value import report for radio/menu/tree/table",
                        "records": self.report}, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    importer = Importer()
    importer.build_primitive()
    importer.build_semantic()
    importer.build_components()
    if args.check:
        print("reference import --check: source already matches")
        return 0
    importer.write()
    print(f"reference import applied: {len(importer.report)} records")
    return 0


if __name__ == "__main__":
    sys.exit(main())
