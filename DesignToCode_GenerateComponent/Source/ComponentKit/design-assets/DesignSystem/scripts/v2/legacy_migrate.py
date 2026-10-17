"""One-shot V2 source builder: migrates the frozen legacy sToken design export
(DesignSystem/imports/stoken.json) into the canonical V2 source files.

This script is provenance tooling, not part of the runtime compile chain. It
runs once per legacy snapshot (or re-runs idempotently with --check). The V2
source files it writes are the daily write target; the legacy export becomes a
read-only design snapshot registered in the manifest.

Implemented V2 contract decisions:
- four roots: primitive / profile / semantic / component (+ product scope)
- kebab-case names, registered enums only
- property `background` -> `background-color` when it is a plain color
- input size enum sm/md/lg -> registered enum small/medium/large
- border/primary merged into border/default (same value, one trunk)
- cross-component aliases re-pointed to their semantic/primitive terminal
  target (value-preserving; no horizontal inheritance)
- 17 application-layout keys moved to the product scope package
- semantic typography aggregated by role/variant (font-size/line-height leaves)
- DTCG-style typed values; colors srgb 8-bit components; explicit units
- Dark remains sparse (9 design roles) and is NOT publishable until expanded
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]  # design-assets
STOKEN = ROOT / "DesignSystem/imports/stoken.json"
SOURCE_DIR = ROOT / "DesignSystem/source"
CONFIG_DIR = ROOT / "DesignSystem/config"

VAR = re.compile(r"^var\((--[^()]+)\)$")
RGBA = re.compile(r"^rgba\((\d+),\s*(\d+),\s*(\d+),\s*([\d.]+)\)$")
STEP = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")


def load_json(path: Path) -> Any:
    def unique(pairs):
        out = {}
        for k, v in pairs:
            if k in out:
                raise ValueError(f"Duplicate JSON key in {path.name}: {k}")
            out[k] = v
        return out

    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique)


def num(value: Any) -> Any:
    if isinstance(value, float):
        return round(value, 6)
    return value


def color_value(rgba: str) -> dict:
    m = RGBA.match(rgba)
    if not m:
        raise ValueError(f"Unsupported legacy color literal: {rgba}")
    comp = {"colorSpace": "srgb", "components": [int(m.group(1)), int(m.group(2)), int(m.group(3))]}
    alpha = float(m.group(4))
    if alpha < 1:
        comp["alpha"] = round(alpha, 6)
    return comp


def set_path(root: dict, path: str, leaf: dict) -> None:
    node = root
    parts = path.split(".")
    for part in parts[:-1]:
        if not STEP.fullmatch(part):
            raise ValueError(f"Invalid canonical path segment {part!r} in {path}")
        node = node.setdefault(part, {})
    if not STEP.fullmatch(parts[-1]):
        raise ValueError(f"Invalid canonical leaf {parts[-1]!r} in {path}")
    if parts[-1] in node:
        raise ValueError(f"Duplicate canonical path: {path}")
    node[parts[-1]] = leaf


def strip_legacy(name: str) -> str:
    if not name.startswith("--"):
        raise ValueError(f"Legacy key missing --: {name}")
    return name[2:].replace("/", ".")


class LegacyExport:
    def __init__(self, path: Path):
        doc = load_json(path)
        self.collections: dict[str, dict[str, dict[str, Any]]] = {}
        for entry in doc:
            for collection, modes in entry.items():
                if collection in self.collections:
                    raise ValueError(f"Duplicate collection: {collection}")
                self.collections[collection] = {}
                for mode in modes:
                    for name, values in mode.items():
                        if name in self.collections[collection]:
                            raise ValueError(f"Duplicate mode: {collection}/{name}")
                        self.collections[collection][name] = values
        assert tuple(self.collections) == ("01.primitive", "02.profile", "03.semantic", "04.component")
        self.primitive = self.collections["01.primitive"]["参数"]
        self.profile = {
            "standard": self.collections["02.profile"]["Standard"],
            "wide": self.collections["02.profile"]["Wide"],
        }
        self.semantic = {
            "light": self.collections["03.semantic"]["Light"],
            "dark": self.collections["03.semantic"]["Dark"],
        }
        self.component = self.collections["04.component"]["default"]
        self.sha256 = hashlib.sha256(path.read_bytes()).hexdigest()


# ---------------------------------------------------------------------------
# Canonical path computation (pass 1)
# ---------------------------------------------------------------------------

def primitive_path(name: str) -> str:
    key = strip_legacy(name)
    m = re.match(r"^color\.neutral\.gray-(\d+)$", key)
    if m:
        return f"primitive.color.neutral.gray.{m.group(1)}"
    m = re.match(r"^color\.(.+)$", key)
    if m:
        return "primitive.color." + m.group(1)
    m = re.match(r"^dimension\.dimension-(\d+)$", key)
    if m:
        return f"primitive.dimension.{m.group(1)}"
    m = re.match(r"^spacing\.spacing-(\d+)$", key)
    if m:
        return f"primitive.spacing.{m.group(1)}"
    m = re.match(r"^(radius|line-width|opacity|boolean|font-family|font-weight|font-size|line-height|letter-spacing|paragraph-spacing)\.(.+)$", key)
    if m:
        return f"primitive.{m.group(1)}.{m.group(2)}"
    raise ValueError(f"Unhandled primitive key: {name}")


def profile_path(name: str) -> str:
    key = strip_legacy(name)
    m = re.match(r"^application-layout\.(.+)$", key)
    if m:
        return "product.application-layout." + m.group(1)
    m = re.match(r"^(visualization-size|interaction-size|viewport|grid|spacing-scale|safe-area|typography-scale)\.(.+)$", key)
    if m:
        return f"profile.{m.group(1)}.{m.group(2)}"
    raise ValueError(f"Unhandled profile key: {name}")


def semantic_path(name: str) -> str:
    key = strip_legacy(name)
    if key == "color.border.primary":
        return "semantic.color.border.default"  # merged
    m = re.match(r"^color\.(.+)$", key)
    if m:
        return "semantic.color." + m.group(1)
    m = re.match(r"^typography\.font-family\.(.+)$", key)
    if m:
        return "semantic.typography.font-family." + m.group(1)
    m = re.match(r"^typography\.font-weight\.(.+)$", key)
    if m:
        return "semantic.typography.font-weight." + m.group(1)
    m = re.match(r"^typography\.letter-spacing\.(.+)$", key)
    if m:
        return "semantic.typography.letter-spacing." + m.group(1)
    m = re.match(r"^typography\.(font-size|line-height)\.([a-z0-9-]+)\.([a-z0-9-]+)$", key)
    if m:
        return f"semantic.typography.{m.group(2)}.{m.group(3)}.{m.group(1)}"
    m = re.match(r"^metric\.border-width\.(.+)$", key)
    if m:
        return "semantic.border-width." + m.group(1)
    m = re.match(r"^metric\.radius\.(.+)$", key)
    if m:
        return "semantic.radius." + m.group(1)
    m = re.match(r"^metric\.interaction-size\.(.+)$", key)
    if m:
        return "semantic.interaction-size." + m.group(1)
    m = re.match(r"^metric\.visualization-size\.(.+)$", key)
    if m:
        return "semantic.visualization-size." + m.group(1)
    m = re.match(r"^metric\.space\.(.+)$", key)
    if m:
        return "semantic.space." + m.group(1)
    m = re.match(r"^opacity\.(.+)$", key)
    if m:
        return "semantic.opacity." + m.group(1)
    m = re.match(r"^layout\.(.+)$", key)
    if m:
        return "semantic.layout." + m.group(1)
    m = re.match(r"^asset\.(.+)$", key)
    if m:
        return "semantic.asset." + m.group(1)
    raise ValueError(f"Unhandled semantic key: {name}")


SIZE_RENAME = {"sm": "small", "md": "medium", "lg": "large"}


def component_path(name: str) -> str:
    key = strip_legacy(name)
    parts = key.split(".")
    parts[0] = "component." + parts[0]
    # registered size enum: small/medium/large
    for i, part in enumerate(parts):
        if part in SIZE_RENAME:
            parts[i] = SIZE_RENAME[part]
    # property background -> background-color when the property slot is a color
    path = ".".join(parts)
    path = re.sub(r"\.background$", ".background-color", path)
    return path


# ---------------------------------------------------------------------------
# Builder
# ---------------------------------------------------------------------------

KIND_INFO = {
    "color": ("color", None),
    "dimension": ("dimension", "px"),
    "number": ("number", None),
    "fontFamily": ("fontFamily", None),
    "fontWeight": ("fontWeight", None),
    "fontSize": ("fontSize", "px"),
    "lineHeight": ("lineHeight", "px"),
    "letterSpacing": ("letterSpacing", "px"),
    "spacing": ("spacing", "px"),
    "borderRadius": ("borderRadius", "px"),
    "borderWidth": ("borderWidth", "px"),
    "opacity": ("opacity", "%"),
    "boolean": ("boolean", None),
    "asset": ("asset", None),
    "string": ("string", None),
}

TYPE_KINDS = {
    "color": "color", "dimension": "dimension", "number": "number",
    "fontFamily": "fontFamily", "fontWeight": "fontWeight", "fontSize": "fontSize",
    "lineHeight": "lineHeight", "letterSpacing": "letterSpacing",
    "spacing": "spacing", "borderRadius": "borderRadius", "borderWidth": "borderWidth",
    "opacity": "opacity", "boolean": "boolean", "asset": "asset", "string": "string",
}

LEGACY_KIND = {
    "radius": "borderRadius", "line-width": "borderWidth",
    "spacing": "spacing", "dimension": "dimension", "opacity": "opacity",
    "font-family": "fontFamily", "font-weight": "fontWeight",
    "font-size": "fontSize", "line-height": "lineHeight",
    "letter-spacing": "letterSpacing", "paragraph-spacing": "spacing",
    "boolean": "boolean", "color": "color", "asset": "asset",
}

FIGMA_PROGRESS_KEYS = {
    "progress-bar/state/paused/indicator/color": "primitive.color.functional.yellow",
    "progress-bar/state/error/indicator/color": "primitive.color.functional.red.default",
    "progress-bar/state/in-progress/indicator/color": "primitive.color.highlight.purple-100",
    "progress-bar/state/completed/indicator/color": "primitive.color.functional.green.60",
}

FIGMA_VISUALIZATION = {"01": (16, 16), "02": (18, 18), "03": (20, 20), "04": (24, 24),
                       "05": (28, 28), "06": (32, 32), "07": (36, 36), "08": (40, 40)}


class Builder:
    def __init__(self, x: LegacyExport):
        self.x = x
        self.docs = {k: {} for k in ("primitive", "profile", "semantic", "component", "product")}
        self.legacy: list[dict] = []
        self.pending: list[dict] = []
        self.private: list[dict] = []
        self.semantic_index: dict[str, Any] = {}
        self.component_aliases: dict[str, str] = {}  # canonical comp path -> legacy var target
        self.component_namespace = {strip_legacy(n).split(".", 1)[0] for n in x.component}
        self.target_map: dict[str, str] = {}
        for name in x.primitive:
            self.target_map[name] = primitive_path(name)
        for name in x.profile["standard"]:
            self.target_map[name] = profile_path(name)
        for name in x.semantic["light"]:
            self.target_map[name] = semantic_path(name)

    def rewrite_target(self, legacy_target: str) -> str:
        canonical = self.target_map.get(legacy_target)
        if canonical is None:
            raise ValueError(f"Unresolved legacy target: {legacy_target}")
        return canonical

    # -- helpers ---------------------------------------------------------------

    def record(self, key: str, decision: str, new: str, reason: str,
               layer: str, kind: str, consumers: str = "", status: str = "migrated") -> None:
        self.legacy.append({
            "legacyKey": key, "decision": decision, "canonicalPath": new,
            "reason": reason, "layer": layer, "kind": kind,
            "consumers": consumers, "status": status,
        })

    def leaf(self, kind: str, value: Any, legacy_name: str, extra: dict | None = None) -> dict:
        t, unit = KIND_INFO[kind]
        leaf: dict[str, Any] = {"$type": t, "$value": value,
                                "$description": f"Migrated from legacy sToken {legacy_name}"}
        if unit:
            leaf["unit"] = unit
        if extra:
            leaf.update(extra)
        return leaf

    def literal_kind(self, legacy_name: str, value: Any) -> str:
        key = strip_legacy(legacy_name)
        if RGBA.match(str(value)):
            return "color"
        for grp, kind in LEGACY_KIND.items():
            if key == grp or key.startswith(grp + "."):
                return kind
        if isinstance(value, bool):
            return "boolean"
        if isinstance(value, (int, float)):
            return "dimension"
        return "string"

    # -- pass 1: primitive -----------------------------------------------------

    def build_primitive(self) -> None:
        doc = self.docs["primitive"]
        for name, value in sorted(self.x.primitive.items()):
            p = primitive_path(name)
            kind = self.literal_kind(name, value)
            if kind == "color":
                v: Any = color_value(str(value))
            else:
                v = num(value)
            set_path(doc, p, self.leaf(kind, v, name))
            decision = "rename" if name != "--" + p[len("primitive."):].replace(".", "/") else "keep"
            self.record(name, decision, p, "primitive literal", "primitive", kind)

    # -- pass 1: profile + product ---------------------------------------------

    def build_profile(self) -> None:
        doc = self.docs["profile"]
        product = self.docs["product"]
        for name, std in sorted(self.x.profile["standard"].items()):
            wide = self.x.profile["wide"][name]
            p = profile_path(name)
            if p.startswith("product."):
                target = product
            else:
                target = doc
            kind = self.profile_kind(p, std)
            if kind == "number":
                # unit-less counts/ratios: declared as literal numbers with the
                # same numeric value as the legacy dimension alias (type fix).
                set_path(target, p, self.leaf(kind, self.dimension_value(std), name))
                self.record(name, "fix-value", p,
                            "count/ratio carried as a dimension alias in the legacy export; "
                            "declared as a unit-less number with the same numeric value "
                            "(type correction, not a value change)",
                            "profile", kind)
                continue
            set_path(target, p, self.leaf(kind, alias_of(self.rewrite_target(self.var(std))), name))
            wide_value = self.rewrite_target(self.var(wide))
            self.mode_override(target, "wide", p, {"$value": alias_of(wide_value)})
            if p.startswith("product."):
                self.record(name, "move", p,
                            "product-owned application layout; moved to product scope, not public profile",
                            "product", kind, consumers="ComponentLab application shell",
                            status="product-scope")
            else:
                self.record(name, "rename", p, "profile indexed scale step", "profile", kind)
            if re.search(r"visualization-size\.\d{2}$", p):
                self.pending_visualization(name, std, wide, p)

    def profile_kind(self, p: str, value: Any) -> str:
        if p.endswith("output-scale") or p.endswith("column-count"):
            return "number"
        if "/font-size/" in "/" + p or p.startswith("profile.typography-scale.font-size"):
            return "fontSize"
        if "letter-spacing" in p:
            return "letterSpacing"
        if "line-height" in p:
            return "lineHeight"
        if "spacing-scale" in p:
            return "spacing"
        return "dimension"

    def var(self, legacy_value: Any) -> str:
        m = VAR.fullmatch(str(legacy_value))
        if not m:
            raise ValueError(f"Expected var() alias, got: {legacy_value}")
        return m.group(1)

    def mode_override(self, doc: dict, mode: str, path: str, leaf: dict) -> None:
        doc.setdefault("$modeOverrides", {}).setdefault(mode, {})[path] = leaf

    def pending_visualization(self, name: str, std: Any, wide: Any, p: str) -> None:
        step = strip_legacy(name).split(".")[1]
        f_std, f_wide = FIGMA_VISUALIZATION[step]
        v_std = self.dimension_value(std)
        v_wide = self.dimension_value(wide)
        if (v_std, v_wide) != (f_std, f_wide):
            self.pending.append({
                "canonicalPath": p, "legacyKey": name, "kind": "conflict-visualization-size",
                "figmaStandard": f_std, "figmaWide": f_wide,
                "fileStandard": v_std, "fileWide": v_wide,
                "decision": "keep-file-as-runtime-baseline",
                "reason": "Diverging scale steps are unverified against binding consumers; values are not changed without verification. Figma values registered; Wide consumers gated.",
                "gate": "wide-context-consumers-blocked", "status": "pending-decision",
            })

    def dimension_value(self, legacy_value: Any) -> int:
        target = self.var(legacy_value)
        key = strip_legacy(target)
        if key.startswith("dimension.dimension-"):
            return int(key.split("dimension-")[1])
        if key.startswith("font-size."):
            return int(key.split(".")[1])
        if key.startswith("line-height."):
            return int(key.split(".")[1])
        raise ValueError(f"Unsupported dimension alias: {target}")

    # -- pass 1: semantic -------------------------------------------------------

    def build_semantic(self) -> None:
        doc = self.docs["semantic"]
        for name, value in sorted(self.x.semantic["light"].items()):
            p = semantic_path(name)
            if name == "--color/border/primary":
                self.record(name, "merge", p,
                            "border/primary and border/default are one role with one value; single trunk kept, consumers re-pointed",
                            "semantic", "color", consumers="input, select, legacy runtime")
                continue
            kind = self.semantic_kind(p, value)
            if VAR.fullmatch(str(value)):
                v: Any = alias_of(self.rewrite_target(self.var(value)))
            else:
                v = value if kind in ("asset", "fontFamily") else num(value)
            set_path(doc, p, self.leaf(kind, v, name))
            self.semantic_index[p] = (kind, value)
            self.record(name, "keep" if p == "semantic." + strip_legacy(name) else "rename", p,
                        "semantic role", "semantic", kind)
        for name, value in sorted(self.x.semantic["dark"].items()):
            key = strip_legacy(name)
            assert key.startswith("color."), f"Dark override must be a color role: {name}"
            p = "semantic.color." + key[6:]
            self.mode_override(doc, "dark", p, {"$value": alias_of(self.rewrite_target(self.var(value))),
                                                "$description": f"Design Dark override from {name}"})
            self.record("03.semantic/Dark:" + name, "rebind", p,
                        "explicit design Dark override; Light inherits for everything else",
                        "semantic", "color", status="dark-mode-sparse")

    def semantic_kind(self, p: str, value: Any) -> str:
        if p.startswith("semantic.asset."):
            return "asset"
        if p.startswith("semantic.typography.font-family"):
            return "fontFamily"
        if p.startswith("semantic.typography.font-weight"):
            return "fontWeight"
        if p.startswith("semantic.typography.letter-spacing"):
            return "letterSpacing"
        if re.search(r"\.font-size$", p):
            return "fontSize"
        if re.search(r"\.line-height$", p):
            return "lineHeight"
        if p.startswith("semantic.border-width."):
            return "borderWidth"
        if p.startswith("semantic.radius."):
            return "borderRadius"
        if p.startswith("semantic.opacity."):
            return "opacity"
        if p.startswith("semantic.color."):
            return "color"
        if p.startswith("semantic.interaction-size.") or p.startswith("semantic.visualization-size."):
            return "dimension"
        # layout: follow terminal through profile grid column-count (number)
        m = VAR.fullmatch(str(value))
        if m:
            target = self.rewrite_target(m.group(1))
            if target.endswith("column-count") or target.endswith("output-scale"):
                return "number"
        return "spacing"

    # -- pass 1: component -------------------------------------------------------

    def build_component(self) -> None:
        doc = self.docs["component"]
        for name, value in sorted(self.x.component.items()):
            p = component_path(name)
            target = self.var(value)
            self.component_aliases[p] = target
            kind = self.component_kind(p, target)
            set_path(doc, p, self.leaf(kind, None, name))  # value finalized in pass 2
            self.record(name, "rename", p, "component token re-homed to canonical root",
                        "component", kind)

    def component_kind(self, p: str, target: str) -> str:
        terminal = self.component_terminal(target)
        return terminal["kind"]

    def component_terminal(self, target: str) -> dict:
        """Follow the legacy alias chain to a non-component terminal."""
        seen: list[str] = []
        while True:
            if target in seen:
                raise ValueError(f"Component alias cycle: {' -> '.join(seen + [target])}")
            seen.append(target)
            key = strip_legacy(target)
            if key.split(".", 1)[0] in self.component_namespace:
                target = self.var(self.x.component[target])
                continue
            canonical = self.rewrite_target(target)
            return self.terminal_kind(canonical)

    def terminal_kind(self, canonical: str) -> dict:
        for name in ("primitive", "profile", "product"):
            if canonical.startswith(name + "."):
                return {"kind": self.kind_from_path(canonical), "directLayer": name}
        if canonical.startswith("semantic."):
            info = self.semantic_index.get(canonical)
            if info:
                kind, _ = info
                return {"kind": kind, "directLayer": "semantic"}
            if canonical.startswith("semantic.color."):
                return {"kind": "color", "directLayer": "semantic"}
        raise ValueError(f"Cannot infer terminal kind for {canonical}")

    def kind_from_path(self, canonical: str) -> str:
        if canonical.startswith("primitive.color."):
            return "color"
        if canonical.startswith("primitive.dimension."):
            return "dimension"
        if canonical.startswith("primitive.spacing."):
            return "spacing"
        if canonical.startswith("primitive.radius."):
            return "borderRadius"
        if canonical.startswith("primitive.line-width."):
            return "borderWidth"
        if canonical.startswith("primitive.opacity."):
            return "opacity"
        if canonical.startswith("primitive.font-family."):
            return "fontFamily"
        if canonical.startswith("primitive.font-weight."):
            return "fontWeight"
        if canonical.startswith("primitive.font-size."):
            return "fontSize"
        if canonical.startswith("primitive.line-height."):
            return "lineHeight"
        if canonical.startswith("primitive.letter-spacing."):
            return "letterSpacing"
        if canonical.startswith("primitive.paragraph-spacing."):
            return "spacing"
        if canonical.startswith("primitive.boolean."):
            return "boolean"
        if canonical.startswith("profile."):
            if canonical.endswith("output-scale") or canonical.endswith("column-count"):
                return "number"
            if "font-size" in canonical:
                return "fontSize"
            if "line-height" in canonical:
                return "lineHeight"
            if "letter-spacing" in canonical:
                return "letterSpacing"
            if "spacing-scale" in canonical:
                return "spacing"
            return "dimension"
        if canonical.startswith("product."):
            return "dimension"
        raise ValueError(f"Unknown canonical kind path: {canonical}")

    # -- pass 2: finalize component aliases --------------------------------------

    def finalize_component(self) -> None:
        doc = self.docs["component"]
        for p, target in sorted(self.component_aliases.items()):
            owner = p.split(".")[1]
            terminal = self.component_terminal(target)
            terminal_path = self._component_terminal_path(target)
            # re-point through the chain to the non-component terminal
            if terminal["directLayer"] == "primitive":
                self.private.append({
                    "componentPath": p, "owner": owner, "target": terminal_path,
                    "reason": "component-private material or fixed construction value; registered direct primitive reference",
                })
            value = alias_of(terminal_path)
            self._set_leaf_value(doc, p, value, owner, terminal["kind"])

    def _component_terminal_path(self, target: str) -> str:
        seen: list[str] = []
        while True:
            if target in seen:
                raise ValueError(f"Component alias cycle: {' -> '.join(seen + [target])}")
            seen.append(target)
            key = strip_legacy(target)
            if key.split(".", 1)[0] in self.component_namespace:
                target = self.var(self.x.component[target])
                continue
            return self.rewrite_target(target)

    def _set_leaf_value(self, doc: dict, p: str, value: Any, owner: str, kind: str) -> None:
        node = doc
        parts = p.split(".")
        for part in parts[:-1]:
            node = node[part]
        leaf = node[parts[-1]]
        leaf["$value"] = value
        leaf["$owner"] = owner
        if leaf["$type"] != TYPE_KINDS[kind]:
            raise ValueError(f"Kind mismatch for {p}: {leaf['$type']} vs {kind}")
        if "unit" not in leaf and KIND_INFO[kind][1]:
            leaf["unit"] = KIND_INFO[kind][1]

    # -- Figma-only additions -----------------------------------------------------

    def add_figma_only(self) -> None:
        doc = self.docs["component"]
        for name, target in FIGMA_PROGRESS_KEYS.items():
            p = "component." + name.replace("/", ".")
            set_path(doc, p, {
                "$type": "color",
                "$value": alias_of(target),
                "$owner": "progress-bar",
                "$sourceOwner": "figma-canvas-audit",
                "$description": "Figma-only progress state indicator color; not present in the legacy export; runtime support unverified",
            })
            self.record("figma:" + name, "rebind", p,
                        "Figma canvas binding found in the design file but missing from the legacy export",
                        "component", "color", consumers="Qt Z_ProgressBar / ProgressAppearance",
                        status="design-defined-runtime-unverified")
            self.private.append({
                "componentPath": p, "owner": "progress-bar", "target": target,
                "reason": "state indicator color from Figma audit; component-private material",
            })


def alias_of(path: str) -> str:
    return "{" + path + "}"


def count_leaves(doc: dict) -> int:
    count = 0
    for key, value in doc.items():
        if key == "$modeOverrides":
            for entries in value.values():
                count += len(entries)
            continue
        if isinstance(value, dict) and "$type" not in value:
            count += count_leaves(value)
        else:
            count += 1
    return count


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    x = LegacyExport(STOKEN)
    b = Builder(x)
    b.build_primitive()
    b.build_profile()
    b.build_semantic()
    b.build_component()
    b.finalize_component()
    b.add_figma_only()

    files = {
        "primitive.json": {"layer": "primitive", "root": b.docs["primitive"]},
        "profile.json": {"layer": "profile", "root": b.docs["profile"]},
        "semantic.json": {"layer": "semantic", "root": b.docs["semantic"]},
        "component.json": {"layer": "component", "root": b.docs["component"]},
    }
    product_doc = b.docs["product"]
    if product_doc:
        files["product/application-layout.json"] = {
            "layer": "product", "product": "ComponentLab", "root": product_doc,
        }

    report = {
        "$description": "V2 migration decisions. Every legacy key has a recorded decision; pending conflicts keep both ends of evidence.",
        "sha256": x.sha256,
        "pendingConflicts": b.pending,
        "privateMaterial": b.private,
        "decisions": b.legacy,
    }

    if args.check:
        _check(files, report)
        print("V2 migration source matches legacy snapshot")
        return 0

    for rel, payload in files.items():
        target = SOURCE_DIR / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "$schema": "../../schema/v2-token-schema.json" if rel.startswith("product/")
            else "../schema/v2-token-schema.json",
            "$description": f"ComponentLab V2 canonical token layer: {payload['layer']}",
            **payload,
        }
        target.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    (CONFIG_DIR / "legacy-map.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print(f"primitive={count_leaves(b.docs['primitive'])} profile={count_leaves(b.docs['profile'])} "
          f"semantic={count_leaves(b.docs['semantic'])} component={count_leaves(b.docs['component'])} "
          f"product={count_leaves(b.docs['product'])} legacy={len(b.legacy)} pending={len(b.pending)} "
          f"private={len(b.private)}")
    return 0


def _check(files: dict, report: dict) -> None:
    for rel, payload in files.items():
        target = SOURCE_DIR / rel
        if not target.exists():
            raise ValueError(f"Missing V2 source file: {target}")
    legacy = load_json(CONFIG_DIR / "legacy-map.json")
    if legacy.get("sha256") != report["sha256"]:
        raise ValueError("legacy-map.json snapshot hash drift")
    if legacy.get("decisions") != report["decisions"]:
        raise ValueError("legacy-map.json decision drift")
    if legacy.get("pendingConflicts") != report["pendingConflicts"]:
        raise ValueError("legacy-map.json pending conflicts drift")


if __name__ == "__main__":
    sys.exit(main())
