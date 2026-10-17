"""Token catalog adapter: full ledger CSV with per-context resolved values and
migration decisions."""
from __future__ import annotations

import csv
import io
from typing import Any

from ..model import TokenModel
from ..resolver import Resolved
from ..values import qt_value

HEADER = [
    "record_id", "canonical_path", "layer", "type", "unit", "owner",
    "resolved_standard_light", "resolved_standard_dark", "resolved_wide_light",
    "resolved_wide_dark", "profile_sensitive", "theme_sensitive",
    "description",
]


def build(model: TokenModel, contexts: dict[str, dict[str, Resolved]]) -> str:
    buffer = io.StringIO()
    writer = csv.DictWriter(buffer, fieldnames=HEADER, lineterminator="\n")
    writer.writeheader()
    paths: set[str] = set()
    for resolved in contexts.values():
        paths.update(resolved)
    context_names = ["standard-light", "standard-dark", "wide-light", "wide-dark"]
    for index, path in enumerate(sorted(paths), start=1):
        record = next((contexts[c][path] for c in context_names if path in contexts[c]), None)
        if record is None:
            continue
        values = [qt_value(contexts[c][path]) for c in context_names]
        writer.writerow({
            "record_id": f"T{index:04d}",
            "canonical_path": path,
            "layer": record.path.split(".", 1)[0],
            "type": record.type,
            "unit": record.unit or "",
            "owner": next((r.owner for c in context_names
                           if (r := contexts[c].get(path)) and r.owner), ""),
            "resolved_standard_light": values[0],
            "resolved_standard_dark": values[1],
            "resolved_wide_light": values[2],
            "resolved_wide_dark": values[3],
            "profile_sensitive": "true" if len({values[0], values[2]}) > 1 else "false",
            "theme_sensitive": "true" if len({values[0], values[1]}) > 1 else "false",
            "description": record.description,
        })
    return buffer.getvalue()
