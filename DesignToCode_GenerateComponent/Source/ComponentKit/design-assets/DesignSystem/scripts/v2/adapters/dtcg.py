"""DTCG 2025.10 export adapter.

Emits one standards-shaped token file per context. Project extension types
(opacity, asset) are exported as their closest standard type with an explicit
$extensions record; they are never presented as DTCG standard types.
"""
from __future__ import annotations

import json
from typing import Any

from ..resolver import Resolved
from ..values import dtcg_value

DTCG_TYPES = {
    "color": "color", "dimension": "dimension", "number": "number",
    "fontFamily": "fontFamily", "fontWeight": "fontWeight",
    "fontSize": "fontSize", "lineHeight": "lineHeight",
    "letterSpacing": "letterSpacing", "spacing": "spacing",
    "borderRadius": "borderRadius", "borderWidth": "borderWidth",
    "boolean": "boolean", "string": "string",
    # project extensions
    "opacity": "number", "asset": "string", "lineWidth": "borderWidth",
}


def build(context_name: str, resolved: dict[str, Resolved]) -> str:
    out: dict[str, Any] = {}
    for path in sorted(resolved):
        record = resolved[path]
        node = out
        parts = path.split(".")
        for part in parts[:-1]:
            node = node.setdefault(part, {})
        standard_type = DTCG_TYPES[record.type]
        value = dtcg_value(record)
        leaf: dict[str, Any] = {"$type": standard_type, "$value": value}
        if record.unit:
            leaf.setdefault("$extensions", {})["com.componentlab.unit"] = record.unit
        if record.type not in ("color", "dimension", "number", "fontFamily", "fontWeight",
                               "fontSize", "lineHeight", "letterSpacing", "spacing",
                               "borderRadius", "borderWidth", "boolean", "string"):
            leaf.setdefault("$extensions", {})["com.componentlab.sourceType"] = record.type
        if record.description:
            leaf["$description"] = record.description
        node[parts[-1]] = leaf
    envelope = {
        "$schema": "https://www.designtokens.org/tr/2025.10/format/",
        "$description": f"ComponentLab V2 tokens, context {context_name}. "
                        "Project extension types are marked under $extensions.com.componentlab.",
        "$extensions": {
            "com.componentlab": {
                "context": context_name,
                "source": "DesignSystem/source (canonical V2 source)",
            },
        },
    }
    envelope.update(out)
    return json.dumps(envelope, ensure_ascii=False, indent=2) + "\n"
