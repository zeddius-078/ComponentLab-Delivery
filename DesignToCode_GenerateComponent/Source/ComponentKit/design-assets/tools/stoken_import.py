"""Normalize the supplied sToken export, then reuse the canonical alias resolver."""
from __future__ import annotations

import copy
import hashlib
import json
import re
from pathlib import Path

VAR = re.compile(r"^var\((--[^()]+)\)$")
LAYERS = ("01.primitive", "02.profile", "03.semantic", "04.component")


def validate_collection_axes(collections):
    """Reject extra design modes instead of silently exporting only the defaults."""
    if tuple(collections) != LAYERS:
        raise ValueError(f"sToken collections must be exactly {LAYERS}; got {tuple(collections)}")
    expected_modes = {
        "01.primitive": ("参数",),
        "02.profile": ("Standard", "Wide"),
        "03.semantic": ("Light", "Dark"),
        "04.component": ("default",),
    }
    for collection, expected in expected_modes.items():
        actual = tuple(collections[collection])
        if actual != expected:
            raise ValueError(f"sToken {collection} modes must be exactly {expected}; got {actual}")
    profile = collections["02.profile"]
    if set(profile["Standard"]) != set(profile["Wide"]):
        raise ValueError("sToken Standard and Wide must share the same Profile role names")
    semantic = collections["03.semantic"]
    unknown_dark = set(semantic["Dark"]) - set(semantic["Light"])
    if unknown_dark:
        raise ValueError(f"Dark semantic roles lack Light counterparts: {sorted(unknown_dark)}")


def load(path: Path):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Duplicate sToken key: {key}")
            result[key] = value
        return result
    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique)


def key(name):
    if not name.startswith("--"):
        raise ValueError(f"Unsupported sToken name: {name}")
    return "base.stoken." + name[2:].replace("/", ".")


def token_type(name, value):
    if "color" in name or (isinstance(value, str) and value.startswith("rgba(")):
        return "color"
    for part, kind in (("font-family", "fontFamily"), ("font-size", "fontSize"),
                       ("font-weight", "fontWeight"), ("line-height", "lineHeight"),
                       ("letter-spacing", "letterSpacing")):
        if part in name:
            return kind
    if "opacity" in name:
        return "opacity"
    if isinstance(value, (int, float)) or VAR.fullmatch(str(value)):
        return "dimension"
    return "string"


def set_token(raw, path, value, kind=None):
    target = raw
    parts = path.split(".")
    for part in parts[:-1]:
        target = target.setdefault(part, {})
    leaf = target.get(parts[-1])
    if not isinstance(leaf, dict) or "$value" not in leaf:
        if kind is None:
            raise ValueError(f"Unknown target Token: {path}")
        leaf = target[parts[-1]] = {"$type": kind}
    leaf["$value"] = value


