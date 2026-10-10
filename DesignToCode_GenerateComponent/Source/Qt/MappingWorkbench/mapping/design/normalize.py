"""Convert an approved Figma snapshot to the existing Qt UI Contract."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from snapshot import source as snapshot_source, design_tree
ROOT = HERE.parents[3] / "ComponentKit"
MAX_BYTES = 1024 * 1024


def require(ok, message):
    if not ok:
        raise ValueError(message)


def read_json(path):
    raw = path.read_bytes()
    require(len(raw) <= MAX_BYTES, f"Input exceeds 1 MiB: {path}")
    return raw, json.loads(raw.decode("utf-8-sig"))


def normalize(raw, registry, variable_index, component_map, reviewed_sha256=None):
    require(len(raw) <= MAX_BYTES, "Snapshot exceeds 1 MiB")
    snapshot = json.loads(raw.decode("utf-8-sig"))
    require(registry.get("schemaVersion") == 1, "Registry schema must be 1")
    require(registry["status"] == "READY", "Design component registry is not READY")
    require(registry["mappingId"] == component_map["id"], "Mapping version mismatch")
    require(snapshot.get("schemaVersion") == 1 and snapshot.get("provider") == "figma",
            "Expected Figma snapshot schema 1")
    require(snapshot.get("fileKey") == registry["fileKey"] == variable_index["fileKey"],
            "Figma file identity mismatch")
    variables = {v["id"]: v for v in variable_index["variables"]}
    profiles = [m["name"] for m in variable_index["collections"]["runtime"]["modes"]]
    profile = snapshot.get("profile")
    require(profile in profiles, "Snapshot Profile is not registered")
    ids, instance_ids = set(), set()
    count = 0
    layout_paths = {"itemSpacing": "base.stoken.metric.space.vertical.standard",
                    **{k: "base.stoken.layout.padding.inline.compact" for k in
                       ("paddingLeft", "paddingTop", "paddingRight", "paddingBottom")}}

    def walk(node, depth=0):
        nonlocal count
        count += 1
        require(count <= 64 and depth <= 8, "Node or depth limit exceeded")
        name = node.get("name", "")
        require(isinstance(name, str) and re.fullmatch(r"[A-Za-z][A-Za-z0-9_]{0,63}", name),
                f"Invalid Qt object name: {name}")
        require(name not in ids, f"Duplicate object name: {name}")
        ids.add(name)
        require(node.get("id") and node["id"] not in instance_ids, f"Duplicate/missing node id: {name}")
        instance_ids.add(node["id"])
        kind = node.get("type")
        if kind == "FRAME":
            direction = node.get("layoutMode")
            require(direction in ("VERTICAL", "HORIZONTAL"), f"{name}: auto-layout required")
            require(not node.get("absoluteChildren"), f"{name}: absolute-positioned children unsupported")
            bindings = node.get("boundVariables", {})
            for key, path in layout_paths.items():
                binding = variables.get(bindings.get(key, {}).get("id"))
                require(binding and binding["sourcePath"] == path, f"{name}.{key}: expected Token {path}")
                require(node.get(key) == binding["values"][profile], f"{name}.{key}: resolved value mismatch")
            children = node.get("children")
            require(isinstance(children, list) and children, f"{name}: children required")
            return {"id": name, "component": "Column" if direction == "VERTICAL" else "Row",
                    "layout": {"spacing": layout_paths["itemSpacing"], "margin": layout_paths["paddingLeft"]},
                    "children": [walk(c, depth + 1) for c in children]}
        require(kind == "INSTANCE", f"{name}: use a registered component instance")
        identity = node.get("mainComponent", {})
        rule = registry["components"].get(identity.get("key"))
        require(rule and rule["nodeId"] == identity.get("id"), f"{name}: unregistered main component")
        require(isinstance(node.get("visualOverrides"), list) and not node["visualOverrides"],
                f"{name}: missing override check or unapproved visual override")
        family = rule["family"]
        definition = component_map["components"].get(family)
        require(definition, f"{name}: unregistered Qt family")
        if "extends" in definition:
            definition = component_map["components"][definition["extends"]]
        properties = {}
        source = node.get("componentProperties", {})
        require(set(source) == set(rule["bindings"]), f"{name}: design property set changed")
        for source_key, binding in rule["bindings"].items():
            item = source[source_key]
            require(item.get("type") == binding["figmaType"], f"{name}.{source_key}: Figma type mismatch")
            value = item.get("value")
            target = binding.get("target")
            if binding["kind"] == "state":
                require(value in binding["values"], f"{name}: unsupported design state {value}")
                translated = binding["values"][value]
            elif binding["kind"] == "icon":
                identity = node.get("iconComponents", {}).get(value, {})
                asset = registry["assets"].get(identity.get("key"))
                require(asset and asset["nodeId"] == identity.get("id"), f"{name}: icon not registered")
                require(asset["assetKey"] in component_map["assets"], f"{name}: icon missing in QRC map")
                translated = {target: asset["assetKey"]}
            elif binding["kind"] == "dataset":
                require(value in registry["datasets"], f"{name}: dataset not registered")
                translated = registry["datasets"][value]
            elif binding["kind"] == "enum":
                require(value in binding["values"], f"{name}.{source_key}: unsupported value {value}")
                translated = {target: binding["values"][value]}
            else:
                require(binding["kind"] in ("text", "boolean"), f"{name}: unsupported binding kind")
                expected = str if binding["kind"] == "text" else bool
                require(type(value) is expected, f"{name}.{source_key}: typed value required")
                translated = {target: value}
            for key, item_value in translated.items():
                require(key in definition["properties"], f"{name}.{key}: Qt property not exposed")
                require(key not in properties, f"{name}.{key}: conflicting bindings")
                properties[key] = item_value
        size_spec = definition["properties"]["controlSize"]
        size = properties.get("controlSize", size_spec["default"])
        require(size in size_spec["values"], f"{name}: unsupported size")
        token_size = size_spec.get("tokenValues", {}).get(size, size)
        overrides = definition.get("tokenRefOverrides", {}).get(size, {})
        refs = [overrides.get(path, path.replace(".size.md.", f".size.{token_size}.")
                            .replace(".sizes.sm.", f".sizes.{token_size}."))
                for path in definition["tokenRefs"]]
        return {"id": name, "component": family, "properties": properties, "tokenRefs": refs,
                "design": {"instanceNodeId": node["id"], "componentKey": node["mainComponent"]["key"],
                           "libraryVersion": registry["libraryVersion"], "alias": family}}

    page = walk(snapshot["root"])
    require(page["component"] in ("Column", "Row"), "Root must be an auto-layout FRAME")
    return {"schemaVersion": 1, "mappingId": component_map["id"],
            "source": snapshot_source(raw, "figmaNormalized",
                                      f"https://www.figma.com/design/{registry['fileKey']}",
                                      snapshot["root"]["id"], reviewed_sha256), "page": page}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--registry", type=Path, default=HERE / "figma-registry.json")
    parser.add_argument("--variables", type=Path, default=HERE / "figma-variables.json")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--reviewed-sha256", required=True, help="SHA-256 of the inspected snapshot")
    args = parser.parse_args()
    tree_path = args.output.with_suffix(".design-tree.json")
    require(not args.output.exists() and not tree_path.exists(), "Output exists; choose a new output path")
    raw, _ = read_json(args.snapshot)
    _, registry = read_json(args.registry)
    _, variables = read_json(args.variables)
    _, mapping = read_json(HERE.parent / "qt-component-map.json")
    for name, expected in variables["sourceHashes"].items():
        require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected,
                f"Figma variable snapshot is stale: {name}")
    contract = normalize(raw, registry, variables, mapping, args.reviewed_sha256)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(contract, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    tree_path.write_text(json.dumps(design_tree(raw), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared registered contract; Qt validation required: {args.output}")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, TypeError, OSError) as error:
        raise SystemExit(f"Mapping rejected: {error}") from error
