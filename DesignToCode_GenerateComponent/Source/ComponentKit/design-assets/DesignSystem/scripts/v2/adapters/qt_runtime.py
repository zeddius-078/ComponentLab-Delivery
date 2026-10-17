"""Qt runtime adapter: emits tokens_qt.json with the V2 canonical contexts plus
a one-way legacy compatibility overlay.

Envelope contract (kept stable for existing consumers):
- $meta (defaultProfile, revisions, axes, v2 context map, legacy policy)
- top-level groups for the default runtime profile
- profiles: {light-comfortable, light-compact, dark-comfortable, dark-compact}
- contexts: {standard-light, standard-dark, wide-light, wide-dark} (canonical)

Legacy overlay rules (one-way, V2 -> legacy names only):
- base.stoken.* keys are re-emitted from canonical values via the legacy map
- legacy runtime roles (semantic.*, base.*, component.basic.*) are re-emitted
  from canonical equivalents; floors (32px interaction, 12/14px font) and
  derived adapter values (iconContainer, hitTarget, qssContentSize) are
  reproduced with the recorded formulas
- legacy dark runtime overrides are applied to the legacy role namespace only
- Commerly frozen keys are resolved by the archived generator and kept as a
  frozen compatibility layer with an exit condition
- canonical paths always win over legacy names with conflicting values
"""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
import sys
from pathlib import Path
from typing import Any

from ..model import TokenModel, load_json
from ..resolver import Resolver, Resolved
from ..values import group_for, qt_value

CONTEXTS = [("standard", "light"), ("standard", "dark"), ("wide", "light"), ("wide", "dark")]
RUNTIME_PROFILES = ["light-comfortable", "light-compact", "dark-comfortable", "dark-compact"]
# Legacy runtime profile identity is preserved; density is a legacy axis that is
# NOT equivalent to the design profile axis. Both densities currently resolve to
# the standard profile values, matching the audited runtime state.
RUNTIME_CONTEXT = {
    "light-comfortable": ("standard", "light"),
    "light-compact": ("standard", "light"),
    "dark-comfortable": ("standard", "dark"),
    "dark-compact": ("standard", "dark"),
}

FLOOR_32 = {"semantic.dimension.control.height-xs", "semantic.dimension.control.height-sm",
            "semantic.dimension.control.height-md", "semantic.dimension.control.height-lg",
            "semantic.dimension.control.interactive-size",
            "component.basic.button.size.sm.height", "component.basic.button.size.md.height",
            "component.basic.button.size.lg.height",
            "component.basic.input.size.sm.height", "component.basic.input.size.md.height",
            "component.basic.input.size.lg.height"}