def prepare(root: Path, raw, profile_config):
    input_path = root / "DesignSystem/imports/stoken.json"
    config_path = root / "tokens/stoken-map.json"
    config, source = load(config_path), load(input_path)
    collections = {}
    for entry in source:
        for collection, modes in entry.items():
            if collection in collections:
                raise ValueError(f"Duplicate collection: {collection}")
            collections[collection] = {}
            for mode in modes:
                for name, values in mode.items():
                    if name in collections[collection]:
                        raise ValueError(f"Duplicate mode: {collection}/{name}")
                    collections[collection][name] = values
    validate_collection_axes(collections)
    profile_modes = collections["02.profile"]
    primitives = collections["01.primitive"]["参数"]
    for mode, values in profile_modes.items():
        namespace = {**primitives, **values}
        if len(namespace) != len(primitives) + len(values):
            raise ValueError(f"sToken {mode} Profile role overlaps a Primitive name")

        def check_profile_alias(name, path=()):
            if name in path:
                raise ValueError(f"Cyclic sToken {mode} Profile alias: {path} -> {name}")
            value = namespace[name]
            alias = VAR.fullmatch(str(value))
            if isinstance(value, str) and "var(" in value and not alias:
                raise ValueError(f"Unsupported sToken {mode} Profile expression: {name}={value}")
            if alias:
                if alias[1] not in namespace:
                    raise ValueError(f"Missing sToken {mode} Profile alias: {name} -> {alias[1]}")
                check_profile_alias(alias[1], (*path, name))

        for name in values:
            check_profile_alias(name)
    for mode, values in collections["04.component"].items():
        for name, value in values.items():
            if not isinstance(value, str) or not VAR.fullmatch(value):
                raise ValueError(f"Component Token must alias an upstream Token: {mode}/{name}={value}")
    device = config["deviceProfile"]
    if device not in profile_modes:
        raise ValueError(f"Unknown sToken device Profile mode: {device}")
    merged = {}
    origins = {}
    for collection, mode in (("01.primitive", "参数"), ("02.profile", device),
                             ("03.semantic", "Light"), ("04.component", "default")):
        for name, value in collections[collection][mode].items():
            if name in merged:
                raise ValueError(f"Ambiguous layered sToken name: {name}")
            merged[name] = value
            origins[name] = f"{collection}/{mode}"
    def inferred(name, seen=()):
        if name in seen:
            raise ValueError(f"Cyclic sToken type chain: {seen} -> {name}")
        value = merged[name]
        alias = VAR.fullmatch(str(value))
        if alias:
            if alias[1] not in merged:
                raise ValueError(f"Missing sToken: {alias[1]}")
            return inferred(alias[1], (*seen, name))
        return token_type(name, value)
    raw, profile_config = copy.deepcopy(raw), copy.deepcopy(profile_config)
    for name, value in merged.items():
        alias = VAR.fullmatch(str(value))
        if alias and alias[1] not in merged:
            raise ValueError(f"Unresolved sToken: {name} -> {alias[1]}")
        if isinstance(value, str) and "var(" in value and not alias:
            raise ValueError(f"Unsupported sToken expression: {name}={value}")
        set_token(raw, key(name), "{" + key(alias[1]) + "}" if alias else value,
                  inferred(name))
    # Alias evaluation, typing and profile precedence remain in the original generator.
    for target, rule in config["mappings"].items():
        if rule["source"] not in merged:
            raise ValueError(f"Missing mapped sToken: {rule['source']}")
        for axis in profile_config["axes"].values():
            for name, mode in axis.items():
                if not name.startswith("$"):
                    mode.get("$overrides", {}).pop(target, None)
    for theme, overrides in config.get("themeOverrides", {}).items():
        dest = profile_config["axes"]["theme"][theme]["$overrides"]
        for target, name in overrides.items():
            dest[target] = {"$value": "{" + key(name) + "}"}
    # Design-authored semantic mode values take precedence over the legacy
    # runtime adapter. Sparse Dark entries inherit Light until reviewed.
    for name, value in collections["03.semantic"].get("Dark", {}).items():
        if name not in collections["03.semantic"]["Light"]:
            raise ValueError(f"Dark semantic role has no Light counterpart: {name}")
        alias = VAR.fullmatch(str(value))
        if not alias or alias[1] not in merged:
            raise ValueError(f"Dark semantic role must alias a known Token: {name}={value}")
        profile_config["axes"]["theme"]["dark"]["$overrides"][key(name)] = {
            "$value": "{" + key(alias[1]) + "}"}
    profile_config["$source"] = {"reference": "sToken local export", "deviceProfile": device,
                                  "modePolicy": config["profilePolicy"]}
    metadata = {"source": "DesignSystem/imports/stoken.json", "deviceProfile": device,
                "sha256": hashlib.sha256(input_path.read_bytes()).hexdigest(),
                "mappingSha256": hashlib.sha256(config_path.read_bytes()).hexdigest(),
                "collections": {c: {m: len(v) for m, v in modes.items()}
                                for c, modes in collections.items()},
                "darkSemanticRoles": sorted(collections["03.semantic"].get("Dark", {})),
                "selectedTokenCount": len(merged), "origins": origins,
                "decisions": config["decisions"]}
    return raw, profile_config, config, metadata


