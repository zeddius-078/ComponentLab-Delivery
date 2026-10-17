"""ComponentLab V2 canonical token model and validation.

Loads the four layer files + product packages, validates them against the
registered vocabulary and schema, flattens them into token records, and applies
mode overrides (profile standard/wide, semantic light/dark, product modes).

This module has no runtime dependencies beyond the standard library.
"""
from __future__ import annotations

import hashlib
import json
import re
from copy import copy
from pathlib import Path
from typing import Any

LAYERS = ("primitive", "profile", "semantic", "component")
TYPES = {
    "color", "dimension", "number", "fontFamily", "fontWeight", "fontSize",
    "lineHeight", "letterSpacing", "spacing", "borderRadius", "borderWidth",
    "boolean", "string", "opacity", "asset", "lineWidth",
}
UNIT_REQUIRED = {"dimension", "spacing", "borderRadius", "borderWidth", "fontSize",
                 "lineHeight", "letterSpacing", "opacity"}
SEGMENT = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")
ALIAS = re.compile(r"^\{([a-z0-9]+(-[a-z0-9]+)*(\.[a-z0-9]+(-[a-z0-9]+)*)+)\}$")

# Registered alias compatibility: length-like types may alias each other
# (the design reuses dimension atoms as spacing scale steps). The leaf's
# declared type wins. Colors, fonts, numbers, booleans and opacity stay exact.
LENGTH_LIKE = {"dimension", "spacing", "borderRadius", "borderWidth", "fontSize",
               "lineHeight", "letterSpacing"}


def alias_types_compatible(source: str, target: str) -> bool:
    if source == target:
        return True
    return source in LENGTH_LIKE and target in LENGTH_LIKE


class ValidationError(ValueError):
    pass


def load_json(path: Path) -> Any:
    def unique(pairs):
        out = {}
        for k, v in pairs:
            if k in out:
                raise ValidationError(f"Duplicate JSON key in {path.name}: {k}")
            out[k] = v
        return out

    try:
        return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique)
    except json.JSONDecodeError as exc:
        raise ValidationError(f"Invalid JSON in {path}: {exc}") from exc


class TokenRecord:
    __slots__ = ("path", "type", "value", "unit", "description", "owner", "sourceOwner",
                 "layer", "modes")

    def __init__(self, path: str, type_: str, value: Any, unit: str | None,
                 description: str, owner: str | None, source_owner: str | None, layer: str):
        self.path = path
        self.type = type_
        self.value = value
        self.unit = unit
        self.description = description
        self.owner = owner
        self.sourceOwner = source_owner
        self.layer = layer
        self.modes: dict[str, Any] = {}


