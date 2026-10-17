"""ComponentLab V2 alias resolver.

Resolves {a.b.c} aliases to final typed values inside one context, with cycle
detection, missing-target and type-consistency checks. Colors stay structured
(srgb components); adapters convert them to platform encodings.
"""
from __future__ import annotations

from typing import Any

from .model import TokenModel, TokenRecord, alias_types_compatible


class Resolved:
    __slots__ = ("type", "value", "unit", "path", "description", "layer", "modes",
                 "owner", "sourceOwner")

    def __init__(self, type_: str, value: Any, unit: str | None, path: str,
                 description: str = "", layer: str = "", owner: str | None = None,
                 source_owner: str | None = None):
        self.type = type_
        self.value = value
        self.unit = unit
        self.path = path
        self.description = description
        self.layer = layer
        self.modes: dict[str, Any] = {}
        self.owner = owner
        self.sourceOwner = source_owner


class Resolver:
    def __init__(self, model: TokenModel, records: dict[str, TokenRecord]):
        self.records = records
        self.cache: dict[str, Resolved] = {}
        self._stack: list[str] = []

    def resolve_all(self) -> dict[str, Resolved]:
        out: dict[str, Resolved] = {}
        for path in sorted(self.records):
            out[path] = self.resolve(path)
        return out

    def resolve(self, path: str) -> Resolved:
        if path in self.cache:
            return self.cache[path]
        if path in self._stack:
            cycle = " -> ".join([*self._stack, path])
            raise ValueError(f"Circular token reference: {cycle}")
        record = self.records.get(path)
        if record is None:
            raise KeyError(f"Unknown token: {path}")
        self._stack.append(path)
        value = record.value
        if isinstance(value, str) and value.startswith("{") and value.endswith("}"):
            target = value[1:-1]
            result = self.resolve(target)
            if not alias_types_compatible(record.type, result.type):
                raise TypeError(
                    f"Alias type mismatch at {path}: {record.type} -> {target} ({result.type})")
            out = Resolved(record.type, result.value, result.unit or record.unit, path,
                           record.description, record.layer, record.owner, record.sourceOwner)
        else:
            out = Resolved(record.type, value, record.unit, path, record.description,
                           record.layer, record.owner, record.sourceOwner)
        out.modes = dict(record.modes)
        self._stack.pop()
        self.cache[path] = out
        return out

    def opacity_ratio(self, path: str) -> float:
        """Opacity computation coefficient 0..1 from the declared source unit."""
        resolved = self.resolve(path)
        if resolved.type != "opacity":
            raise TypeError(f"{path} is not an opacity token ({resolved.type})")
        if resolved.unit == "%":
            return float(resolved.value) / 100.0
        raise TypeError(f"Opacity token {path} has unknown unit {resolved.unit}")
