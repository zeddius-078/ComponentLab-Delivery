"""Carry exact input bytes and an explicit hash-bound review into a Qt contract."""
import base64
import hashlib
import json


def source(raw, kind, document_url, node_id, reviewed_sha256=None):
    digest = hashlib.sha256(raw).hexdigest()
    if reviewed_sha256 is not None and reviewed_sha256.lower() != digest:
        raise ValueError("Review hash does not match the source snapshot")
    return {"kind": kind, "documentUrl": document_url, "nodeId": node_id,
            "rawSha256": digest, "snapshotBase64": base64.b64encode(raw).decode("ascii"),
            "reviewed": reviewed_sha256 is not None,
            "reviewedSha256": digest if reviewed_sha256 is not None else ""}


def design_tree(raw):
    """Keep design identity and bindings separate from the platform-specific contract."""
    data = json.loads(raw)
    provider = data["provider"]
    if provider != "figma":
        raise ValueError("Pixso consumers require canonical/index.json")
    roots = [data["root"]]
    nodes = []
    seen = set()

    def walk(node, parent_id):
        node_id = node["id"]
        if node_id in seen:
            raise ValueError("Duplicate design node id")
        seen.add(node_id)
        bindings = dict(node.get("boundVariables", {}))

        def aliases(value, path):
            if isinstance(value, str) and value.startswith("variable:"):
                bindings[path] = value
            elif isinstance(value, dict):
                for key, item in value.items():
                    if key != "children":
                        aliases(item, f"{path}.{key}" if path else key)
            elif isinstance(value, list):
                for i, item in enumerate(value):
                    aliases(item, f"{path}[{i}]")

        aliases(node, "")
        children = node.get("children", [])
        layout = {
            key: node[key] for key in ("layoutMode", "itemSpacing", "paddingLeft",
                                      "paddingTop", "paddingRight", "paddingBottom") if key in node}
        box = node.get("absoluteBoundingBox", node.get("box", {}))
        bounds = {"left": box.get("x"), "top": box.get("y"),
                  "width": box.get("width", box.get("w")), "height": box.get("height", box.get("h"))}
        reference = None
        if node.get("mainComponent") or node.get("componentSetRef"):
            main = node.get("mainComponent", {})
            reference = {"nodeId": main.get("id", node_id), "setId": node.get("componentSetRef"),
                         "key": main.get("key", node.get("componentKey")), "fileKey": data["fileKey"]}
        nodes.append({"nodeId": node_id, "name": node.get("name", ""), "type": node["type"],
                      "parentId": parent_id, "children": [c["id"] for c in children],
                      "componentRef": reference, "bounds": bounds,
                      "layout": layout, "tokenBindings": bindings})
        for child in children:
            walk(child, node_id)

    for root in roots:
        walk(root, None)
    return {"schemaVersion": 1, "provider": provider, "fileKey": data["fileKey"],
            "boundsSpace": "canvas",
            "sourceSha256": hashlib.sha256(raw).hexdigest(),
            "rootIds": [r["id"] for r in roots], "nodes": nodes}