# legacy runtime role -> canonical token path (light/dark resolution below)
RUNTIME_ROLES: dict[str, str] = {
    "semantic.color.background.app": "semantic.color.background.surface",
    "semantic.color.background.elevated": "semantic.color.background.surface-raised",
    "semantic.color.background.surface-muted": "semantic.color.background.surface",
    "semantic.color.background.disabled": "semantic.color.background.disabled",
    "semantic.color.background.interactive-hover": "semantic.color.background.hover",
    "semantic.color.background.interactive-active": "semantic.color.background.pressed",
    "semantic.color.background.brand": "semantic.color.interactive.brand.default",
    "semantic.color.background.surface": "semantic.color.background.page",
    "semantic.color.text.primary": "semantic.color.text.primary",
    "semantic.color.text.secondary": "semantic.color.text.secondary",
    "semantic.color.text.tertiary": "semantic.color.text.tertiary",
    "semantic.color.text.muted": "semantic.color.text.muted",
    "semantic.color.text.placeholder": "semantic.color.text.tertiary",
    "semantic.color.text.disabled": "semantic.color.text.disabled",
    "semantic.color.text.onBrand": "semantic.color.text.invert",
    "semantic.color.text.inverse": "semantic.color.text.invert",
    "semantic.color.border.default": "semantic.color.border.default",
    "semantic.color.border.subtle": "semantic.color.border.secondary",
    "semantic.color.border.control": "semantic.color.border.default",
    "semantic.color.border.focus": "semantic.color.interactive.brand.focus-ring",
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
    "semantic.radius.control-sm": "semantic.radius.control-sm",
    "semantic.radius.input": "semantic.radius.control-default",
    "semantic.dimension.control.height-xs": "semantic.interaction-size.embedded",
    "semantic.dimension.control.height-sm": "semantic.interaction-size.embedded",
    "semantic.dimension.control.height-md": "semantic.interaction-size.default",
    "semantic.dimension.control.height-lg": "semantic.interaction-size.large",
    "semantic.dimension.control.interactive-size": "semantic.interaction-size.default",
    "base.fontSize.caption": "semantic.typography.label.default-auxiliary.font-size",
    "base.fontSize.body-sm": "semantic.typography.body.default.font-size",
    "base.fontSize.body-md": "semantic.typography.subtitle.sub-title.font-size",
    "base.lineHeight.body-sm": "semantic.typography.body.default.line-height",
    "base.lineHeight.body-md": "semantic.typography.subtitle.sub-title.line-height",
    "base.fontWeight.regular": "semantic.typography.font-weight.body",
    "base.borderWidth.hairline": "semantic.border-width.boundary-default",
    "component.basic.button.size.sm.height": "component.button.size.small.container.min-height",
    "component.basic.button.size.sm.paddingInline": "component.button.size.small.container.padding-inline",
    "component.basic.button.size.sm.fontSize": "semantic.typography.label.default.font-size",
    "component.basic.button.size.md.height": "component.button.size.medium.container.min-height",
    "component.basic.button.size.md.paddingInline": "component.button.size.medium.container.padding-inline",
    "component.basic.button.size.md.fontSize": "semantic.typography.label.default.font-size",
    "component.basic.button.size.lg.height": "component.button.size.large.container.min-height",
    "component.basic.button.size.lg.paddingInline": "component.button.size.large.container.padding-inline",
    "component.basic.button.size.lg.fontSize": "semantic.typography.label.default.font-size",
    "component.basic.input.size.sm.height": "semantic.interaction-size.embedded",
    "component.basic.input.size.sm.paddingInline": "component.input.size.small.container.padding-inline",
    "component.basic.input.size.sm.paddingBlock": "component.button.base.default.container.padding-block",
    "component.basic.input.size.sm.fontSize": "component.input.base.default.content.font-size",
    "component.basic.input.size.md.height": "semantic.interaction-size.default",
    "component.basic.input.size.md.paddingInline": "component.input.size.medium.container.padding-inline",
    "component.basic.input.size.md.paddingBlock": "component.button.base.default.container.padding-block",
    "component.basic.input.size.md.fontSize": "component.input.base.default.content.font-size",
    "component.basic.input.size.lg.height": "semantic.interaction-size.large",
    "component.basic.input.size.lg.paddingInline": "component.input.size.large.container.padding-inline",
    "component.basic.input.size.lg.paddingBlock": "component.button.base.default.container.padding-block",
    "component.basic.input.size.lg.fontSize": "component.input.base.default.content.font-size",
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
}

FONT_FLOOR = {
    "base.fontSize.caption": 12,
    "base.fontSize.body-sm": 14,
    "base.fontSize.body-md": 14,
    "component.basic.button.size.sm.fontSize": 14,
    "component.basic.button.size.md.fontSize": 14,
    "component.basic.button.size.lg.fontSize": 14,
    "component.basic.input.size.sm.fontSize": 14,
    "component.basic.input.size.md.fontSize": 14,
    "component.basic.input.size.lg.fontSize": 14,
    "component.basic.checkbox.sizes.sm.fontSize": 14,
    "component.basic.checkbox.sizes.lg.fontSize": 14,
}

