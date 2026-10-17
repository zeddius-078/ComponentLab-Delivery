"""Platform value formatting shared by V2 adapters."""
from __future__ import annotations

from typing import Any

from .resolver import Resolved

GROUP_TYPES = {
    "color": {"color"},
    "font": {"fontFamily", "fontWeight", "fontSize", "lineHeight", "letterSpacing"},
    "layout": {"dimension", "spacing", "borderRadius", "borderWidth"},
    "motion": {"duration", "cubicBezier"},
    "shape": set(),
    "gradient": {"gradient"},
    "shadow": {"shadow", "boxShadow"},
    "asset": {"asset"},
    "term": set(),
    "string": {"string"},
    "number": {"number", "opacity"},
    "boolean": {"boolean"},
    "other": set(),
}


def group_for(type_: str) -> str:
    for group, types in GROUP_TYPES.items():
        if type_ in types:
            return group
    return "other"


def qt_value(resolved: Resolved) -> Any:
    value = resolved.value
    if resolved.type == "color":
        return color_to_css(value)
    if resolved.type in ("asset", "string", "fontFamily"):
        return value
    if resolved.type == "boolean":
        return bool(value)
    return value


def color_to_css(value: Any) -> str:
    if not isinstance(value, dict):
        return str(value)
    r, g, b = value["components"][:3]
    alpha = value.get("alpha", 1.0)
    if alpha >= 1:
        return f"#{r:02X}{g:02X}{b:02X}"
    a = max(0, min(255, round(alpha * 255)))
    return f"rgba({r}, {g}, {b}, {a})"


def css_value(resolved: Resolved) -> str:
    value = resolved.value
    if resolved.type == "color":
        return color_to_css(value)
    if resolved.type in ("dimension", "spacing", "borderRadius", "borderWidth",
                         "fontSize", "lineHeight", "letterSpacing"):
        return f"{value}px"
    if resolved.type == "opacity":
        return f"{float(value) / 100.0:g}"
    return str(value)


def dtcg_value(resolved: Resolved) -> Any:
    if resolved.type == "color":
        return resolved.value
    if resolved.type == "opacity":
        return {"$type": "number", "$value": float(resolved.value) / 100.0,
                "$extensions": {"com.componentlab.opacityPercent": resolved.value}}
    return resolved.value
