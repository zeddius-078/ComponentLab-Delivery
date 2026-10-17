"""Legacy sToken format compatibility export.

Regenerates the legacy stoken.json array format from the canonical V2 source so
the old format becomes a V2-generated product instead of a hand-written second
source of truth. Structure-compatible, not byte-identical: merged synonyms
(border/primary) are re-emitted from the single canonical trunk.

The frozen design snapshot stays at DesignSystem/imports/stoken.json as
read-only evidence with its hash registered in the manifest.
"""
from __future__ import annotations

import json
from typing import Any

from ..model import TokenModel
from ..resolver import Resolved


def build(model: TokenModel, standard_light: dict[str, Resolved]) -> str:
    legacy_of: dict[str, str] = {}
    for row in _decisions(model):
        legacy_key = row.get("legacyKey", "")
        canonical = row.get("canonicalPath")
        if not legacy_key.startswith("--") or not canonical:
            continue
        legacy_of[canonical] = legacy_key
    # merged trunk: border/primary shares the default trunk
    legacy_of["semantic.color.border.default"] = "--color/border/default"
    legacy_of["primitive.color.neutral.gray.35"] = "--color/neutral/gray-35"

    primitive_mode: dict[str, Any] = {}
    profile_std: dict[str, Any] = {}
    profile_wide: dict[str, Any] = {}
    semantic_light: dict[str, Any] = {}
    semantic_dark: dict[str, Any] = {}
    component_mode: dict[str, Any] = {}

    for path, record in sorted(standard_light.items()):
        layer = record.layer
        if layer == "primitive":
            primitive_mode[_legacy_name(record, legacy_of, "primitive")] = _legacy_value(record, legacy_of)
        elif layer == "profile":
            name = _legacy_name(record, legacy_of, "profile")
            profile_std[name] = _legacy_value(record, legacy_of)
            from copy import copy
            wide_record = copy(record)
            if "wide" in record.modes:
                wide_record.value = record.modes["wide"]
            profile_wide[name] = _legacy_value(wide_record, legacy_of)
        elif layer == "semantic":
            name = _legacy_name(record, legacy_of, "semantic")
            semantic_light[name] = _legacy_value(record, legacy_of)
            if "dark" in record.modes:
                from copy import copy
                dark_record = copy(record)
                dark_record.value = record.modes["dark"]
                semantic_dark[name] = _legacy_value(dark_record, legacy_of)
        elif layer == "component":
            component_mode[_legacy_name(record, legacy_of, "component")] = _legacy_value(record, legacy_of)
        elif layer == "product":
            name = _legacy_name(record, legacy_of, "product")
            profile_std[name] = _legacy_value(record, legacy_of)
            from copy import copy
            wide_record = copy(record)
            if "wide" in record.modes:
                wide_record.value = record.modes["wide"]
            profile_wide[name] = _legacy_value(wide_record, legacy_of)
    # re-emit the merged synonym
    semantic_light["--color/border/primary"] = _legacy_alias_value(standard_light["semantic.color.border.default"], legacy_of)

    payload = [
        {"01.primitive": [{"参数": primitive_mode}]},
        {"02.profile": [{"Standard": profile_std}, {"Wide": profile_wide}]},
        {"03.semantic": [{"Light": semantic_light}, {"Dark": semantic_dark}]},
        {"04.component": [{"default": component_mode}]},
    ]
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def _decisions(model: TokenModel) -> list[dict]:
    from pathlib import Path
    from ..model import load_json
    legacy_map = load_json(model.source_dir.parent / "config" / "legacy-map.json")
    return legacy_map.get("decisions", [])


def _legacy_name(record: Resolved, legacy_of: dict[str, str], layer: str) -> str:
    name = legacy_of.get(record.path)
    if name:
        return name
    # deterministic fallback for tokens without a legacy identity (figma-only)
    return "--" + record.path.replace(".", "/")


def _legacy_value(record: Resolved, legacy_of: dict[str, str]) -> Any:
    value = record.value
    if isinstance(value, str) and value.startswith("{"):
        target = value[1:-1]
        legacy_target = legacy_of.get(target)
        if legacy_target:
            return f"var({legacy_target})"
        return f"var(--{target.replace('.', '/')})"
    if record.type == "color" and isinstance(value, dict):
        r, g, b = value["components"][:3]
        alpha = value.get("alpha", 1.0)
        return f"rgba({r}, {g}, {b}, {alpha:g})"
    return value


def _legacy_alias_value(record: Resolved, legacy_of: dict[str, str]) -> Any:
    value = record.value
    if isinstance(value, str) and value.startswith("{"):
        target = value[1:-1]
        legacy_target = legacy_of.get(target)
        if legacy_target:
            return f"var({legacy_target})"
    if record.type == "color" and isinstance(value, dict):
        r, g, b = value["components"][:3]
        alpha = value.get("alpha", 1.0)
        return f"rgba({r}, {g}, {b}, {alpha:g})"
    return value
