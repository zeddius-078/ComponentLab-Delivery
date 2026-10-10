"""Capture reproducible Pixso MCP evidence without modifying the design file."""

import argparse
import collections
import copy
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from urllib.request import Request, urlopen


DEFAULT_ENDPOINT = "http://127.0.0.1:3667/mcp"
FILE_KEY = "4S0Ktm81WnvzuJmZHvRuOg"


class McpClient:
    def __init__(self, endpoint):
        self.endpoint = endpoint
        self.headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
        self.request_id = 0

    def rpc(self, method, params):
        self.request_id += 1
        payload = {"jsonrpc": "2.0", "id": self.request_id, "method": method, "params": params}
        request = Request(self.endpoint, json.dumps(payload).encode("utf-8"), self.headers)
        with urlopen(request, timeout=90) as response:
            session = response.headers.get("Mcp-Session-Id")
            if session:
                self.headers["Mcp-Session-Id"] = session
            body = response.read().decode("utf-8")
        if body.lstrip().startswith("{"):
            result = json.loads(body)
        else:
            messages = [json.loads(line[5:].strip()) for line in body.splitlines() if line.startswith("data:")]
            result = next(item for item in messages if item.get("id") == self.request_id)
        if "error" in result:
            raise RuntimeError(f"MCP {method}: {result['error']}")
        return result

    def initialize(self):
        result = self.rpc("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                                         "clientInfo": {"name": "pixso-evidence-capture", "version": "1"}})
        # A notification has no response body and must not reuse rpc().
        payload = {"jsonrpc": "2.0", "method": "notifications/initialized", "params": {}}
        with urlopen(Request(self.endpoint, json.dumps(payload).encode("utf-8"), self.headers), timeout=30):
            pass
        return result

    def tool(self, name, arguments):
        response = self.rpc("tools/call", {"name": name, "arguments": arguments})
        result = response["result"]
        if result.get("isError"):
            raise RuntimeError(f"Pixso {name}: {result}")
        return response


def decoded_tool_value(response):
    blocks = response["result"]["content"]
    text = next(block["text"] for block in blocks if block.get("type") == "text")
    return json.loads(text)


def save_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    raw = (json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    path.write_bytes(raw)
    return {"path": path.name, "bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}


def copy_source(path, target):
    raw = path.read_bytes()
    target.write_bytes(raw)
    return {"path": target.name, "sourcePath": str(path), "bytes": len(raw),
            "sha256": hashlib.sha256(raw).hexdigest()}


def build_evidence(page, variants):
    if page.get("provider") != "pixso" or page.get("fileKey") != FILE_KEY:
        raise ValueError("Snapshot is not the registered Pixso DesignTest file")
    if page.get("nodeId") != "2:145":
        raise ValueError("Snapshot is not the registered component-library page")
    roots = page["dsl"]["roots"]
    nodes = []
    component_sets = []
    instances = []
    bindings = []
    types = collections.Counter()
    seen = set()

    def walk(node, parent_id, parent_locator, source, locator):
        node_id = node.get("id")
        if node_id is not None:
            identity = (source, node_id)
            if identity in seen:
                raise ValueError(f"Duplicate node ID in {source}: {node_id}")
            seen.add(identity)
        node_type = node["type"]
        types[node_type] += 1
        children = node.get("children", [])
        for index, child in enumerate(children):
            if not isinstance(child, dict):
                raise ValueError(f"Unsupported child at {source}/{node_id}[{index}]: {str(child)[:300]}")
        nodes.append({"source": source, "locator": locator, "nodeId": node_id,
                      "componentNodeRef": node.get("componentNodeRef"),
                      "parentId": parent_id, "parentLocator": parent_locator,
                      "name": node.get("name"), "type": node_type,
                      "children": [{"nodeId": child.get("id"), "componentNodeRef": child.get("componentNodeRef"),
                                    "locator": f"{locator}.children[{index}]"}
                                   for index, child in enumerate(children)],
                      "bounds": node.get("box"), "layout": node.get("autoLayout")})
        if node_type == "COMPONENT_SET":
            component_sets.append({"nodeId": node_id, "name": node.get("name"),
                                   "bounds": node.get("box"),
                                   "source": source})
        if node_type == "INSTANCE":
            instances.append({"nodeId": node_id, "locator": locator, "name": node.get("name"),
                              "parentId": parent_id,
                              "componentSetRef": node.get("componentSetRef"),
                              "mainComponent": node.get("mainComponent"),
                              "source": source})

        def references(value, path):
            if isinstance(value, str) and value.startswith("variable:"):
                match = re.fullmatch(r"variable:([^$]+)(?:\$(.+))?", value)
                bindings.append({"source": source, "nodeId": node_id, "locator": locator,
                                 "propertyPath": path,
                                 "reference": value, "variableId": match.group(1) if match else None,
                                 "selectorId": match.group(2) if match else None})
            elif isinstance(value, dict):
                for key, item in value.items():
                    if key != "children":
                        references(item, f"{path}.{key}" if path else key)
            elif isinstance(value, list):
                for index, item in enumerate(value):
                    references(item, f"{path}[{index}]")

        references(node, "")
        for index, child in enumerate(children):
            walk(child, node_id, locator, source, f"{locator}.children[{index}]")

    for index, root in enumerate(roots):
        walk(root, None, None, "page-2-145", f"roots[{index}]")
    variant_examples = []
    for variant in variants:
        if variant.get("provider") != "pixso" or variant.get("fileKey") != FILE_KEY:
            raise ValueError("Variant snapshot is from another design file")
        for root in variant["dsl"]["roots"]:
            if root["type"] != "VARIANT" or root["id"] != variant["nodeId"]:
                raise ValueError("Variant root and requested node ID disagree")
            variant_examples.append({"variantNodeId": root["id"],
                                     "componentSetRef": root.get("componentSetRef"),
                                     "variantProperties": root.get("variantProperties", []),
                                     "name": root.get("name")})
            walk(root, None, None, f"variant-{root['id']}", "roots[0]")

    variable_catalog = {item["id"]: item for item in page["dsl"]["refsIndex"].get("variables", [])}
    use_count = collections.Counter(item["variableId"] for item in bindings if item["variableId"])
    referenced_sets = page["dsl"]["refsIndex"].get("componentSets", [])
    component_registry = [
        {"componentSetId": item["id"], "designName": item.get("name"),
         "properties": item.get("propertyDefinitions", []),
         "variantCount": len(item.get("variants", [])),
         "variantNodeIds": [variant["id"] for variant in item.get("variants", [])]}
        for item in referenced_sets
    ]
    variables = [{"variableId": variable_id, "name": item.get("name"),
                  "bindingCount": use_count[variable_id], "resolvedValues": None,
                  "valueEvidence": "not-present-in-snapshot"}
                 for variable_id, item in sorted(variable_catalog.items())]
    return {
        "schemaVersion": 1,
        "evidenceLevel": "derived-from-preserved-local-snapshot; not-a-fresh-MCP-response",
        "designFile": {"provider": "pixso", "fileKey": FILE_KEY, "requestedPageNodeId": "2:145",
                       "url": f"https://pixso.cn/app/design/{FILE_KEY}?page-id=2%3A145"},
        "sourceStatsAsReported": page["dsl"].get("stats"),
        "counts": {"pageRoots": len(roots), "allIndexedNodes": len(nodes),
                   "pageComponentSets": len(component_sets), "allIndexedInstances": len(instances),
                   "referencedComponentSetsWithPropertyDefinitions": len(component_registry),
                   "referencedVariants": sum(item["variantCount"] for item in component_registry),
                   "idlessOverrideNodes": sum(item["nodeId"] is None for item in nodes),
                   "variablesNamedInPageRefsIndex": len(variables),
                   "variableBindingOccurrences": len(bindings), "indexedTypes": dict(sorted(types.items()))},
        "componentSets": component_sets,
        "componentRegistry": component_registry,
        "referencedComponentSets": referenced_sets,
        "instances": instances,
        "variables": variables,
        "variableBindings": bindings,
        "variantExamples": variant_examples,
        "nodes": nodes,
        "limitations": [
            "This is an exact copy of a prior local get_node_dsl snapshot, not a new live MCP capture.",
            "The snapshot reports variableMap counts but does not include the complete variable definitions or profile values.",
            "A COMPONENT_SET root node ID and an INSTANCE componentSetRef are separate identifiers; no equality is inferred.",
            "Native Pixso component keys and page-instance override values are recorded only when present in the source node.",
        ],
    }


def build_live_evidence(output):
    def read(name):
        response = json.loads((output / name).read_bytes())
        return decoded_tool_value(response)

    page_dsl = copy.deepcopy(read("03-page-dsl.json"))
    variant_dsl = read("04-button-variant-dsl.json")
    groups = read("06-variables.json")
    components = read("07-all-components.json")
    collections = read("09-variable-collections.json")
    supplemental_ids = set()
    if (output / "11-state-group-dsl.json").exists():
        extra_groups = json.loads((output / "11-state-group-dsl.json").read_bytes())
        for record in extra_groups:
            group_dsl = decoded_tool_value(record["response"])
            if len(group_dsl.get("roots", [])) != 1 or group_dsl["roots"][0]["id"] != record["componentSetId"]:
                raise ValueError(f"State-group DSL root mismatch: {record['componentSetId']}")
            references = group_dsl.get("refsIndex", {}).get("componentSets", [])
            if len(references) != 1 or references[0]["id"] != record["componentSetId"]:
                raise ValueError(f"State-group component reference mismatch: {record['componentSetId']}")
            page_dsl["refsIndex"]["componentSets"].extend(references)
            supplemental_ids.add(record["componentSetId"])
    if not isinstance(groups, dict) or not isinstance(components, list):
        raise ValueError("Unexpected Pixso variable or component payload")
    page = {"provider": "pixso", "fileKey": FILE_KEY, "nodeId": "2:145", "dsl": page_dsl}
    variant = {"provider": "pixso", "fileKey": FILE_KEY, "nodeId": "3:5686", "dsl": variant_dsl}
    evidence = build_evidence(page, [variant])
    evidence["evidenceLevel"] = "fresh-Pixso-MCP-JSON-RPC-responses"
    for registered in evidence["componentRegistry"]:
        registered["definitionSource"] = ("11-state-group-dsl.json" if registered["componentSetId"]
                                          in supplemental_ids else "03-page-dsl.json")
        registered["nativeKeySource"] = "07-all-components.json"
    evidence["limitations"] = [
        "The page DSL is compact; ID-less expanded instance children use local structural locators.",
        "COMPONENT_SET root node IDs and componentSetRef IDs are preserved separately.",
        "get_all_components lists native component keys for variants; absent keys remain null.",
        "getLocalVariableCollectionsAsync describes local collections only; other available groups retain ID-only modes.",
    ]
    collection_by_id = {item["id"]: item for item in collections}
    mode_names = {mode["modeId"]: mode["name"] for collection in collections
                  for mode in collection.get("modes", [])}
    variables_by_id = {}
    group_summaries = []
    for group_id, items in groups.items():
        collection = collection_by_id.get(group_id)
        group_summaries.append({"variableSetId": group_id,
                                "name": collection.get("name") if collection else None,
                                "localCollection": collection is not None,
                                "variableCount": len(items),
                                "modes": collection.get("modes") if collection else None})
        for item in items:
            variable_id = item["id"]
            if variable_id in variables_by_id:
                raise ValueError(f"Variable ID occurs in multiple groups: {variable_id}")
            variables_by_id[variable_id] = (group_id, item)
    missing_variables = []
    for reference in evidence["variables"]:
        variable_id = reference["variableId"]
        found = variables_by_id.get(variable_id)
        if found is None:
            missing_variables.append(variable_id)
            continue
        group_id, item = found
        reference.update({"name": item.get("name"), "variableSetId": group_id,
                          "variableSetName": collection_by_id.get(group_id, {}).get("name"),
                          "resolvedType": item.get("resolvedType"), "nativeVariableKey": item.get("key"),
                          "definitionSource": "06-variables.json",
                          "modeNameSource": "09-variable-collections.json",
                          "resolvedValues": [
                              {"modeId": mode_id, "modeName": mode_names.get(mode_id), "payload": payload}
                              for mode_id, payload in item.get("modeValues", {}).items()
                          ], "valueEvidence": "Pixso get_variables"})

    def resolve_alias(variable_id, mode_name):
        chain = []
        visited = set()
        current_id = variable_id
        while True:
            if current_id in visited:
                return {"status": "alias-cycle", "finalValue": None, "aliasChain": chain}
            visited.add(current_id)
            found = variables_by_id.get(current_id)
            if found is None:
                return {"status": "alias-target-missing", "finalValue": None, "aliasChain": chain}
            group_id, item = found
            collection = collection_by_id.get(group_id)
            if collection is None:
                return {"status": "mode-label-unavailable", "finalValue": None, "aliasChain": chain}
            mode_id = next((mode["modeId"] for mode in collection.get("modes", [])
                            if mode["name"] == mode_name), None)
            if mode_id is None or mode_id not in item.get("modeValues", {}):
                return {"status": "profile-mode-missing", "finalValue": None, "aliasChain": chain}
            payload = item["modeValues"][mode_id]
            value = payload.get("value")
            chain.append({"variableId": current_id, "variableName": item.get("name"),
                          "variableSetId": group_id, "modeId": mode_id, "rawValue": value})
            if isinstance(value, dict) and isinstance(value.get("id"), str):
                current_id = value["id"]
                continue
            return {"status": "resolved", "finalValue": value, "aliasChain": chain}

    resolved_count = 0
    unresolved_count = 0
    for reference in evidence["variables"]:
        for mode in reference["resolvedValues"] or []:
            result = resolve_alias(reference["variableId"], mode["modeName"])
            mode.update(result)
            if result["status"] == "resolved":
                resolved_count += 1
            else:
                unresolved_count += 1
    evidence["variableGroups"] = sorted(group_summaries, key=lambda item: item["variableSetId"])
    evidence["localVariableCollections"] = collections
    evidence["missingReferencedVariableIds"] = missing_variables

    components_by_node = {item.get("node_id"): item for item in components if item.get("node_id")}
    variant_keys = []
    for registered in evidence["componentRegistry"]:
        keys_found = 0
        for variant_id in registered["variantNodeIds"]:
            component = components_by_node.get(variant_id)
            native_key = component.get("component_key") if component else None
            if component and component.get("file_key") != FILE_KEY:
                raise ValueError(f"Component key belongs to another file: {variant_id}")
            if native_key:
                keys_found += 1
            variant_keys.append({"componentSetId": registered["componentSetId"],
                                 "variantNodeId": variant_id, "nativeComponentKey": native_key})
        registered["variantsWithNativeKey"] = keys_found
    evidence["variantNativeKeys"] = variant_keys
    evidence["counts"].update({
        "availableVariableGroups": len(groups),
        "availableVariables": len(variables_by_id),
        "localVariableCollections": len(collections),
        "referencedVariablesWithValues": len(evidence["variables"]) - len(missing_variables),
        "resolvedProfileValues": resolved_count,
        "unresolvedProfileValues": unresolved_count,
        "variantsWithNativeKey": sum(item["variantsWithNativeKey"] for item in evidence["componentRegistry"]),
    })
    return evidence


def verify_live(output):
    manifest = json.loads((output / "manifest.json").read_bytes())
    for item in manifest["files"]:
        raw = (output / item["path"]).read_bytes()
        if hashlib.sha256(raw).hexdigest() != item["sha256"]:
            raise ValueError(f"Evidence hash mismatch: {item['path']}")
    evidence = json.loads((output / "design-evidence.json").read_bytes())
    counts = evidence["counts"]
    registry = evidence["componentRegistry"]
    if len(registry) != 11 or len({item["componentSetId"] for item in registry}) != 11:
        raise ValueError("Expected 11 distinct Pixso state groups")
    if sum(item["variantCount"] for item in registry) != counts["referencedVariants"]:
        raise ValueError("Variant count disagrees with the component registry")
    variant_keys = evidence["variantNativeKeys"]
    if (len(variant_keys) != counts["referencedVariants"] or
            len({item["variantNodeId"] for item in variant_keys}) != len(variant_keys) or
            any(not item["nativeComponentKey"] for item in variant_keys)):
        raise ValueError("Variant keys are missing or duplicated")
    variables = evidence["variables"]
    if (len(variables) != counts["variablesNamedInPageRefsIndex"] or
            evidence["missingReferencedVariableIds"] or
            any(mode["status"] != "resolved" for item in variables for mode in item["resolvedValues"])):
        raise ValueError("A page variable or profile value is unresolved")
    page_source = decoded_tool_value(json.loads((output / "03-page-dsl.json").read_bytes()))
    if counts["pageRoots"] != len(page_source["roots"]):
        raise ValueError("Page root count disagrees with Pixso response")
    return {"verifiedFiles": len(manifest["files"]), "componentSets": len(registry),
            "variantsWithNativeKey": len(variant_keys), "pageVariables": len(variables),
            "resolvedProfileValues": sum(len(item["resolvedValues"]) for item in variables)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", default=DEFAULT_ENDPOINT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--list", action="store_true", help="Probe current MCP capabilities without writing files")
    parser.add_argument("--snapshot", type=Path, help="Preserved page 2:145 snapshot")
    parser.add_argument("--variant", type=Path, action="append", default=[], help="Preserved variant snapshot")
    parser.add_argument("--fresh", action="store_true", help="Capture the active DesignTest file through Pixso MCP")
    parser.add_argument("--read-collections", action="store_true",
                        help="Add local variable collection names and modes to an existing fresh capture")
    parser.add_argument("--capture-state-groups", action="store_true",
                        help="Read true state-group IDs for components omitted by compact DSL refsIndex")
    parser.add_argument("--capture-state-group-dsl", action="store_true",
                        help="Read DSL for native state groups omitted by the page's compact refsIndex")
    parser.add_argument("--build-live", action="store_true",
                        help="Compile saved fresh MCP responses into a verifiable design index")
    parser.add_argument("--verify", action="store_true", help="Verify response hashes and cross references")
    parser.add_argument("--refresh-derived", action="store_true",
                        help="Rebuild generated indexes only when the source snapshots match")
    args = parser.parse_args()
    if args.verify:
        if args.output is None or not args.output.is_dir():
            parser.error("--verify requires an existing fresh capture directory")
        print(json.dumps(verify_live(args.output), ensure_ascii=False))
        return
    if args.build_live:
        if args.output is None or not args.output.is_dir():
            parser.error("--build-live requires an existing fresh capture directory")
        evidence = build_live_evidence(args.output)
        saved = save_json(args.output / "design-evidence.json", evidence)
        manifest_path = args.output / "manifest.json"
        manifest = json.loads(manifest_path.read_bytes())
        manifest["files"] = [item for item in manifest["files"] if item["path"] != saved["path"]]
        manifest["files"].append(saved)
        save_json(manifest_path, manifest)
        print(json.dumps(evidence["counts"], ensure_ascii=False))
        return
    client = McpClient(args.endpoint)
    initialization = client.initialize()
    try:
        listing = client.rpc("tools/list", {})
        tool_error = None
    except RuntimeError as error:
        listing = None
        tool_error = str(error)
    if args.list:
        print(json.dumps({"initialize": initialization, "toolsListError": tool_error}, ensure_ascii=False))
        for item in listing["result"]["tools"] if listing else []:
            print(json.dumps(item, ensure_ascii=False))
        return
    if args.output is None:
        parser.error("--output is required unless --list is used")
    output = args.output
    if args.capture_state_group_dsl:
        if not output.is_dir() or not (output / "10-state-group-variants.json").exists():
            parser.error("--capture-state-group-dsl requires the native state-group capture")
        if (output / "11-state-group-dsl.json").exists():
            parser.error("State-group DSL already captured")
        if listing is None or "get_node_dsl" not in {item["name"] for item in listing["result"]["tools"]}:
            parser.error(f"Pixso get_node_dsl is unavailable: {tool_error}")
        groups = json.loads((output / "10-state-group-variants.json").read_bytes())
        records = []
        for group in groups:
            arguments = {"guid": group["componentSetId"], "simplify": True}
            records.append({"componentSetId": group["componentSetId"],
                            "designName": group["designName"], "tool": "get_node_dsl",
                            "arguments": arguments, "response": client.tool("get_node_dsl", arguments)})
        saved = save_json(output / "11-state-group-dsl.json", records)
        manifest_path = output / "manifest.json"
        manifest = json.loads(manifest_path.read_bytes())
        manifest["files"].append(saved)
        manifest["calls"].extend({"tool": "get_node_dsl", "arguments": item["arguments"],
                                  "resultFile": saved["path"], "sha256": saved["sha256"]}
                                 for item in records)
        save_json(manifest_path, manifest)
        print(json.dumps({"stateGroups": [item["componentSetId"] for item in records],
                          "bytes": saved["bytes"]}, ensure_ascii=False))
        return
    if args.capture_state_groups:
        if not output.is_dir() or not (output / "07-all-components.json").exists():
            parser.error("--capture-state-groups requires an existing fresh capture")
        if (output / "10-state-group-variants.json").exists():
            parser.error("State groups already captured")
        if listing is None or "get_variants" not in {item["name"] for item in listing["result"]["tools"]}:
            parser.error(f"Pixso get_variants is unavailable: {tool_error}")
        components = decoded_tool_value(json.loads((output / "07-all-components.json").read_bytes()))
        wanted = {"ComponentLab/Icons", "ComponentLab/SelectOption", "ComponentLab/FieldChrome"}
        groups = {}
        for component in components:
            frame = component.get("containing_frame") or {}
            state_group = frame.get("containingStateGroup") or {}
            if state_group.get("name") in wanted:
                groups[state_group["nodeId"]] = state_group["name"]
        if set(groups.values()) != wanted:
            parser.error(f"Expected three native state groups, got {groups}")
        records = []
        for group_id, name in sorted(groups.items()):
            arguments = {"guid": group_id}
            records.append({"componentSetId": group_id, "designName": name, "tool": "get_variants",
                            "arguments": arguments, "response": client.tool("get_variants", arguments)})
        saved = save_json(output / "10-state-group-variants.json", records)
        manifest_path = output / "manifest.json"
        manifest = json.loads(manifest_path.read_bytes())
        manifest["files"].append(saved)
        manifest["calls"].extend({"tool": "get_variants", "arguments": item["arguments"],
                                  "resultFile": saved["path"], "sha256": saved["sha256"]}
                                 for item in records)
        save_json(manifest_path, manifest)
        print(json.dumps({"stateGroups": groups, "bytes": saved["bytes"]}, ensure_ascii=False))
        return
    if args.read_collections:
        if not output.is_dir() or not (output / "manifest.json").exists():
            parser.error("--read-collections requires an existing fresh capture directory")
        if (output / "09-variable-collections.json").exists():
            parser.error("Variable collections already captured in this directory")
        if listing is None or "eval_script" not in {item["name"] for item in listing["result"]["tools"]}:
            parser.error(f"Pixso eval_script is unavailable: {tool_error}")
        script = ("const c = await pixso.variables.getLocalVariableCollectionsAsync(); "
                  "return c.map(x => ({id:x.id,name:x.name,defaultModeId:x.defaultModeId,"
                  "modes:(x.modes || []).map(m => ({modeId:m.modeId,name:m.name}))}));")
        response = client.tool("eval_script", {"script": script})
        value = decoded_tool_value(response)
        saved = save_json(output / "09-variable-collections.json", response)
        manifest_path = output / "manifest.json"
        manifest = json.loads(manifest_path.read_bytes())
        manifest["files"].append(saved)
        manifest["calls"].append({"tool": "eval_script", "arguments": {"script": script},
                                  "resultFile": saved["path"], "sha256": saved["sha256"]})
        save_json(manifest_path, manifest)
        print(json.dumps({"collections": len(value), "file": str(output / saved["path"])}, ensure_ascii=False))
        return
    if args.fresh:
        if listing is None:
            parser.error(f"Pixso tools are unavailable: {tool_error}")
        available = {item["name"] for item in listing["result"]["tools"]}
        requested = ["eval_script", "get_node_dsl", "get_variable_sets", "get_variables",
                     "get_all_components", "get_local_styles"]
        missing = sorted(set(requested) - available)
        if missing:
            parser.error(f"Pixso MCP does not expose: {', '.join(missing)}")
        identity_response = client.tool("eval_script", {"script": "return {fileKey:pixso.fileKey};"})
        identity = decoded_tool_value(identity_response)
        if identity.get("fileKey") != FILE_KEY:
            parser.error(f"Active Pixso file is {identity.get('fileKey')}, expected {FILE_KEY}")
        if output.exists() and any(output.iterdir()):
            parser.error("Output directory is not empty; use a fresh location")
        output.mkdir(parents=True, exist_ok=True)
        manifest = {"capturedAtUtc": datetime.now(timezone.utc).isoformat(), "endpoint": args.endpoint,
                    "fileKey": FILE_KEY, "evidenceSource": "fresh-Pixso-MCP-tools-call",
                    "files": [], "calls": []}
        manifest["files"].append(save_json(output / "00-initialize.json", initialization))
        manifest["files"].append(save_json(output / "01-tools-list.json", listing))
        manifest["files"].append(save_json(output / "02-file-identity.json", identity_response))
        calls = [
            ("get_node_dsl", {"guid": "2:145", "simplify": True}, "03-page-dsl.json"),
            ("get_node_dsl", {"guid": "3:5686", "simplify": True}, "04-button-variant-dsl.json"),
            ("get_variable_sets", {}, "05-variable-sets.json"),
            ("get_variables", {}, "06-variables.json"),
            ("get_all_components", {}, "07-all-components.json"),
            ("get_local_styles", {}, "08-local-styles.json"),
        ]
        for name, parameters, filename in calls:
            try:
                response = client.tool(name, parameters)
                saved = save_json(output / filename, response)
                manifest["files"].append(saved)
                manifest["calls"].append({"tool": name, "arguments": parameters,
                                          "resultFile": filename, "sha256": saved["sha256"]})
                print(f"Captured {name}: {saved['bytes']} bytes")
            except Exception as error:
                manifest["calls"].append({"tool": name, "arguments": parameters, "error": str(error)})
                print(f"Failed {name}: {error}")
        save_json(output / "manifest.json", manifest)
        return
    if args.snapshot is None:
        parser.error("--snapshot is required unless --fresh is used")
    if output.exists() and any(output.iterdir()) and not args.refresh_derived:
        parser.error("Output directory is not empty; use a fresh location")
    output.mkdir(parents=True, exist_ok=True)
    if args.refresh_derived:
        sources = [(args.snapshot, output / "page-2-145.source.json")]
        sources.extend((path, output / f"variant-{index}.source.json")
                       for index, path in enumerate(args.variant, start=1))
        for source, target in sources:
            if not target.exists() or target.read_bytes() != source.read_bytes():
                parser.error(f"Cannot refresh: source snapshot differs from {target}")
    manifest = {"assembledAtUtc": datetime.now(timezone.utc).isoformat(),
                "fileKeyExpected": FILE_KEY, "files": [], "evidenceSource": "preserved-local-snapshot"}
    manifest["files"].append(copy_source(args.snapshot, output / "page-2-145.source.json"))
    for index, path in enumerate(args.variant, start=1):
        manifest["files"].append(copy_source(path, output / f"variant-{index}.source.json"))
    page = json.loads(args.snapshot.read_bytes())
    variants = [json.loads(path.read_bytes()) for path in args.variant]
    evidence = build_evidence(page, variants)
    manifest["files"].append(save_json(output / "design-evidence.json", evidence))
    probe = {"probedAtUtc": datetime.now(timezone.utc).isoformat(), "endpoint": args.endpoint,
             "initialize": initialization, "toolsList": listing, "toolsListError": tool_error}
    manifest["files"].append(save_json(output / "mcp-probe.json", probe))
    save_json(output / "manifest.json", manifest)
    print(json.dumps({"output": str(output), "counts": evidence["counts"],
                      "mcpToolsAvailable": listing is not None}, ensure_ascii=False))


if __name__ == "__main__":
    main()
