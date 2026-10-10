"""Map the registered Button variant from canonical/index.json into the Qt contract."""
import argparse
import base64
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "DesignEvidence" / "tools"))
from contract_io import encoded, digest, load, write, read_artifacts

def evidence(index_path, rule):
    index_path = Path(index_path)
    index, docs = read_artifacts(index_path, ("components", "component-variants", "page-structure", "variables-used", "provenance"))
    def require(condition, message):
        if not condition:
            raise ValueError(message)
    require(index["provider"] == "pixso" and index["fileKey"] == rule["fileKey"], "Unregistered design file")
    variant = next((v for v in docs["component-variants"]["variants"] if v["variantNodeId"] == rule["variantId"]), None)
    require(variant is not None and variant["componentSetId"] == rule["componentSetId"], "Unregistered variant family")
    component = next((c for c in docs["components"]["components"] if c["componentSetId"] == rule["componentSetId"]), None)
    require(component is not None and variant["variantNodeId"] in component["variantNodeIds"], "Component contract mismatch")
    props = variant["properties"]
    attrs = {p["name"]:p["value"] for p in props}
    require(len(attrs) == len(props) and attrs == rule["expected"], "Unsupported property combination; review rule first")
    structure = docs["page-structure"]
    node = next((n for n in structure["variantExamples"] if n["nodeId"] == rule["variantId"]), None)
    require(node is not None and node["type"] == "VARIANT" and node.get("componentSetRef") == rule["componentSetId"], "Variant example missing")
    require({p["name"]:p["value"] for p in node["variantProperties"]} == attrs, "Variant example property mismatch")
    by_locator = {n["locator"]:n for n in structure["variantExamples"]}
    children = [by_locator[c] for c in node["children"]]
    require(len(children) == 1 and children[0]["type"] == "TEXT" and children[0].get("name") == rule["labelLayer"], "Unexpected content or icon")
    label = children[0].get("text",{}).get("content")
    require(isinstance(label,str) and 0 < len(label) <= 256, "Invalid button label")
    locators = {node["locator"],children[0]["locator"]}
    bindings = [b for b in docs["variables-used"]["bindings"] if b["locator"] in locators]
    variable_ids = {b["variableId"] for b in bindings}
    variables = [v for v in docs["variables-used"]["variables"] if v["variableId"] in variable_ids]
    prefixes = {
        "components": ["/components/" + str(i) for i,c in enumerate(docs["components"]["components"]) if c["componentSetId"] == rule["componentSetId"]],
        "component-variants": ["/variants/" + str(i) for i,v in enumerate(docs["component-variants"]["variants"]) if v["variantNodeId"] == rule["variantId"]],
        "page-structure": ["/variantExamples/" + str(i) for i,n in enumerate(structure["variantExamples"]) if n["locator"] in locators],
        "variables-used": ["/variables/" + str(i) for i,v in enumerate(docs["variables-used"]["variables"]) if v["variableId"] in variable_ids] +
                          ["/bindings/" + str(i) for i,b in enumerate(docs["variables-used"]["bindings"]) if b["locator"] in locators]}
    sources = [m for m in docs["provenance"]["mappings"] if any(
        m["pointer"] == prefix or m["pointer"].startswith(prefix + "/")
        for prefix in prefixes.get(m["artifact"].removesuffix(".json"), []))]
    call_ids = {s["callId"] for mapping in sources for s in mapping["sources"]}
    proof = {"schemaVersion":1,"provider":"pixso","fileKey":index["fileKey"],"pageId":index["pageId"],
             "captureId":index["captureId"],"nodeId":variant["variantNodeId"],
             "canonicalIndexSha256":digest(index_path.read_bytes()),
             "indexBase64":base64.b64encode(index_path.read_bytes()).decode("ascii"),
             "component":component,"variant":variant,"structure":[node]+children,"bindings":bindings,"variables":variables,
             "provenance":{"mappings":sources,"calls":[c for c in docs["provenance"]["calls"] if c["callId"] in call_ids]}}
    return proof, label

def convert(index_path, rule, reviewed_sha256=None):
    proof,label = evidence(index_path, rule)
    raw = encoded(proof)
    sha = digest(raw)
    if reviewed_sha256 is not None and reviewed_sha256.lower() != sha:
        raise ValueError("Review hash does not match Canonical evidence snapshot")
    variant = proof["variant"]
    return {"schemaVersion":1,"mappingId":rule["mappingId"],
            "source":{"kind":"pixsoCanonical","documentUrl":f'https://pixso.cn/app/design/{rule["fileKey"]}?item-id={rule["variantId"]}',
                      "nodeId":rule["variantId"],"snapshotSha256":sha,"snapshotBase64":base64.b64encode(raw).decode("ascii"),
                      "canonicalIndexSha256":proof["canonicalIndexSha256"],"reviewed":reviewed_sha256 is not None,
                      "reviewedSha256":sha if reviewed_sha256 is not None else ""},
            "page":{"id":"PixsoButtonExample","component":"Column",
                    "layout":{"spacing":"base.stoken.metric.space.vertical.standard","margin":"base.stoken.layout.padding.inline.compact"},
                    "children":[{"id":"pixsoProcessButton","component":rule["component"],
                                 "properties":{"text":label,"type":"primary","controlSize":"md","appearance":"solid",
                                               "danger":False,"ghost":False,"loading":False,"disabled":False},
                                 "tokenRefs":["component.basic.button.size.md.height"],
                                 "design":{"variantNodeId":variant["variantNodeId"],"componentSetId":variant["componentSetId"],
                                           "nativeComponentKey":variant["nativeComponentKey"],
                                           "mappingKey":rule["fileKey"]+":"+rule["componentSetId"],
                                           "libraryVersion":"pixso-canonical-v1","alias":"Button"}}]}}

def design_tree(index_path, rule):
    proof,_ = evidence(index_path,rule)
    return {"schemaVersion":1,"provider":"pixso","fileKey":proof["fileKey"],"boundsSpace":"parent-local",
            "canonicalIndexSha256":proof["canonicalIndexSha256"],
            "rootLocators":[proof["structure"][0]["locator"]],"nodes":proof["structure"],
            "tokenBindings":proof["bindings"],"variables":proof["variables"],"provenance":proof["provenance"]}

if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("index",type=Path)
    parser.add_argument("rule",type=Path)
    parser.add_argument("output",type=Path)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--reviewed-sha256",help="SHA-256 of the inspected compact Canonical evidence snapshot")
    group.add_argument("--inspect",action="store_true",help="Write review snapshot only; does not mark it reviewed")
    args=parser.parse_args()
    rule=load(args.rule)
    if args.output.exists():
        parser.error("Output exists; choose a new staging path")
    if args.inspect:
        proof,_=evidence(args.index,rule)
        print(json.dumps(write(args.output,proof)))
    else:
        tree_path=args.output.with_suffix(".design-tree.json")
        if tree_path.exists():
            parser.error("Design tree output exists")
        result=convert(args.index,rule,args.reviewed_sha256)
        write(args.output,result)
        write(tree_path,design_tree(args.index,rule))
        print(args.output)
