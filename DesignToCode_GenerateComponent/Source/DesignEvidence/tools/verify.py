"""Offline schema, hash, identity, alias and provenance verification."""
import argparse
import json
from pathlib import Path
from contract_io import load, digest, safe_path, pointer, read_calls, validate_schema

def verify_canonical(index_path):
    index_path = Path(index_path)
    index = load(index_path)
    validate_schema(index, "package-index.schema.json")
    documents = {}
    for name, artifact in index["artifacts"].items():
        path = safe_path(index_path.parent, artifact["path"])
        raw = path.read_bytes()
        if len(raw) != artifact["bytes"] or digest(raw) != artifact["sha256"]:
            raise ValueError("Canonical artifact hash mismatch: " + name)
        document = json.loads(raw)
        validate_schema(document, artifact["schema"])
        documents[name] = document
    identity = documents["design-identity"]
    if any(identity[k] != index[k] for k in ("provider", "fileKey", "pageId")):
        raise ValueError("Canonical design identity mismatch")
    sets = {s["componentSetId"]: s for s in documents["components"]["components"]}
    variants = {v["variantNodeId"]: v for v in documents["component-variants"]["variants"]}
    if len(sets) != len(documents["components"]["components"]) or len(variants) != len(documents["component-variants"]["variants"]):
        raise ValueError("Duplicate component identity")
    if {s["componentSetId"] for s in identity["componentSets"]} != set(sets):
        raise ValueError("Component identity index mismatch")
    for variant in identity["variants"]:
        if variant != {k: variants.get(variant["variantNodeId"], {}).get(k) for k in variant}:
            raise ValueError("Variant identity index mismatch")
    if len(identity["variants"]) != len(variants):
        raise ValueError("Variant identity count mismatch")
    for s in sets.values():
        if s["mappingKey"] != index["fileKey"] + ":" + s["componentSetId"]:
            raise ValueError("Mapping key mismatch")
        if len(s["variantNodeIds"]) != len(set(s["variantNodeIds"])):
            raise ValueError("Duplicate variant in component family")
        for vid in s["variantNodeIds"]:
            if vid not in variants or variants[vid]["componentSetId"] != s["componentSetId"]:
                raise ValueError("Variant family mismatch")
    structure = documents["page-structure"]
    nodes = {n["locator"]: n for n in structure["nodes"] + structure["variantExamples"]}
    if len(nodes) != len(structure["nodes"]) + len(structure["variantExamples"]):
        raise ValueError("Duplicate structural locator")
    for node in nodes.values():
        for child in node["children"]:
            if child not in nodes or nodes[child]["parentLocator"] != node["locator"]:
                raise ValueError("Broken node hierarchy")
        if node["parentLocator"] is not None and node["parentLocator"] not in nodes:
            raise ValueError("Missing parent")
    page_nodes = {n["locator"] for n in structure["nodes"]}
    if set(structure["rootLocators"]) != {n["locator"] for n in structure["nodes"] if n["parentLocator"] is None}:
        raise ValueError("Page root mismatch")
    if len({i["instanceNodeId"] for i in identity["instances"]}) != len(identity["instances"]):
        raise ValueError("Duplicate instance identity")
    for instance in identity["instances"]:
        node = nodes.get(instance["locator"], {})
        if node.get("type") != "INSTANCE" or node.get("nodeId") != instance["instanceNodeId"] or instance["componentSetId"] not in sets:
            raise ValueError("Instance identity mismatch")
    used = documents["variables-used"]
    variables = {v["variableId"]: v for v in used["variables"]}
    collections = {c["collectionId"]: c for c in documents["variable-collections"]["collections"]}
    if len(variables) != len(used["variables"]) or set(variables) != {b["variableId"] for b in used["bindings"]}:
        raise ValueError("Variables must be exactly binding references")
    for b in used["bindings"]:
        if b["locator"] not in nodes or nodes[b["locator"]]["nodeId"] != b["nodeId"]:
            raise ValueError("Binding locator mismatch")
        if (b["scope"] == "page") != (b["locator"] in page_nodes):
            raise ValueError("Binding scope mismatch")
    for variable in variables.values():
        if not variable["modes"]:
            raise ValueError("Missing variable modes")
        for mode in variable["modes"]:
            chain = mode["aliasChain"]
            if not chain or chain[0]["variableId"] != variable["variableId"] or len({c["variableId"] for c in chain}) != len(chain):
                raise ValueError("Broken or cyclic alias chain")
            for i, hop in enumerate(chain):
                collection = collections.get(hop["collectionId"], {})
                matching = [m for m in collection.get("modes", []) if m["modeId"] == hop["modeId"] and m["name"] == mode["modeName"]]
                if len(matching) != 1:
                    raise ValueError("Alias mode mismatch")
                if i + 1 < len(chain) and hop["rawValue"] != {"id": chain[i+1]["variableId"]}:
                    raise ValueError("Alias target mismatch")
            if chain[-1]["rawValue"] != mode["finalValue"]:
                raise ValueError("Alias final value mismatch")
    expected = {"componentSets":len(sets),"variants":len(variants),"instances":len(identity["instances"]),
                "pageRoots":len(structure["rootLocators"]),"pageNodes":len(structure["nodes"]),
                "supplementalVariantNodes":len(structure["variantExamples"]),"referencedVariables":len(variables),
                "pageReferencedVariables":len({b["variableId"] for b in used["bindings"] if b["scope"]=="page"}),
                "supplementalReferencedVariables":len({b["variableId"] for b in used["bindings"] if b["scope"]!="page"}),
                "pageBindings":sum(b["scope"]=="page" for b in used["bindings"]),
                "supplementalBindings":sum(b["scope"]!="page" for b in used["bindings"]),
                "resolvedModeValues":sum(len(v["modes"]) for v in variables.values())}
    if expected != index["counts"]:
        raise ValueError("Canonical counts mismatch")
    provenance = documents["provenance"]
    if provenance["sourceManifest"] != index["sourceManifest"] or provenance["manifestSha256"] != index["sourceManifestSha256"]:
        raise ValueError("Provenance manifest mismatch")
    call_ids = {c["callId"] for c in provenance["calls"]}
    covered = {}
    for mapping in provenance["mappings"]:
        name = mapping["artifact"].removesuffix(".json")
        if name not in documents:
            raise ValueError("Unknown provenance artifact")
        pointer(documents[name], mapping["pointer"])
        if any(s["callId"] not in call_ids for s in mapping["sources"]):
            raise ValueError("Missing provenance call")
        covered.setdefault(name, []).append(mapping["pointer"])
    def leaves(value, path=""):
        if isinstance(value, dict):
            for k,v in value.items():
                from contract_io import escape
                yield from leaves(v,path+"/"+escape(k))
        elif isinstance(value, list):
            for i,v in enumerate(value):
                yield from leaves(v,path+"/"+str(i))
        else:
            yield path
    for name, document in documents.items():
        if name in ("provenance", "validation"):
            continue
        for leaf in leaves(document):
            if not any(leaf == p or leaf.startswith(p + "/") for p in covered.get(name, [])):
                raise ValueError("Missing provenance coverage: " + name + leaf)
    return index, documents

