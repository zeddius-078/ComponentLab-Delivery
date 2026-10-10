"""Deterministically normalize a Raw Manifest; never collect or map Qt components."""
import argparse
import collections
import json
import re
import shutil
import tempfile
from pathlib import Path
from contract_io import VERSION, ROOT, load, write, digest, safe_path, pointer, escape, read_calls, validate_schema

ARTIFACTS = {"design-identity": "design-identity.schema.json", "page-structure": "page-structure.schema.json",
             "components": "component-contract.schema.json", "component-variants": "component-variants.schema.json",
             "variables-used": "variables-used.schema.json", "variable-collections": "variable-collections.schema.json",
             "provenance": "provenance.schema.json", "validation": "validation.schema.json"}

def build(manifest_path, source_manifest=None):
    manifest, calls = read_calls(manifest_path)
    roles = collections.defaultdict(list)
    for call_id, (call, parsed) in calls.items():
        roles[call["role"]].append((call_id, parsed))
    def one(role):
        if len(roles[role]) != 1:
            raise ValueError("Expected one call for role: " + role)
        return roles[role][0]
    identity_call, identity = one("file-identity")
    page_call, page = one("page-dsl")
    variables_call, groups = one("variables-full")
    components_call, all_components = one("components-full")
    collections_call, raw_collections = one("variable-collections")
    if identity.get("fileKey") != manifest["fileKey"]:
        raise ValueError("Design file identity mismatch")
    if calls[page_call][0]["arguments"].get("guid") != manifest["pageId"]:
        raise ValueError("Page identity mismatch")
    docs = {k: {"schemaVersion": VERSION} for k in ARTIFACTS}
    provenance = []
    def prov(artifact, out_pointer, sources, rule="copy"):
        provenance.append({"artifact": artifact + ".json", "pointer": out_pointer,
                           "rule": rule, "sources": sources})
    def src(call_id, body_pointer=""):
        return {"callId": call_id, "bodyPointer": body_pointer}
    def fields(artifact, base, value, call_id, raw_base, rename=None):
        rename = rename or {}
        for key in value:
            raw_key = rename.get(key, key)
            prov(artifact, base + "/" + escape(key), [src(call_id, raw_base + "/" + escape(raw_key))],
                 "rename" if raw_key != key else "copy")
    for artifact in docs:
        if artifact not in ("provenance", "validation"):
            prov(artifact, "/schemaVersion", [], "contract-version-1")
    nodes, examples, bindings, instances = [], [], [], []
    roots = []
    identity_instances = []
    exported = {"name", "type", "visible", "box", "autoLayout", "text", "componentSetRef", "componentNodeRef",
                "mainComponent", "variantProperties"}
    def walk(node, parent, call_id, raw_pointer, scope, output):
        if not isinstance(node, dict) or not isinstance(node.get("type"), str):
            raise ValueError("Invalid DSL node")
        locator = call_id + "#" + raw_pointer
        item = {"locator": locator, "nodeId": node.get("id"), "parentLocator": parent,
                "children": [call_id + "#" + raw_pointer + "/children/" + str(i)
                             for i in range(len(node.get("children", [])))]}
        for key in sorted(exported & node.keys()):
            item[{"box": "bounds", "autoLayout": "layout"}.get(key, key)] = node[key]
        base = ("/nodes/" if scope == "page" else "/variantExamples/") + str(len(output))
        output.append(item)
        for key in item:
            raw_key = {"nodeId": "id", "bounds": "box", "layout": "autoLayout"}.get(key, key)
            raw_path = raw_pointer + "/" + escape(raw_key)
            if key in ("locator", "parentLocator", "children") or (key == "nodeId" and "id" not in node):
                raw_path = raw_pointer
            prov("page-structure", base + "/" + key, [src(call_id, raw_path)],
                 "structural-locator" if raw_path == raw_pointer else "copy-or-rename")
        def references(value, property_path, body_path):
            if isinstance(value, str) and value.startswith("variable:"):
                match = re.fullmatch(r"variable:([^$]+)(?:\$(.+))?", value)
                if not match:
                    raise ValueError("Invalid variable binding: " + value)
                binding = {"scope": scope, "nodeId": node.get("id"), "locator": locator,
                           "propertyPath": property_path, "reference": value,
                           "variableId": match[1], "selectorId": match[2]}
                prov("variables-used", "/bindings/" + str(len(bindings)), [src(call_id, body_path)], "parse-variable-reference")
                bindings.append(binding)
            elif isinstance(value, dict):
                for key in sorted(value):
                    if key != "children":
                        references(value[key], property_path + "/" + escape(key), body_path + "/" + escape(key))
            elif isinstance(value, list):
                for i, entry in enumerate(value):
                    references(entry, property_path + "/" + str(i), body_path + "/" + str(i))
        references(node, "", raw_pointer)
        if scope == "page" and node["type"] == "INSTANCE":
            if not node.get("id"):
                raise ValueError("Instance identity missing")
            record = {"instanceNodeId": node["id"], "componentSetId": node.get("componentSetRef"),
                      "variantNodeId": None, "nativeComponentKey": None, "locator": locator}
            main = node.get("mainComponent")
            if isinstance(main, dict):
                record["variantNodeId"] = main.get("id", main.get("nodeId"))
                record["nativeComponentKey"] = main.get("key", main.get("componentKey"))
            identity_instances.append(record)
            prov("design-identity", "/instances/" + str(len(identity_instances)-1), [src(call_id, raw_pointer)], "instance-identity-no-inference")
        for i, child in enumerate(node.get("children", [])):
            walk(child, locator, call_id, raw_pointer + "/children/" + str(i), scope, output)
    for i, node in enumerate(page["roots"]):
        roots.append(page_call + "#/roots/" + str(i))
        walk(node, None, page_call, "/roots/" + str(i), "page", nodes)
    for call_id, dsl in roles["component-variant-dsl"]:
        for i, node in enumerate(dsl["roots"]):
            if node.get("id") != calls[call_id][0]["arguments"].get("guid") or node["type"] != "VARIANT":
                raise ValueError("Variant identity mismatch")
            walk(node, None, call_id, "/roots/" + str(i), "supplemental-variant", examples)
    docs["page-structure"].update(boundsSpace="parent-local", rootLocators=roots, nodes=nodes, variantExamples=examples)
    prov("page-structure", "/rootLocators", [src(page_call, "/roots")], "ordered-root-locators")
    prov("page-structure", "/boundsSpace", [], "Pixso-DSL-box-parent-local")
    refs = []
    for call_id, dsl in [(page_call, page)] + roles["state-group-dsl"]:
        if call_id != page_call:
            guid = calls[call_id][0]["arguments"].get("guid")
            if len(dsl["roots"]) != 1 or dsl["roots"][0].get("id") != guid:
                raise ValueError("State group identity mismatch")
        for i, ref in enumerate(dsl.get("refsIndex", {}).get("componentSets", [])):
            refs.append((ref, call_id, "/refsIndex/componentSets/" + str(i)))
    component_map = {}
    for i, component in enumerate(all_components):
        cid = component.get("node_id")
        if cid in component_map:
            raise ValueError("Duplicate native component node")
        component_map[cid] = (component, i)
    contracts, variants, set_ids, variant_ids = [], [], [], []
    seen_sets, seen_variants = set(), set()
    for ref, call_id, raw_base in refs:
        set_id = ref["id"]
        if set_id in seen_sets:
            raise ValueError("Duplicate component set definition")
        seen_sets.add(set_id)
        record = {"componentSetId": set_id, "designName": ref.get("name"),
                  "properties": ref.get("propertyDefinitions", []),
                  "variantNodeIds": [v["id"] for v in ref.get("variants", [])],
                  "mappingKey": manifest["fileKey"] + ":" + set_id}
        contracts.append(record)
        prov("components", "/components/" + str(len(contracts)-1), [src(call_id, raw_base)], "component-contract-projection")
        set_ids.append({"componentSetId": set_id, "mappingKey": record["mappingKey"]})
        prov("design-identity", "/componentSets/" + str(len(set_ids)-1), [src(call_id, raw_base + "/id")], "local-mapping-key-not-native-key")
        for j, variant in enumerate(ref.get("variants", [])):
            vid = variant["id"]
            if vid in seen_variants:
                raise ValueError("Duplicate variant identity")
            seen_variants.add(vid)
            native, native_i = component_map.get(vid, ({}, -1))
            if native.get("file_key") != manifest["fileKey"] or not native.get("component_key"):
                raise ValueError("Variant key missing or wrong file: " + vid)
            properties = variant.get("properties", [])
            if len({p["name"] for p in properties}) != len(properties):
                raise ValueError("Duplicate Variant property")
            record = {"componentSetId": set_id, "variantNodeId": vid, "name": variant.get("name"),
                      "properties": properties, "nativeComponentKey": native["component_key"]}
            sources = [src(call_id, raw_base + "/variants/" + str(j)), src(components_call, "/" + str(native_i))]
            variants.append(record)
            prov("component-variants", "/variants/" + str(len(variants)-1), sources, "join-by-variant-node-id")
            variant_ids.append({k: record[k] for k in ("componentSetId", "variantNodeId", "nativeComponentKey")})
            prov("design-identity", "/variants/" + str(len(variant_ids)-1), sources, "variant-identity-projection")
    for instance in identity_instances:
        if instance["componentSetId"] not in seen_sets:
            raise ValueError("Instance references missing component family")
        if instance["variantNodeId"]:
            found = next((v for v in variant_ids if v["variantNodeId"] == instance["variantNodeId"]), None)
            if not found or found["componentSetId"] != instance["componentSetId"]:
                raise ValueError("Instance variant mismatch")
            if instance["nativeComponentKey"] and instance["nativeComponentKey"] != found["nativeComponentKey"]:
                raise ValueError("Instance native key mismatch")
    docs["components"]["components"] = contracts
    docs["component-variants"]["variants"] = variants
    docs["design-identity"].update(provider=manifest["provider"], fileKey=manifest["fileKey"], pageId=manifest["pageId"],
                                    componentSets=set_ids, variants=variant_ids, instances=identity_instances)
    for key in ("provider", "fileKey", "pageId"):
        prov("design-identity", "/" + key, [src(identity_call)] if key == "fileKey" else [], "manifest-identity")
    variable_map = {}
    for group_id, items in groups.items():
        for i, item in enumerate(items):
            if item["id"] in variable_map:
                raise ValueError("Duplicate variable ID")
            variable_map[item["id"]] = (group_id, item, "/" + escape(group_id) + "/" + str(i))
    collection_map = {c["id"]: (c, i) for i, c in enumerate(raw_collections)}
    used_collections = set()
    def resolve(variable_id, mode_name):
        chain, sources, visited = [], [], set()
        while True:
            if variable_id in visited:
                raise ValueError("Alias cycle: " + variable_id)
            visited.add(variable_id)
            if variable_id not in variable_map:
                raise ValueError("Alias target missing: " + variable_id)
            group_id, item, raw_base = variable_map[variable_id]
            if group_id not in collection_map:
                raise ValueError("Alias mode labels unavailable: " + group_id)
            collection, ci = collection_map[group_id]
            modes = [m for m in collection["modes"] if m["name"] == mode_name]
            if len(modes) != 1 or modes[0]["modeId"] not in item.get("modeValues", {}):
                raise ValueError("Alias profile mode missing: " + mode_name)
            mid = modes[0]["modeId"]
            value = item["modeValues"][mid]["value"]
            chain.append({"variableId": variable_id, "variableName": item["name"], "collectionId": group_id,
                          "modeId": mid, "rawValue": value})
            sources += [src(variables_call, raw_base), src(collections_call, "/" + str(ci))]
            used_collections.add(group_id)
            if isinstance(value, dict) and isinstance(value.get("id"), str):
                variable_id = value["id"]
            else:
                return chain, value, sources
    used = sorted({b["variableId"] for b in bindings})
    variables = []
    for variable_id in used:
        if variable_id not in variable_map:
            raise ValueError("Referenced variable missing: " + variable_id)
        group_id, item, raw_base = variable_map[variable_id]
        collection, ci = collection_map[group_id]
        mode_map = {m["modeId"]: m["name"] for m in collection["modes"]}
        record = {"variableId": variable_id, "name": item["name"], "collectionId": group_id,
                  "nativeVariableKey": item.get("key"), "resolvedType": item.get("resolvedType"),
                  "scopes": sorted({b["scope"] for b in bindings if b["variableId"] == variable_id}), "modes": []}
        base = "/variables/" + str(len(variables))
        prov("variables-used", base, [src(variables_call, raw_base)], "used-variable-projection")
        for mid, payload in sorted(item.get("modeValues", {}).items()):
            if mid not in mode_map:
                raise ValueError("Unknown mode ID: " + mid)
            chain, final, sources = resolve(variable_id, mode_map[mid])
            record["modes"].append({"modeId": mid, "modeName": mode_map[mid], "rawPayload": payload,
                                    "aliasChain": chain, "finalValue": final, "status": "resolved"})
            prov("variables-used", base + "/modes/" + str(len(record["modes"])-1), sources, "alias-join-by-exact-unique-mode-name-no-fallback")
        variables.append(record)
    docs["variables-used"].update(variables=variables, bindings=bindings)
    docs["variable-collections"]["collections"] = []
    for group_id in sorted(used_collections):
        c, i = collection_map[group_id]
        record = {"collectionId": c["id"], "name": c["name"], "defaultModeId": c["defaultModeId"], "modes": c["modes"]}
        docs["variable-collections"]["collections"].append(record)
        fields("variable-collections", "/collections/" + str(len(docs["variable-collections"]["collections"])-1),
               record, collections_call, "/" + str(i), {"collectionId": "id"})
    counts = {"componentSets": len(contracts), "instances": len(identity_instances), "variants": len(variants),
              "pageRoots": len(roots), "pageNodes": len(nodes), "supplementalVariantNodes": len(examples),
              "pageReferencedVariables": len({b["variableId"] for b in bindings if b["scope"] == "page"}),
              "supplementalReferencedVariables": len({b["variableId"] for b in bindings if b["scope"] != "page"}),
              "referencedVariables": len(variables), "pageBindings": sum(b["scope"] == "page" for b in bindings),
              "supplementalBindings": sum(b["scope"] != "page" for b in bindings),
              "resolvedModeValues": sum(len(v["modes"]) for v in variables)}
    docs["provenance"].update(sourceManifest=source_manifest or "runs/" + manifest["captureId"] + "/manifest.json",
                               manifestSha256=digest(Path(manifest_path).read_bytes()),
                               calls=[call for call, parsed in calls.values()], mappings=provenance)
    docs["validation"].update(status="passed", checks={k: "passed" for k in
               ("schema", "identity", "components", "variables", "alias", "hash", "provenance")})
    for name, document in docs.items():
        validate_schema(document, ARTIFACTS[name])
    return manifest, docs, counts

