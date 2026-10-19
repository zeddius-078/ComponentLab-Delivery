"""Qt runtime adapter: emits tokens_qt.json from the canonical V2 source only.

The runtime now consumes pure V2: canonical contexts (standard/wide x
light/dark), canonical token paths, and the registered adapter scope
(adapter.qt.*: business floors and derived geometry). No legacy names, no
legacy role namespace, no legacy dark adaptations and no frozen third-party
layers remain.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
from typing import Any

from ..model import TokenModel
from ..resolver import Resolver, Resolved
from ..values import group_for, qt_value

CONTEXTS = [("standard", "light"), ("standard", "dark"), ("wide", "light"), ("wide", "dark")]
# The runtime profile names are the canonical context names. The legacy
# density axis (comfortable/compact) is removed from the architecture.
RUNTIME_PROFILES = [f"{profile}-{theme}" for profile, theme in CONTEXTS]


def build_contexts(model: TokenModel) -> dict[str, dict[str, Resolved]]:
    resolved: dict[str, dict[str, Resolved]] = {}
    for profile_mode, theme_mode in CONTEXTS:
        name = f"{profile_mode}-{theme_mode}"
        records = model.context(profile_mode, theme_mode)
        resolved[name] = Resolver(model, records).resolve_all()
    return resolved


def adapter_values(standard_light: dict[str, Resolved]) -> dict[str, Any]:
    """Registered adapter-scope tokens: business floors and derived geometry.

    These are computed Qt adapter values (32px interactive floor, 12/14px font
    floors, icon container insets, checkbox hit targets). They are V2 tokens
    under the adapter.qt namespace, computed from the canonical context only.
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
            num(f"component.button.base.size.{component}.slot.container.min-height"), 32))
    for size, component in (("sm", "small"), ("md", "medium"), ("lg", "large")):
        out[f"adapter.qt.input.size.{size}.height"] = whole(max(
            num(f"component.input.base.size.{component}.slot.container.visual-height"), 32))
    for size, path in (("XS", "component.button.base.size.small.slot.icon.size"),
                       ("S", "component.button.base.size.small.slot.icon.size"),
                       ("M", "component.button.base.size.medium.slot.icon.size"),
                       ("L", "component.button.base.size.large.slot.icon.size")):
        out[f"adapter.qt.button.iconContainer.{size}"] = whole(num(path) + 4)
    focus = num("semantic.border-width.focus-ring")
    boundary = num("semantic.border-width.boundary-default")
    # Checkbox geometry is owned by the current component Recipe.  Deriving the
    # Qt hit target from the old visualization-size scale lets the adapter drift
    # when the Recipe changes.
    for size, recipe_size in (
        ("sm", "small"),
        ("lg", "large"),
    ):
        indicator = f"component.checkbox.base.size.{recipe_size}.slot.indicator.visual-size"
        interaction = f"component.checkbox.base.size.{recipe_size}.slot.container.interaction-size"
        focus_gap = 2
        out[f"adapter.qt.checkbox.hitTarget.{size}"] = whole(
            max(num(interaction), num(indicator) + 2 * (focus_gap + focus), 32)
        )
        out[f"adapter.qt.checkbox.qssContentSize.{size}"] = whole(max(num(indicator) - 2 * boundary, 0))
    # runtime accessibility config (audited values); visual focus-ring tokens
    # are globally disabled in the V2 design system.
    out["adapter.qt.focus-indicator.mode"] = "keyboard"
    out["adapter.qt.focus-indicator.opacity"] = 0.5
    return out


def context_groups(resolved: dict[str, Resolved]) -> dict[str, dict[str, Any]]:
    groups: dict[str, dict[str, Any]] = {}
    for path, record in sorted(resolved.items()):
        groups.setdefault(group_for(record.type), {})[path] = qt_value(record)
    return groups


def build(model: TokenModel) -> dict[str, Any]:
    contexts = build_contexts(model)

    profiles: dict[str, dict[str, dict[str, Any]]] = {}
    for name in RUNTIME_PROFILES:
        groups = context_groups(contexts[name])
        # adapter values are profile-dependent: computed from the profile's
        # own resolved context (wide resolves larger interaction sizes and
        # icon canvases)
        computed = adapter_values(contexts[name])
        for key, value in sorted(computed.items()):
            groups["layout" if "font-size" not in key else "font"][key] = value
        profiles[name] = groups

    default_groups = profiles["standard-light"]
    runtime = {
        "$description": "Generated Qt runtime token adapter from the canonical V2 source. Do not hand-edit.",
        "$meta": {
            "source": {
                "reference": "ComponentLab V2 canonical token source",
                "canonicalDir": "DesignSystem/source",
            },
            "commonSourceRevision": model.source_revision()[:12],
            "platformSourceRevision": hashlib.sha256(
                Path(__file__).read_bytes()).hexdigest()[:12],
            "defaultProfile": "standard-light",
            "axisOrder": ["profile", "theme"],
            "axes": {
                "profile": {"default": "standard", "values": ["standard", "wide"],
                            "description": "Design profile mode from the profile layer."},
                "theme": {"default": "light", "values": ["light", "dark"],
                          "description": "Design theme mode from the semantic layer."},
            },
            "v2": {
                "schemaVersion": "2.0",
                "contexts": RUNTIME_PROFILES,
                "wideRuntimeConsumer": "none-registered",
                "darkPublishable": False,
                "adapterScope": {
                    "description": "adapter.qt.* are computed, registered Qt adapter values "
                                   "(business floors and derived geometry); they are V2 tokens, "
                                   "not legacy names.",
                },
            },
        },
    }
    runtime.update(default_groups)
    runtime["profiles"] = profiles
    return runtime