def derived_value(values, rule):
    result = max(values[key(s)] for s in rule["sources"]) + rule.get("add", 0)
    for term in rule.get("terms", []):
        result += values[key(term["source"])] * term["factor"]
    return max(result, rule.get("minimum", result))


def adjust(raw, first_profiles, config, types):
    values = next(iter(first_profiles.values()))
    for target, rule in config["mappings"].items():
        imported = key(rule["source"])
        value = values[imported]
        actual = max(value, rule["minimum"]) if "minimum" in rule else value
        same_type = types[target] == types[imported]
        set_token(raw, target, "{" + imported + "}" if same_type and actual == value else actual)
        rule["typeAdaptation"] = types[imported] + " -> " + types[target]
    for target, rule in config.get("derived", {}).items():
        set_token(raw, target, derived_value(values, rule), rule.get("type", "dimension"))


def finish(root, profiles, token_types, config, metadata, check=False):
    report = {k: v for k, v in metadata.items() if k != "origins"}
    report["profiles"] = {}
    for name, values in profiles.items():
        rows = []
        for target, rule in config["mappings"].items():
            original = values[target]
            actual = max(original, rule["minimum"]) if "minimum" in rule else original
            values[target] = actual
            source_mode = ("03.semantic/Dark" if name.startswith("dark-")
                           and rule["source"] in metadata["darkSemanticRoles"]
                           else metadata["origins"][rule["source"]])
            rows.append({"target": target, "source": rule["source"],
                         "sourceCollection": source_mode,
                         "sourceValue": values[key(rule["source"])],
                         "profileValue": original, "effectiveValue": actual,
                         "typeAdaptation": rule.get("typeAdaptation"),
                         "policy": rule.get("policy", "direct")})
        for target, rule in config.get("derived", {}).items():
            sources = rule["sources"] + [t["source"] for t in rule.get("terms", [])]
            inputs = [values[key(s)] for s in sources]
            actual = derived_value(values, rule)
            values[target] = actual
            token_types[target] = rule.get("type", "dimension")
            rows.append({"target": target, "source": sources, "sourceValue": inputs,
                         "sourceCollection": [metadata["origins"][s] for s in sources],
                         "effectiveValue": actual, "policy": rule["policy"]})
        report["profiles"][name] = rows
    report["mappedTargetCount"] = len(config["mappings"])
    report["derivedTargetCount"] = len(config.get("derived", {}))
    report["componentBindings"] = []
    original_names = {key(name): name for name in metadata["origins"]}
    def bindings(value, path):
        if isinstance(value, dict):
            for part, child in value.items():
                yield from bindings(child, f"{path}.{part}")
        elif isinstance(value, list):
            for index, child in enumerate(value):
                yield from bindings(child, f"{path}[{index}]")
        elif isinstance(value, str) and value.startswith("{base.stoken") and value.endswith("}"):
            yield path, value[1:-1]
    for contract in sorted((root / "tokens").glob("*-contract.json")):
        for field, reference in bindings(load(contract), "$"):
            source = original_names.get(reference)
            report["componentBindings"].append({
                "contract": str(contract.relative_to(root)).replace("\\", "/"),
                "field": field, "token": reference, "source": source,
                "sourceCollection": metadata["origins"].get(source, "project-derived"),
                "values": {name: values[reference] for name, values in profiles.items()}})
    path = root / "DesignSystem/generated/stoken-mapping-report.json"
    encoded = (json.dumps(report, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    if check:
        if not path.exists() or path.read_bytes() != encoded:
            raise ValueError("sToken mapping report drift")
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        # This report is also read by the catalog/check targets during an SDK
        # build. Do not reopen unchanged evidence with a truncating write.
        if not path.exists() or path.read_bytes() != encoded:
            path.write_bytes(encoded)
    return report