def normalize(manifest_path, output, source_manifest=None):
    output = Path(output).resolve()
    manifest_path = Path(manifest_path).resolve()
    if output.name != "canonical" or not output.parent.is_relative_to(ROOT / "packages"):
        raise ValueError("Output must be packages/<package>/canonical")
    source_manifest = source_manifest or manifest_path.relative_to(output.parent).as_posix()
    manifest, docs, counts = build(manifest_path, source_manifest)
    if manifest["packageId"] != output.parent.name:
        raise ValueError("Package directory identity mismatch")
    output.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=".canonical-stage-", dir=output.parent))
    try:
        artifacts = {}
        for name, doc in docs.items():
            artifacts[name] = {**write(stage / (name + ".json"), doc), "schema": ARTIFACTS[name]}
        index = {"schemaVersion": VERSION, "packageId": manifest["packageId"], "provider": manifest["provider"],
                 "fileKey": manifest["fileKey"], "pageId": manifest["pageId"], "captureId": manifest["captureId"],
                 "generatedAt": manifest["capturedAt"], "sourceManifest": docs["provenance"]["sourceManifest"],
                 "sourceManifestSha256": docs["provenance"]["manifestSha256"], "artifacts": artifacts,
                 "counts": counts, "validationStatus": "passed"}
        validate_schema(index, "package-index.schema.json")
        write(stage / "index.json", index)
        from verify import verify
        verify(stage / "index.json", manifest_path)
        current = output.parent / "current.json"
        if output.exists() and current.exists() and all(
                (output / p.name).is_file() and (output / p.name).read_bytes() == p.read_bytes()
                for p in stage.iterdir()):
            verify(output / "index.json", manifest_path, current)
            return index
        backup = output.with_name("." + output.name + "-previous")
        current = output.parent / "current.json"
        previous_current = current.read_bytes() if current.exists() else None
        if backup.exists():
            raise ValueError("Previous interrupted publish needs recovery: " + str(backup))
        if output.exists():
            output.rename(backup)
        try:
            stage.rename(output)
            authority = {"schemaVersion": VERSION, "packageId": index["packageId"], "captureId": index["captureId"],
                         "sourceManifest": index["sourceManifest"], "sourceManifestSha256": index["sourceManifestSha256"],
                         "canonicalIndex": "canonical/index.json", "canonicalIndexSha256": digest((output / "index.json").read_bytes())}
            validate_schema(authority, "current.schema.json")
            pending = current.with_name("current.pending.json")
            write(pending, authority)
            pending.replace(current)
            verify(output / "index.json", manifest_path, current)
        except Exception:
            failed = output.with_name(".canonical-failed")
            if output.exists():
                output.rename(failed)
            if backup.exists():
                backup.rename(output)
            if previous_current is not None:
                current.write_bytes(previous_current)
            elif current.exists():
                current.unlink()
            if failed.exists() and failed.resolve().is_relative_to(ROOT / "packages"):
                shutil.rmtree(failed)
            raise
        if backup.exists():
            history = ROOT / "archive" / index["packageId"] / "canonical-history" / digest((backup / "index.json").read_bytes())
            history.parent.mkdir(parents=True, exist_ok=True)
            if history.exists():
                if not backup.resolve().is_relative_to(ROOT / "packages"):
                    raise ValueError("Unsafe backup cleanup path")
                shutil.rmtree(backup)
            else:
                backup.rename(history)
            if previous_current is not None:
                (history / "current.json").write_bytes(previous_current)
        return index
    finally:
        if stage.exists() and stage.resolve().is_relative_to(ROOT / "packages"):
            shutil.rmtree(stage)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(normalize(args.manifest, args.output)["counts"], ensure_ascii=False, sort_keys=True))