class TokenModel:
    def __init__(self, source_dir: Path):
        self.source_dir = source_dir
        self.dictionary = load_json(source_dir / "dictionary.json")
        self.registry = load_json(source_dir / "component-registry.json")
        self.layers: dict[str, dict[str, TokenRecord]] = {}
        self.modes: dict[str, dict[str, str | None]] = {"profile": {}, "theme": {}}
        self.product: dict[str, TokenRecord] = {}
        self.product_meta: dict[str, Any] = {}
        for layer in LAYERS:
            doc = load_json(source_dir / f"{layer}.json")
            self._load_layer(layer, doc)
        self._load_product_packages()
        self._validate_references()

    # ------------------------------------------------------------------ load

    def _load_layer(self, layer: str, doc: dict) -> None:
        if doc.get("layer") != layer:
            raise ValidationError(f"Layer file declares {doc.get('layer')!r}, expected {layer!r}")
        wrapper = doc.get("root")
        if not isinstance(wrapper, dict):
            raise ValidationError(f"Missing root in {layer}.json")
        root = wrapper.get(layer)
        if not isinstance(root, dict):
            raise ValidationError(f"Missing root.{layer} in {layer}.json")
        records: dict[str, TokenRecord] = {}
        self._walk(root, layer, layer, "", records)
        overrides = wrapper.get("$modeOverrides", {})
        if not isinstance(overrides, dict):
            raise ValidationError(f"$modeOverrides must be an object in {layer}.json")
        for mode, entries in overrides.items():
            if not SEGMENT.fullmatch(mode):
                raise ValidationError(f"Invalid mode name {mode!r} in {layer}.json")
            for path, leaf in entries.items():
                if path not in records:
                    raise ValidationError(
                        f"Mode override targets unknown token: {layer} {mode} {path}")
                if not isinstance(leaf, dict) or "$value" not in leaf:
                    raise ValidationError(f"Mode override leaf missing $value: {layer} {mode} {path}")
                if mode in records[path].modes:
                    raise ValidationError(
                        f"Duplicate mode override for {layer} {mode} {path}")
                records[path].modes[mode] = leaf["$value"]
        if layer == "profile":
            self.modes["profile"] = {"standard": "standard", "wide": "wide"}
        if layer == "semantic":
            self.modes["theme"] = {"light": "light", "dark": "dark"}
        self.layers[layer] = records

    def _load_product_packages(self) -> None:
        product_dir = self.source_dir / "product"
        if not product_dir.is_dir():
            return
        for path in sorted(product_dir.glob("*.json")):
            doc = load_json(path)
            if doc.get("layer") != "product":
                raise ValidationError(f"Product package must declare layer=product: {path.name}")
            product_name = doc.get("product")
            if not product_name:
                raise ValidationError(f"Product package missing product name: {path.name}")
            self.product_meta[product_name] = {"file": path.name}
            wrapper = doc.get("root", {})
            root = wrapper.get("product")
            if not isinstance(root, dict):
                raise ValidationError(f"Missing root.product in {path.name}")
            self._walk(root, "product", "product", "", self.product)
            overrides = wrapper.get("$modeOverrides", {})
            if not isinstance(overrides, dict):
                raise ValidationError(f"$modeOverrides must be an object in {path.name}")
            for mode, entries in overrides.items():
                for token_path, leaf in entries.items():
                    record = self.product.get(token_path)
                    if record is None:
                        raise ValidationError(
                            f"Mode override targets unknown token: product {mode} {token_path}")
                    if mode in record.modes:
                        raise ValidationError(f"Duplicate mode override: product {mode} {token_path}")
                    record.modes[mode] = leaf.get("$value")

    def _walk(self, node: dict, layer: str, prefix: str, inherited_desc: str,
              records: dict[str, TokenRecord]) -> None:
        for key, child in node.items():
            if key == "$modeOverrides":
                continue
            if not SEGMENT.fullmatch(key):
                raise ValidationError(f"Invalid path segment {key!r} under {prefix or layer}")
            path = f"{prefix}.{key}" if prefix else key
            if isinstance(child, dict) and "$type" in child:
                self._leaf(layer, path, child, inherited_desc, records)
            elif isinstance(child, dict):
                desc = child.get("$description", inherited_desc) if isinstance(child.get("$description"), str) else inherited_desc
                self._walk(child, layer, path, desc, records)
            else:
                raise ValidationError(f"Non-object token node at {path}")

    def _leaf(self, layer: str, path: str, leaf: dict, inherited_desc: str,
              records: dict[str, TokenRecord]) -> None:
        type_ = leaf.get("$type")
        if type_ not in TYPES:
            raise ValidationError(f"Invalid $type {type_!r} at {path}")
        if "$value" not in leaf:
            raise ValidationError(f"Token missing $value at {path}")
        unit = leaf.get("unit")
        if type_ in UNIT_REQUIRED and not unit:
            raise ValidationError(f"Token {path} of type {type_} requires an explicit unit")
        if unit and unit not in ("px", "%"):
            raise ValidationError(f"Invalid unit {unit!r} at {path}")
        if type_ == "opacity" and unit != "%":
            raise ValidationError(f"Opacity token {path} must declare unit '%'")
        if type_ == "color":
            self._check_color(path, leaf["$value"])
        records[path] = TokenRecord(
            path=path, type_=type_, value=leaf["$value"], unit=unit,
            description=str(leaf.get("$description", inherited_desc)),
            owner=leaf.get("$owner"), source_owner=leaf.get("$sourceOwner"), layer=layer)

    @staticmethod
    def _check_color(path: str, value: Any) -> None:
        if isinstance(value, str) and ALIAS.fullmatch(value):
            return
        if not isinstance(value, dict):
            raise ValidationError(f"Color token {path} must be a structured color or an alias")
        if value.get("colorSpace") != "srgb":
            raise ValidationError(f"Color token {path} must declare srgb colorSpace")
        components = value.get("components")
        if not isinstance(components, list) or len(components) < 3:
            raise ValidationError(f"Color token {path} has invalid components")
        for c in components[:3]:
            if not isinstance(c, int) or not 0 <= c <= 255:
                raise ValidationError(f"Color token {path} components must be 8-bit integers")
        if len(components) > 3:
            raise ValidationError(f"Color token {path} must keep alpha outside components")
        alpha = value.get("alpha")
        if alpha is not None and not (isinstance(alpha, (int, float)) and 0 <= alpha <= 1):
            raise ValidationError(f"Color token {path} alpha must be 0..1")

    # ------------------------------------------------------------- references

    def _validate_references(self) -> None:
        all_paths: set[str] = set(self.product)
        type_of: dict[str, str] = {}
        for layer in LAYERS:
            for path, record in self.layers[layer].items():
                all_paths.add(path)
                type_of[path] = record.type
        for path, record in self.product.items():
            type_of[path] = record.type
        for layer in LAYERS:
            for path, record in self.layers[layer].items():
                value = record.value
                m = ALIAS.fullmatch(str(value))
                if not m:
                    continue
                target = m.group(1)
                if target not in all_paths:
                    raise ValidationError(f"Missing alias target {target} from {path}")
                if not alias_types_compatible(record.type, type_of[target]):
                    raise ValidationError(
                        f"Alias type mismatch: {path} ({record.type}) -> {target} ({type_of[target]})")

    # ----------------------------------------------------------------- access

    def context(self, profile_mode: str, theme_mode: str) -> dict[str, TokenRecord]:
        """Build one context: primitive + profile(mode) + semantic(theme) + component + product.

        TokenRecords are copied so applying a mode never mutates the shared layer.
        """
        records: dict[str, TokenRecord] = {}
        for source in (self.product, self.layers["primitive"], self.layers["profile"],
                       self.layers["semantic"], self.layers["component"]):
            for path, record in source.items():
                records[path] = copy(record)
        self._apply(records, self.layers["profile"], profile_mode)
        self._apply(records, self.layers["semantic"], theme_mode)
        self._apply(records, self.product, profile_mode)
        return records

    @staticmethod
    def _apply(records: dict[str, TokenRecord], layer: dict[str, TokenRecord],
               mode: str) -> None:
        for path, record in layer.items():
            if mode in record.modes:
                records[path].value = record.modes[mode]

    def source_revision(self) -> str:
        digest = hashlib.sha256()
        for name in ("primitive.json", "profile.json", "semantic.json",
                     "component.json", "dictionary.json", "component-registry.json"):
            path = self.source_dir / name
            digest.update(name.encode())
            if path.exists():
                digest.update(path.read_bytes())
        product_dir = self.source_dir / "product"
        if product_dir.is_dir():
            for path in sorted(product_dir.glob("*.json")):
                digest.update(path.name.encode())
                digest.update(path.read_bytes())
        return digest.hexdigest()
