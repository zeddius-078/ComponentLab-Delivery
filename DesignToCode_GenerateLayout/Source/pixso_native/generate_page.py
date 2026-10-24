"""Stage a static Qt page from the independent native-export path.

Successful staging is not six-gate acceptance. No SDK is rebuilt or installed.
All parsing and lowering complete in memory before the page directory is created.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from assets import ResourceBundle, bind_collections, bind_icons
from acquire import checked_work_path
from component_binding import ComponentBinding, bind_text_slots, bind_text_lists, bind_table, index_instances
from component_engine import ComponentError, read_mapping
from page_output import render_page, write_page
from styled_layout import select_root, lower_tree
from generation_policy import export_first


def attach_dsl_names(graph, dsl_path, expected_node_id):
    """Attach same-batch Pixso names to source nodes without changing parsing."""
    if dsl_path is None:
        raise ValueError("DSL_REQUIRED_FOR_SECOND_ROUTE")
    payload = json.loads(dsl_path.read_text(encoding="utf-8-sig"))
    if isinstance(payload, dict) and payload.get("content"):
        text = next((item.get("text") for item in payload["content"]
                     if item.get("type") == "text"), None)
        if text:
            payload = json.loads(text)
    index = {}

    def visit(node):
        if isinstance(node, dict):
            guid = node.get("guid")
            name = node.get("name")
            if guid and name:
                index[str(guid).replace(":", "_")] = str(name)
            for key in ("childNode", "children", "pixTreeNodes"):
                value = node.get(key, [])
                if isinstance(value, list):
                    for child in value:
                        visit(child)
    visit(payload)
    normalized_expected = str(expected_node_id).replace(":", "_")
    if normalized_expected not in index:
        raise ValueError("DSL_ROOT_NODE_MISMATCH: " + str(expected_node_id))
    attached = 0

    def annotate(node):
        nonlocal attached
        if not isinstance(node, dict):
            return
        source = node.get("source", {})
        design_id = source.get("designNodeId") or node.get("attrs", {}).get("id")
        if design_id:
            name = index.get(str(design_id).replace(":", "_"))
            if name:
                node["designName"] = name
                attached += 1
        else:
            # Generated frame wrappers carry no id; the class token still names
            # the owning DSL frame, so derive a readable container name.
            for token in (node.get("attrs", {}).get("class") or "").split():
                match = re.fullmatch(r"frame-content-(\d+_\d+)", token)
                if match:
                    owner = index.get(match.group(1))
                    if owner:
                        node["designName"] = owner + " Content"
                        attached += 1
                    break
        for child in node.get("children", []):
            annotate(child)
    annotate(graph.get("styledTree", {}))
    if attached == 0:
        raise ValueError("DSL_NO_SOURCE_NODES_ASSOCIATED")
    return {"status": "ATTACHED", "dsl": str(dsl_path),
            "indexedNodes": len(index), "attachedNodes": attached}


def stage_with_record(destination, files, evidence, metadata):
    """Keep the generated page pure; record this attempt beside it, not inside it.

    Exclusive record creation prevents accidental reuse of a batch. An interrupted
    write leaves RUNNING or an invalid record, neither of which is acceptance.
    """
    destination = checked_work_path(destination)
    record_path = destination.with_name(destination.name + ".generation.json")
    checked_work_path(record_path)
    if destination.exists():
        raise ValueError("OUTPUT_ALREADY_EXISTS")
    if record_path.exists():
        raise ValueError("GENERATION_RECORD_ALREADY_EXISTS")
    record = {"schemaVersion": 1, "status": "RUNNING", "acceptance": "not-run",
              "pageDirectory": destination.name, "metadata": metadata,
              "evidence": evidence,
              "files": [{"path": name, "bytes": len(data),
                         "sha256": hashlib.sha256(data).hexdigest()}
                        for name, data in sorted(files.items())]}
    encode = lambda: (json.dumps(record, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    # Serialize before creating anything, including the RUNNING record.
    initial = encode()
    record_path.parent.mkdir(parents=True, exist_ok=True)
    with record_path.open("xb") as handle:
        handle.write(initial)
    try:
        output = write_page(destination, files)
        for item in record["files"]:
            if hashlib.sha256((output / item["path"]).read_bytes()).hexdigest() != item["sha256"]:
                raise RuntimeError("STAGED_FILE_HASH_MISMATCH")
    except (ValueError, RuntimeError, OSError) as error:
        record.update(status="FAILED", error=str(error))
        record_path.write_bytes(encode())
        raise
    record["status"] = "STAGED"
    record_path.write_bytes(encode())
    return output, record_path


def convert_graph(graph, mapping, source_file_key, sdk_release, profile,
                  input_root, page_name, instance, node_id, viewport):
    binding = ComponentBinding(mapping, graph["componentTree"], source_file_key)
    permissive = export_first(mapping)
    warnings = []
    component_bindings = []
    root = select_root(graph["styledTree"], instance, node_id)
    bundle = ResourceBundle(input_root, page_name)
    records = {}
    for record in graph.get("componentEvidence", []):
        scope = record["source"]["instancePath"]
        if scope in records:
            raise ComponentError("CONTENT_SCOPE_NOT_UNIQUE")
        records[scope] = record

    def component(node, name):
        boundaries = node["componentBoundaries"]
        if len(boundaries) != 1:
            raise ComponentError("NESTED_PUBLIC_BOUNDARY_CONTRACT_REQUIRED")
        boundary = boundaries[0]
        inspected = binding.inspect(boundary)
        if inspected["issues"]:
            raise ComponentError("COMPONENT_BINDING_FAILED: " + boundary.get("tag", "?") +
                                 " @ " + boundary["instancePath"] + ": " +
                                 ", ".join(inspected["issues"]))
        rules = [rule for rule in mapping["components"]
                 if rule.get("mappingKey") == inspected["mappingKey"]]
        if len(rules) != 1:
            raise ComponentError("MAPPING_KEY_NOT_UNIQUE")
        rule = rules[0]
        if inspected.get("warnings"):
            warnings.append({"source": boundary, "kind": "COMPONENT_MAPPING_DIFFERENCE",
                             "details": inspected})
        record = records.get(boundary["instancePath"])
        if record is None or (record.get("conflicts") and not permissive):
            raise ComponentError("COMPONENT_CONTENT_UNRESOLVED")
        if record.get("conflicts"):
            warnings.append({"source": boundary, "kind": "SOURCE_CONTENT_CONFLICT",
                             "details": record["conflicts"], "props": inspected["props"]})
        # SDK-owned appearance must be explicitly accounted for by the mapping.
        # No wildcard CSS suppression and no page-QSS override of SDK controls.
        values = node.get("style", {}).get("values", {})
        placement_keys = {"width", "height", "box-sizing", "flex-shrink", "flex-grow",
                          "flex-basis"}
        placement = {key: value for key, value in values.items() if key in placement_keys}
        appearance = {key: value for key, value in values.items() if key not in placement_keys}
        if "styleContract" not in rule or rule["styleContract"] != appearance:
            if not permissive:
                raise ComponentError("SDK_BOUNDARY_STYLE_CONTRACT_REQUIRED")
            warnings.append({"source": boundary, "kind": "SDK_APPEARANCE_USED",
                             "unappliedSourceStyle": appearance})
        definitions = rule.get("collections", {})
        groups = record.get("collections", {})
        slots = bind_collections(groups, definitions, bundle) if definitions else {}
        # Until single-text/icon-slot lowering is provided, never drop content
        # merely because a component constructor has a convenient default.
        text_slots, consumed_text = bind_text_slots(record, rule.get("textBindings", {}))
        if set(text_slots) & set(slots):
            raise ComponentError("CONTENT_SLOT_CONSUMED_TWICE")
        slots.update(text_slots)
        list_slots, list_text = bind_text_lists(record, rule.get("textListBindings", {}))
        if set(list_slots) & set(slots) or any(text in consumed_text for text in list_text):
            raise ComponentError("CONTENT_SLOT_CONSUMED_TWICE")
        slots.update(list_slots)
        consumed_text.extend(list_text)
        if rule.get("tableBinding"):
            table, table_text = bind_table(record, rule["tableBinding"])
            target = rule["tableBinding"]["output"]
            if target in slots:
                raise ComponentError("CONTENT_SLOT_CONSUMED_TWICE")
            slots[target] = table
            consumed_text.extend(table_text)
        icon_slots, consumed_images, icon_fallbacks = bind_icons(record, rule.get("iconBindings", {}), bundle)
        if set(icon_slots) & set(slots):
            raise ComponentError("CONTENT_SLOT_CONSUMED_TWICE")
        slots.update(icon_slots)
        for fallback in icon_fallbacks:
            warnings.append({"source": boundary, "objectName": name,
                             "kind": "SYSTEM_ICON_FALLBACK", "details": fallback})
        for group in groups.values():
            for item in group["items"]:
                consumed_text.append(item["text"])
                consumed_images.extend(item["images"])
        canonical = lambda values: sorted(json.dumps(value, sort_keys=True) for value in values)
        if (canonical(consumed_text) != canonical(record["content"]["text"])
                or canonical(consumed_images) != canonical(record["content"]["images"])):
            if not permissive:
                raise ComponentError("UNCONSUMED_COMPONENT_CONTENT")
            warnings.append({"source": boundary, "kind": "UNCONSUMED_COMPONENT_CONTENT",
                             "sourceContent": record["content"],
                             "consumedText": consumed_text, "consumedImages": consumed_images})
        bound = binding.bind(boundary, slots)
        bound.update({"kind": "component", "name": name, "css": placement})
        component_bindings.append({"objectName": name, "mappingKey": rule["mappingKey"],
                                   "qtClass": rule["qt"]["class"], "source": boundary,
                                   "declaredProps": inspected["props"], "boundSlots": slots,
                                   "unappliedProperties": inspected.get("unappliedProperties", [])})
        return bound

    adapted = lower_tree(root, component_adapter=component, viewport=viewport)
    for declaration in adapted.get("layoutDeclarations", []):
        if declaration.get("disposition") == "no-op":
            warnings.append({"source": adapted["trace"][declaration["objectName"]],
                             "objectName": declaration["objectName"],
                             "kind": "PAGE_LAYOUT_NOOP", "details": declaration})
    if permissive:
        for name, style in adapted["pageStyles"].items():
            unapplied = {key: value for key, value in style.items()
                         if key != "background-color"}
            if unapplied:
                warnings.append({"source": adapted["trace"][name],
                                 "objectName": name, "kind": "PAGE_VISUAL_DIFFERENCE",
                                 "unappliedSourceStyle": unapplied})
            adapted["pageStyles"][name] = {
                key: value for key, value in style.items() if key == "background-color"}
    files = render_page(page_name, adapted["root"], mapping, sdk_release, profile,
                        assets=bundle, page_styles=adapted["pageStyles"])
    return files, {"trace": adapted["trace"], "resources": bundle.evidence,
                   "generationPolicy": mapping.get("generationPolicy", "strict"),
                   "warnings": warnings,
                   "layoutDeclarations": adapted.get("layoutDeclarations", []),
                   "componentBindings": component_bindings,
                   "acceptance": "not-run; staging only"}


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("node", "modules", "input", "output", "mapping", "sdk", "sdk-verifier"):
        parser.add_argument("--" + name, type=Path, required=True)
    for name in ("entry", "main", "modes", "instance", "node-id", "page-name", "profile",
                 "source-file-key", "viewport-source"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--dsl", type=Path, required=True,
                        help="same-batch Pixso get_node_dsl JSON; required for naming")
    parser.add_argument("--viewport", type=int, nargs=2, required=True)
    args = parser.parse_args()
    for name in ("node", "modules", "input", "output", "mapping", "sdk", "sdk_verifier"):
        if not getattr(args, name).is_absolute():
            raise ValueError("ABSOLUTE_PATH_REQUIRED: " + name)
    if args.output.exists():
        raise ValueError("OUTPUT_ALREADY_EXISTS")
    mapping_bytes = args.mapping.read_bytes()
    mapping = read_mapping(args.mapping)
    # Reuse the workspace's one SDK verifier, selected explicitly. It is not a
    # consumer's private implementation and does not build the SDK.
    checked = subprocess.run([sys.executable, "-B", str(args.sdk_verifier), "verify",
                              "--sdk", str(args.sdk), "--configuration", "Release"],
                             capture_output=True, timeout=60)
    if checked.returncode:
        raise RuntimeError("SDK_VERIFICATION_FAILED: " +
                           checked.stderr.decode("utf-8", errors="replace"))
    manifest_path = args.sdk / "manifest/component-sdk.json"
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes.decode("utf-8-sig"))
    if args.profile not in manifest.get("profiles", []):
        raise ValueError("SDK_PROFILE_UNAVAILABLE")
    parsed = subprocess.run([
        str(args.node), str(Path(__file__).with_name("style_graph.cjs")), str(args.input),
        args.entry, str(args.modules), args.main, args.modes, str(args.mapping),
    ], capture_output=True, timeout=60)
    if parsed.returncode:
        raise RuntimeError("STATIC_PARSE_FAILED: " + parsed.stderr.decode("utf-8", errors="replace"))
    graph = json.loads(parsed.stdout)
    if not args.dsl.is_absolute():
        raise ValueError("ABSOLUTE_PATH_REQUIRED: dsl")
    if not args.dsl.is_file():
        raise ValueError("DSL_FILE_NOT_FOUND")
    dsl_evidence = attach_dsl_names(graph, args.dsl, args.node_id)
    files, evidence = convert_graph(
        graph, mapping, args.source_file_key, manifest["releaseId"], args.profile,
        args.input, args.page_name, args.instance, args.node_id,
        {"width": args.viewport[0], "height": args.viewport[1], "source": args.viewport_source})
    if args.mapping.read_bytes() != mapping_bytes or manifest_path.read_bytes() != manifest_bytes:
        raise RuntimeError("GENERATION_INPUT_CHANGED")
    metadata = {
        "sourceFileKey": args.source_file_key,
        "sourceRelationship": "caller-supplied; same-batch not established",
        "entry": args.entry, "instance": args.instance, "nodeId": args.node_id,
        "viewport": {"width": args.viewport[0], "height": args.viewport[1],
                     "source": args.viewport_source},
        "mapping": {"id": mapping.get("id"), "version": mapping.get("version"),
                    "sha256": hashlib.sha256(mapping_bytes).hexdigest()},
        "sdk": {"releaseId": manifest["releaseId"], "profile": args.profile,
                "manifestSha256": hashlib.sha256(manifest_bytes).hexdigest()},
        "pythonVersion": ".".join(map(str, sys.version_info[:3])),
        "parsedModules": [item["source"] for _, item in
                          sorted(index_instances(graph["componentTree"]).items())],
        "modeEvidence": graph.get("modeEvidence"),
        "dslNameEvidence": dsl_evidence,
        "undeclaredTemplateIdentifiers": graph.get("undeclaredTemplateIdentifiers", []),
        "browserMeasured": graph.get("browserMeasured", False),
        "sourceHashScope": "parsed SFC modules and collected resources; not a full input archive hash",
    }
    output, record_path = stage_with_record(args.output, files, evidence, metadata)
    print(json.dumps({"page": str(output), "files": sorted(files),
                      "generationRecord": str(record_path),
                      "sourceFileKey": args.source_file_key,
                      "sourceRelationship": "caller-supplied; same-batch not established",
                      "acceptance": evidence["acceptance"]}, ensure_ascii=True))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(json.dumps({"status": "failed", "error": str(error),
                          "acceptance": "not-run"}, ensure_ascii=True), file=sys.stderr)
        sys.exit(2)
