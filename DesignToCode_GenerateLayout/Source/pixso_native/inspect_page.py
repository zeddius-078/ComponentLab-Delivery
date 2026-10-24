"""Inspect an exported Vue page using explicit installed tools, without installs."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

from acquire import new_output
from assets import ResourceBundle, bind_collections
from component_binding import inspect_boundaries, bind_text_slots, bind_table
from component_engine import read_mapping
from styled_layout import inspect_tree, select_root


def blocking_summary(report, mapping=None):
    """List observed blockers, not a substitute for generation or acceptance.

    Inspection deliberately continues across partial mappings. Even an empty
    list means only that these checks found nothing; it cannot certify slots,
    SDK API behavior, source provenance, or visual equivalence.
    """
    findings = []

    def add(stage, source, code, detail=None):
        item = {"stage": stage, "source": source, "code": code}
        if detail is not None:
            item["detail"] = detail
        findings.append(item)

    for issue in report.get("layoutIssues", []):
        add("layout", issue.get("source"), issue["error"])
    bindings = {item["source"]["instancePath"]: item
                for item in report.get("componentBindings", [])}
    rules = {rule["mappingKey"]: rule for rule in (mapping or {}).get("components", [])}
    placement = {"width", "height", "box-sizing", "flex-shrink", "flex-grow"}
    for item in report.get("componentBoundaries", []):
        if len(item["boundaries"]) != 1:
            add("identity", item.get("source"), "NESTED_PUBLIC_BOUNDARY_CONTRACT_REQUIRED")
        for source in item["boundaries"]:
            bound = bindings.get(source["instancePath"])
            if bound is None:
                add("identity", source, "COMPONENT_MAPPING_NOT_INSPECTED")
                continue
            for code in bound["issues"]:
                detail_key = {"UNMAPPED_COMPONENT_PROPS": "unmappedProps",
                              "MISSING_COMPONENT_PROP": "missingProps",
                              "UNSUPPORTED_PROP_VALUE": "unsupportedPropValues"}.get(code)
                add("component", source, code, bound.get(detail_key) if detail_key else None)
            rule = rules.get(bound.get("mappingKey"))
            if rule is not None:
                appearance = {key: value for key, value in item.get("style", {}).items()
                              if key not in placement}
                if "styleContract" not in rule or rule["styleContract"] != appearance:
                    add("style", source, "SDK_BOUNDARY_STYLE_CONTRACT_REQUIRED",
                        {"observed": appearance, "declared": rule.get("styleContract")})
    for item in report.get("componentContent", []):
        for conflict in item.get("conflicts", []):
            add("content", item["source"], "COMPONENT_CONTENT_UNRESOLVED", conflict)
    counts = {}
    for item in findings:
        counts[item["code"]] = counts.get(item["code"], 0) + 1
    return {"status": "blocked" if findings else "no-observed-blockers",
            "generationReady": False, "acceptance": "not-run",
            "scope": "static inspection only; not exhaustive generation validation",
            "counts": counts, "findings": findings}


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("node", "modules", "input", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    for name in ("entry", "main", "modes", "instance", "node-id"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--viewport", type=int, nargs=2, metavar=("WIDTH", "HEIGHT"))
    parser.add_argument("--viewport-source")
    parser.add_argument("--mapping", type=Path)
    parser.add_argument("--source-file-key")
    args = parser.parse_args()
    for path in (args.node, args.modules, args.input, args.output):
        if not path.is_absolute():
            raise ValueError("ABSOLUTE_PATH_REQUIRED")
    script = Path(__file__).with_name("style_graph.cjs")
    command = [
        str(args.node), str(script), str(args.input), args.entry,
        str(args.modules), args.main, args.modes,
    ]
    mapping = None
    if args.mapping:
        if not args.mapping.is_absolute():
            raise ValueError("ABSOLUTE_MAPPING_PATH_REQUIRED")
        mapping = read_mapping(args.mapping)
        command.append(str(args.mapping))
    result = subprocess.run(command, capture_output=True, timeout=60)
    if result.returncode:
        raise RuntimeError("STATIC_PARSE_FAILED: " + result.stderr.decode("utf-8", errors="replace"))
    graph = json.loads(result.stdout)
    selected = select_root(graph["styledTree"], args.instance, args.node_id)
    if bool(args.viewport) != bool(args.viewport_source):
        raise ValueError("VIEWPORT_AND_SOURCE_MUST_BE_PROVIDED_TOGETHER")
    viewport = ({"width": args.viewport[0], "height": args.viewport[1],
                 "source": args.viewport_source} if args.viewport else None)
    report = inspect_tree(selected, viewport=viewport)
    report["viewport"] = viewport
    report["modeEvidence"] = graph["modeEvidence"]
    report["browserMeasured"] = graph["browserMeasured"]
    report["input"] = {"root": str(args.input), "entry": args.entry,
                       "instance": args.instance, "nodeId": args.node_id}
    if args.mapping:
        report["componentBindings"] = inspect_boundaries(
            graph["componentTree"], report["componentBoundaries"],
            mapping, args.source_file_key)
        report["mapping"] = str(args.mapping)
        scopes = {binding["source"]["instancePath"] for binding in report["componentBindings"]}
        report["componentContent"] = [
            item for item in graph.get("componentEvidence", [])
            if item["source"]["instancePath"] in scopes]
        bundle = ResourceBundle(args.input, "InspectionOnly")
        for item in report["componentContent"]:
            if item["conflicts"]:
                continue
            candidates = [rule for rule in mapping["components"]
                          if rule["source"]["import"] == item["source"]["import"]
                          and rule["source"]["tag"] == item["source"]["tag"]]
            if len(candidates) == 1:
                if candidates[0].get("tableBinding"):
                    table, consumed = bind_table(item, candidates[0]["tableBinding"])
                    item["resolvedTableData"] = table
                    item["consumedTableText"] = consumed
                if candidates[0].get("collections"):
                    item["resolvedCollectionSlots"] = bind_collections(
                        item["collections"], candidates[0]["collections"], bundle)
                if candidates[0].get("textBindings"):
                    values, consumed = bind_text_slots(item, candidates[0]["textBindings"])
                    item["resolvedTextSlots"] = values
                    item["consumedTextEvidence"] = consumed
        report["collectionResources"] = {
            "status": "validated-in-memory-only; no QRC or page written",
            "assets": bundle.evidence}
    elif args.source_file_key:
        raise ValueError("SOURCE_FILE_KEY_REQUIRES_MAPPING")
    report["blockingSummary"] = blocking_summary(report, mapping)
    output = new_output(args.output)
    (output / "inspection.json").write_bytes(
        (json.dumps(report, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    binding_issues = {}
    for binding in report.get("componentBindings", []):
        for issue in binding["issues"]:
            binding_issues[issue] = binding_issues.get(issue, 0) + 1
    print(json.dumps({"report": str(output / "inspection.json"),
                      "containers": report["containers"],
                      "layoutIssues": len(report["layoutIssues"]),
                      "componentBoundaries": len(report["componentBoundaries"]),
                      "componentBindingIssues": binding_issues,
                      "blockingCounts": report["blockingSummary"]["counts"],
                      "pageGeneration": "not-run"}))
    return 2 if report["layoutIssues"] or report["componentBoundaries"] else 0


if __name__ == "__main__":
    sys.exit(main())