# Frozen Commerly keys remain for binary/config compatibility, but their values
# must follow the current font policy. No legacy family or asset may re-enter
# the runtime envelope through the compatibility overlay.
LEGACY_FONT_COMPAT = {
    "font": {
        "base.fontFamily.app": "HarmonyOS Sans, HarmonyOS Sans SC",
        "base.fontFamily.fallback": "HarmonyOS Sans SC",
        "base.fontFamily.primary": "HarmonyOS Sans",
        "base.fontFamily.ui": "HarmonyOS Sans",
    },
    "asset": {
        "base.asset.font.poppins-medium":
            "runtime/assets/fonts/HarmonyOSSans/HarmonyOS_Sans.ttf",
        "base.asset.font.poppins-regular":
            "runtime/assets/fonts/HarmonyOSSans/HarmonyOS_Sans.ttf",
        "base.asset.font.poppins-semibold":
            "runtime/assets/fonts/HarmonyOSSans/HarmonyOS_Sans.ttf",
        "semantic.asset.font.appMedium":
            "runtime/assets/fonts/HarmonyOSSans/HarmonyOS_Sans.ttf",
        "semantic.asset.font.appRegular":
            "runtime/assets/fonts/HarmonyOSSans/HarmonyOS_Sans.ttf",
        "semantic.asset.font.appSemibold":
            "runtime/assets/fonts/HarmonyOSSans/HarmonyOS_Sans.ttf",
    },
}

# Legacy dark runtime adaptations on the legacy role namespace only. These are
# runtime-owned and are never written back into the design semantic modes.
LEGACY_DARK: dict[str, str] = {
    "semantic.color.background.app": "primitive.color.neutral.gray.90",
    "semantic.color.background.surface": "primitive.color.neutral.gray.80",
    "semantic.color.background.surface-muted": "primitive.color.neutral.gray.85",
    "semantic.color.background.disabled": "primitive.color.neutral.gray.75",
    "semantic.color.background.interactive-hover": "primitive.color.neutral.gray.70",
    "semantic.color.background.interactive-active": "primitive.color.neutral.gray.65",
    "semantic.color.text.secondary": "primitive.color.neutral.gray.20",
    "semantic.color.text.tertiary": "primitive.color.neutral.gray.30",
    "semantic.color.text.muted": "primitive.color.neutral.gray.25",
    "semantic.color.text.placeholder": "primitive.color.neutral.gray.40",
    "semantic.color.border.default": "primitive.color.neutral.gray.55",
    "semantic.color.border.control": "primitive.color.neutral.gray.50",
    "component.basic.input.state.default.background": "primitive.color.neutral.gray.80",
    "component.basic.input.state.default.placeholder": "primitive.color.neutral.gray.40",
    "component.basic.input.state.default.border": "primitive.color.neutral.gray.55",
    "component.basic.input.state.hover.border": "primitive.color.neutral.gray.30",
    "component.basic.checkbox.state.unchecked.border": "primitive.color.neutral.gray.35",
}

# canonical path -> legacy runtime role carrying the audited dark adaptation.
# Applied to legacy (runtime) profiles only; canonical contexts stay pure.
LEGACY_DARK_CANONICAL: dict[str, str] = {
    "semantic.color.background.surface": "semantic.color.background.surface",
    "semantic.color.background.disabled": "semantic.color.background.disabled",
    "semantic.color.background.hover": "semantic.color.background.interactive-hover",
    "semantic.color.background.pressed": "semantic.color.background.interactive-active",
    "semantic.color.text.secondary": "semantic.color.text.secondary",
    "semantic.color.text.tertiary": "semantic.color.text.tertiary",
    "semantic.color.text.muted": "semantic.color.text.muted",
    "semantic.color.border.default": "semantic.color.border.default",
    "component.input.base.default.container.background-color":
        "component.basic.input.state.default.background",
    "component.input.base.default.placeholder.color":
        "component.basic.input.state.default.placeholder",
    "component.input.base.default.border.color":
        "component.basic.input.state.default.border",
    "component.input.base.hover.border.color":
        "component.basic.input.state.hover.border",
    "component.checkbox.base.default.indicator.border-color":
        "component.basic.checkbox.state.unchecked.border",
}


def legacy_qt_value(resolved: Resolved) -> Any:
    """Legacy stoken value encoding: colors always rgba(r, g, b, a)."""
    value = resolved.value
    if resolved.type == "color":
        if isinstance(value, dict):
            r, g, b = value["components"][:3]
            alpha = value.get("alpha", 1.0)
            return f"rgba({r}, {g}, {b}, {alpha:g})"
        return str(value)
    if resolved.type == "boolean":
        return bool(value)
    return value