def verify(index_path, manifest_path=None, current_path=None):
    index, docs = verify_canonical(index_path)
    if manifest_path is not None:
        manifest_path = Path(manifest_path)
        if digest(manifest_path.read_bytes()) != index["sourceManifestSha256"]:
            raise ValueError("Manifest hash mismatch")
        manifest, calls = read_calls(manifest_path)
        for key in ("packageId", "provider", "fileKey", "pageId", "captureId"):
            if index[key] != manifest[key]:
                raise ValueError("Manifest identity mismatch: " + key)
        if docs["provenance"]["calls"] != [c for c,p in calls.values()]:
            raise ValueError("Provenance call metadata mismatch")
        for mapping in docs["provenance"]["mappings"]:
            for source in mapping["sources"]:
                pointer(calls[source["callId"]][1], source["bodyPointer"])
        from normalize import build
        _, expected, counts = build(manifest_path, index["sourceManifest"])
        if expected != docs or counts != index["counts"] or index["generatedAt"] != manifest["capturedAt"]:
            raise ValueError("Canonical does not reproduce from Raw")
    if current_path is not None:
        current_path = Path(current_path)
        current = load(current_path)
        validate_schema(current, "current.schema.json")
        for key in ("packageId", "captureId", "sourceManifest", "sourceManifestSha256"):
            if current[key] != index[key]:
                raise ValueError("Current authority mismatch")
        target = safe_path(current_path.parent, current["canonicalIndex"])
        if target != Path(index_path).resolve() or digest(target.read_bytes()) != current["canonicalIndexSha256"]:
            raise ValueError("Current index hash mismatch")
        raw_manifest = safe_path(current_path.parent, current["sourceManifest"])
        if digest(raw_manifest.read_bytes()) != current["sourceManifestSha256"]:
            raise ValueError("Current manifest hash mismatch")
    return {"status":"passed", "counts":index["counts"], "rawAudited":manifest_path is not None}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index", required=True, type=Path)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--current", type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.index, args.manifest, args.current), ensure_ascii=False, sort_keys=True))
