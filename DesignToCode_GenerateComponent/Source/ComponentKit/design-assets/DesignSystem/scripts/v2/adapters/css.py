"""CSS custom-properties adapter.

Flattens canonical paths to legal CSS identifiers and detects name collisions
(a-b.c vs a.b-c flatten to the same identifier) instead of silently overwriting.
"""
from __future__ import annotations

import re
from typing import Any

from ..resolver import Resolved
from ..values import css_value

IDENT = re.compile(r"^--[a-zA-Z_][a-zA-Z0-9_-]*$")


def css_identifier(path: str) -> str:
    ident = "--" + path.replace(".", "-").replace("_", "-")
    if not IDENT.fullmatch(ident):
        raise ValueError(f"Canonical path cannot form a CSS identifier: {path}")
    return ident


def build(contexts: dict[str, dict[str, Resolved]]) -> str:
    blocks: list[str] = []
    blocks.append("/* ComponentLab V2 tokens as CSS custom properties. Generated; do not edit. */\n")
    collisions: dict[str, list[str]] = {}
    selector_map = {
        "standard-light": '[data-theme="light"][data-profile="standard"]',
        "standard-dark": '[data-theme="dark"][data-profile="standard"]',
        "wide-light": '[data-theme="light"][data-profile="wide"]',
        "wide-dark": '[data-theme="dark"][data-profile="wide"]',
    }
    for name, resolved in sorted(contexts.items()):
        lines = [f"{selector_map[name]} {{"]
        seen: dict[str, str] = {}
        for path in sorted(resolved):
            ident = css_identifier(path)
            if ident in seen:
                collisions.setdefault(ident, [seen[ident], path])
                continue
            seen[ident] = path
            lines.append(f"  {ident}: {css_value(resolved[path])};")
        lines.append("}")
        blocks.append("\n".join(lines))
    if collisions:
        details = "; ".join(f"{k}: {', '.join(v)}" for k, v in sorted(collisions.items()))
        raise ValueError(f"CSS identifier collisions: {details}")
    return "\n\n".join(blocks) + "\n"