def load_commerly(archive_dir: Path):
    """Resolve the frozen Commerly layer with the archived generator."""
    sys.path.insert(0, str(archive_dir))
    spec = importlib.util.spec_from_file_location("commerly_generate", archive_dir / "generate.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    module.ROOT = archive_dir.parents[2]  # design-assets
    module.DESIGN_SYSTEM_DIR = archive_dir.parent
    module.SOURCE_DIR = archive_dir
    for attr, name in (
        ("PRIMITIVES_PATH", "primitives.json"), ("PROFILES_PATH", "profiles.json"),
        ("WIDGETS_PATH", "widgets.json"), ("COMPONENT_LIBRARY_PATH", "component_library.json"),
        ("UI_LAYOUT_BINDINGS_PATH", "ui_layout_bindings.json"),
        ("QSS_CAPABILITIES_PATH", "qt_qss_capabilities.json"),
    ):
        setattr(module, attr, archive_dir / name)
    return module


def build_contexts(model: TokenModel) -> dict[str, dict[str, Resolved]]:
    resolved: dict[str, dict[str, Resolved]] = {}
    for profile_mode, theme_mode in CONTEXTS:
        name = f"{profile_mode}-{theme_mode}"
        records = model.context(profile_mode, theme_mode)
        resolved[name] = Resolver(model, records).resolve_all()
    return resolved


LEGACY_TYPE_HINT = {
    "color": "color", "dimension": "dimension", "spacing": "dimension",
    "borderRadius": "dimension", "borderWidth": "dimension", "number": "dimension",
    "fontSize": "fontSize", "lineHeight": "lineHeight", "fontWeight": "fontWeight",
    "letterSpacing": "letterSpacing", "fontFamily": "fontFamily",
    "opacity": "opacity", "boolean": "dimension", "asset": "asset", "string": "string",
}


def stoken_legacy_values(decisions: list[dict],
                         standard_light: dict[str, Resolved],
                         raw_records: dict) -> dict[str, Any]:
    """base.stoken.* values derived one-way from the canonical standard-light context.

    Canonical aliases are re-emitted as base.stoken aliases (like the audited
    chain did) so design Dark overrides propagate through shared roles.
    """
    canonical_of: dict[str, str] = {}
    for row in decisions:
        legacy_key = row.get("legacyKey", "")
        canonical = row.get("canonicalPath")
        # merged synonyms (border/primary) must not shadow the canonical trunk
        if legacy_key.startswith("--") and canonical and row.get("decision") != "merge":
            canonical_of.setdefault(canonical, legacy_key)
    out: dict[str, Any] = {}
    for row in decisions:
        legacy_key = row.get("legacyKey", "")
        if legacy_key.startswith("03.semantic/Dark:") or legacy_key.startswith("figma:"):
            continue
        canonical = row.get("canonicalPath")
        if not canonical:
            continue
        base_key = "base.stoken." + legacy_key[2:].replace("/", ".")
        record = raw_records.get(canonical)
        source = standard_light.get(canonical)
        if record is None or source is None:
            continue
        raw_value = record.value
        if isinstance(raw_value, str) and raw_value.startswith("{") and raw_value.endswith("}"):
            target = raw_value[1:-1]
            legacy_target = canonical_of.get(target)
            if legacy_target:
                out[base_key] = "{" + "base.stoken." + legacy_target[2:].replace("/", ".") + "}"
                continue
        out[base_key] = legacy_qt_value(source)
    return out


def legacy_type_hint(archive_dir: Path, name: str, value: Any) -> str:
    sys.path.insert(0, str(archive_dir))
    spec = importlib.util.spec_from_file_location("legacy_stoken_import",
                                                  archive_dir / "stoken_import.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module.token_type(name, value)


def _set_legacy_token(raw: dict, path: str, leaf: dict) -> None:
    node = raw
    parts = path.split(".")
    for part in parts[:-1]:
        node = node.setdefault(part, {})
    if parts[-1] in node and isinstance(node[parts[-1]], dict) and "$value" in node[parts[-1]]:
        node[parts[-1]]["$value"] = leaf["$value"]
    else:
        node[parts[-1]] = leaf


def resolve_stoken(stoken_values: dict[str, Any], value: Any) -> Any:
    seen: list[str] = []
    while isinstance(value, str) and value.startswith("{") and value.endswith("}"):
        target = value[1:-1]
        if target in seen or target not in stoken_values:
            return value
        seen.append(target)
        value = stoken_values[target]
    return value


def build_legacy_profiles(
    model: TokenModel,
    contexts: dict[str, dict[str, Resolved]],
    decisions: list[dict],
    archive_dir: Path,
) -> tuple[dict[str, dict[str, dict[str, Any]]], dict, list]:
    """Replay the audited legacy pipeline with V2-derived base.stoken values.

    The old chain injected base.stoken.* keys and mapping-target aliases into
    the Commerly source BEFORE resolution, so Commerly aliases flowing through
    shared semantic roles picked up the design values. This replay preserves
    that behavior exactly; canonical keys are then appended on top.
    """
    module = load_commerly(archive_dir)
    import copy

    standard_light = contexts["standard-light"]
    standard_dark = contexts["standard-dark"]
    raw_light = model.context("standard", "light")
    raw_dark = model.context("standard", "dark")
    stoken_values = stoken_legacy_values(decisions, standard_light, raw_light)
    canonical_of: dict[str, str] = {}
    type_of_base: dict[str, str] = {}
    for row in decisions:
        legacy_key = row.get("legacyKey", "")
        canonical = row.get("canonicalPath")
        if legacy_key.startswith("--") and canonical:
            if row.get("decision") != "merge":
                canonical_of.setdefault(canonical, legacy_key)
            base_key = "base.stoken." + legacy_key[2:].replace("/", ".")
            record = standard_light.get(canonical)
            if record is not None:
                type_of_base[base_key] = LEGACY_TYPE_HINT.get(record.type, "string")

    conflicts: list[dict] = []
    profiles: dict[str, dict[str, dict[str, Any]]] = {}

    for runtime_name in RUNTIME_PROFILES:
        _, theme_mode = RUNTIME_CONTEXT[runtime_name]
        raw = copy.deepcopy(module.load_json(module.PRIMITIVES_PATH))
        profile_config = copy.deepcopy(module.load_json(module.PROFILES_PATH))

        # 0. Mapping targets take precedence over frozen profile overrides:
        #    the audited chain removed every mapped target from the Commerly
        #    theme/density overrides so design values could not be re-shaded.
        for target in RUNTIME_ROLES:
            for axis in profile_config["axes"].values():
                for name, mode in axis.items():
                    if not name.startswith("$"):
                        mode.get("$overrides", {}).pop(target, None)

        # 1. base.stoken.* keys (standard-light values in every profile,
        #    matching the audited chain where the device profile was fixed).
        for base_key, value in stoken_values.items():
            hint = type_of_base.get(base_key, "string")
            _set_legacy_token(raw, base_key, {"$type": hint, "$value": value})

        # 2. mapping targets: alias to the base.stoken source when the audited
        #    type rules allow it (same type, no floor), otherwise the resolved
        #    value. This reproduces the audited chain exactly, including Dark
        #    propagation through shared roles.
        target_types = module.collect_token_types(raw)
        for target, canonical in RUNTIME_ROLES.items():
            source_legacy = canonical_of.get(canonical)
            source_base = ("base.stoken." + source_legacy[2:].replace("/", ".")
                           if source_legacy else None)
            value = stoken_values.get(source_base) if source_base else None
            resolved = None
            if value is None:
                resolved = standard_light.get(canonical)
                value = legacy_qt_value(resolved) if resolved is not None else None
            if value is None:
                continue
            source_type = type_of_base.get(source_base) if source_base else None
            same_type = source_type is not None and target_types.get(target) == source_type
            resolved_value = resolve_stoken(stoken_values, value)
            floor_value = resolved_value
            if isinstance(floor_value, (int, float)) and target in FLOOR_32:
                floor_value = max(floor_value, 32)
            if isinstance(floor_value, (int, float)) and target in FONT_FLOOR:
                floor_value = max(floor_value, FONT_FLOOR[target])
            if same_type and floor_value == resolved_value and source_base:
                # alias to the direct base.stoken source so design Dark
                # overrides on shared roles propagate (audited behavior)
                _set_legacy_token(raw, target, {"$type": "string", "$value": f"{{{source_base}}}"})
            else:
                _set_legacy_token(raw, target, {"$type": "string", "$value": floor_value})

        # 3. legacy dark runtime adaptations (on the legacy role namespace),
        #    plus the 9 explicit design Dark roles re-applied to base.stoken keys
        #    exactly like the audited chain did.
        if theme_mode == "dark":
            dark_overrides = profile_config["axes"]["theme"]["dark"]["$overrides"]
            for target, canonical in LEGACY_DARK.items():
                source_legacy = canonical_of.get(canonical)
                if not source_legacy:
                    continue
                source_base = "base.stoken." + source_legacy[2:].replace("/", ".")
                if source_base in stoken_values:
                    dark_overrides[target] = {"$value": f"{{{source_base}}}"}
            for row in decisions:
                if not row.get("legacyKey", "").startswith("03.semantic/Dark:--"):
                    continue
                legacy_name = row["legacyKey"][len("03.semantic/Dark:"):]
                base_key = "base.stoken." + legacy_name[2:].replace("/", ".")
                if base_key not in stoken_values:
                    continue
                dark_resolved = standard_dark.get(row.get("canonicalPath", ""))
                if dark_resolved is not None:
                    dark_overrides[base_key] = {"$value": legacy_qt_value(dark_resolved)}

        # 4. derived adapter values.
        derived = derive_values(contexts["standard-light"])
        for key, value in derived["values"].items():
            _set_legacy_token(raw, key, {"$type": "dimension", "$value": value})

        profiles[runtime_name] = {}
        try:
            legacy_profiles, legacy_meta = module.build_profiles(raw, profile_config)
        except Exception as exc:  # noqa: BLE001 - surface as structured conflict
            conflicts.append({"profile": runtime_name, "issue": str(exc)})
            continue
        tokens = legacy_profiles[runtime_name]
        types = module.collect_token_types(raw)
        groups = module._group_qt_tokens(tokens, types)
        for group, values in LEGACY_FONT_COMPAT.items():
            groups.setdefault(group, {}).update(values)
        # canonical keys appended; on paths where the audited runtime value
        # differs from the canonical value (identity/adapted roles), the legacy
        # runtime value stays authoritative for runtime consumers and the
        # deviation is recorded. Canonical contexts stay pure.
        context = contexts[RUNTIME_CONTEXT[runtime_name][0] + "-" + theme_mode]
        for path, resolved in sorted(context.items()):
            if path in tokens and _format_equal(tokens[path], qt_value(resolved)) is False:
                groups[group_for(resolved.type)][path] = tokens[path]
                conflicts.append({
                    "path": path, "profile": runtime_name,
                    "legacyValue": tokens[path], "canonicalValue": qt_value(resolved),
                    "decision": "runtime-value-wins-on-legacy-profiles",
                    "reason": "Audited runtime value on a colliding role is preserved "
                             "for runtime consumers; the canonical context carries the "
                             "canonical value. Exit condition: consumer migration or a "
                             "complete design Dark mode.",
                })
                continue
            groups[group_for(resolved.type)][path] = qt_value(resolved)
        # adapter.qt scope: computed V2 tokens (floors, derived geometry)
        for key, value in sorted(adapter_values(contexts["standard-light"]).items()):
            groups["layout" if "font-size" not in key else "font"][key] = value
        # dark runtime-adaptation overlay on canonical paths: the audited dark
        # adaptations are re-applied onto the canonical role paths so migrated
        # contracts keep the audited dark values (design Dark is incomplete).
        if theme_mode == "dark":
            for canonical_path, legacy_role in LEGACY_DARK_CANONICAL.items():
                value = tokens.get(legacy_role)
                if value is None:
                    continue
                groups["color"][canonical_path] = value
                conflicts.append({
                    "path": canonical_path, "profile": runtime_name,
                    "legacyValue": value,
                    "decision": "dark-adaptation-overlay-on-canonical-role",
                    "reason": "Audited dark runtime adaptation re-applied to the "
                             "canonical role; design Dark is incomplete.",
                })
        profiles[runtime_name] = groups
    return profiles, {"legacyReplay": True}, conflicts


def _format_equal(legacy: Any, canonical: Any) -> bool:
    if isinstance(legacy, str) and isinstance(canonical, str):
        return legacy.replace(" ", "").lower() == canonical.replace(" ", "").lower()
    return legacy == canonical


def derive_values(standard_light: dict[str, Resolved]) -> dict:
    def num(path: str) -> float:
        resolved = standard_light[path]
        return float(resolved.value)

    values: dict[str, Any] = {}
    icon_sizes = {
        "XS": "component.button.icon.size.small.icon.size",
        "S": "component.button.icon.size.small.icon.size",
        "M": "component.button.icon.size.medium.icon.size",
        "L": "component.button.icon.size.large.icon.size",
    }
    for size, path in icon_sizes.items():
        values[f"base.stokenAdapter.button.{size}.iconContainer"] = num(path) + 4
    focus = num("semantic.border-width.focus-ring")
    boundary = num("semantic.border-width.boundary-default")
    for size, indicator in (("sm", "semantic.visualization-size.sm"),
                            ("lg", "semantic.visualization-size.md")):
        base = num(indicator)
        values[f"component.basic.checkbox.sizes.{size}.hitTarget"] = max(
            base + 4 + 2 * focus, 32)
        values[f"component.basic.checkbox.sizes.{size}.qssContentSize"] = max(
            base - 2 * boundary, 0)
    return {"values": values}


def adapter_values(standard_light: dict[str, Resolved]) -> dict[str, Any]:
    """V2-defined adapter-scope tokens: business floors and derived values.

    These are computed, registered Qt adapter values (32px interactive floor,
    12/14px font floors, icon container insets, checkbox hit targets). They are
    V2 tokens under the adapter.qt namespace; the legacy base.stokenAdapter.* /
    component.basic.* names are re-emitted only as a compat view.
    """
    def num(path: str) -> float:
        return float(standard_light[path].value)

    def whole(value: float):
        return int(value) if value == int(value) else value

    out: dict[str, Any] = {}
    for size, role in (("xs", "semantic.interaction-size.embedded"),
                       ("sm", "semantic.interaction-size.embedded"),
                       ("md", "semantic.interaction-size.default"),
                       ("lg", "semantic.interaction-size.large"),
                       ("interactive-size", "semantic.interaction-size.default")):
        out[f"adapter.qt.dimension.control-height.{size}"] = whole(max(num(role), 32))
    for size, role in (("caption", ("semantic.typography.label.default-auxiliary.font-size", 12)),
                       ("body-sm", ("semantic.typography.body.default.font-size", 14)),
                       ("body-md", ("semantic.typography.subtitle.sub-title.font-size", 14))):
        out[f"adapter.qt.font-size.{size}"] = whole(max(num(role[0]), role[1]))
    for size, component in (("sm", "small"), ("md", "medium"), ("lg", "large")):
        out[f"adapter.qt.button.size.{size}.height"] = whole(max(
            num(f"component.button.size.{component}.container.min-height"), 32))
    for size, component in (("sm", "small"), ("md", "medium"), ("lg", "large")):
        out[f"adapter.qt.input.size.{size}.height"] = whole(max(
            num(f"component.input.size.{component}.container.visual-height"), 32))
    for size, path in (("XS", "component.button.icon.size.small.icon.size"),
                       ("S", "component.button.icon.size.small.icon.size"),
                       ("M", "component.button.icon.size.medium.icon.size"),
                       ("L", "component.button.icon.size.large.icon.size")):
        out[f"adapter.qt.button.iconContainer.{size}"] = whole(num(path) + 4)
    focus = num("semantic.border-width.focus-ring")
    boundary = num("semantic.border-width.boundary-default")
    for size, indicator in (("sm", "semantic.visualization-size.sm"),
                            ("lg", "semantic.visualization-size.md")):
        out[f"adapter.qt.checkbox.hitTarget.{size}"] = whole(max(num(indicator) + 4 + 2 * focus, 32))
        out[f"adapter.qt.checkbox.qssContentSize.{size}"] = whole(max(num(indicator) - 2 * boundary, 0))
    return out
