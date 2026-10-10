"""Small shared I/O primitives; no collection or platform mapping logic."""
import hashlib
import json
import math
import re
from pathlib import Path, PurePosixPath

VERSION = 1
ROOT = Path(__file__).resolve().parents[1]

def encoded(value):
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False) + "\n").encode("utf-8")

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def value_hash(value):
    return digest(json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8"))

def load(path):
    return json.loads(Path(path).read_bytes())

def write(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(encoded(value))
    return {"path": path.name, "bytes": path.stat().st_size, "sha256": digest(path.read_bytes())}

def safe_path(base, relative):
    if not isinstance(relative, str) or not relative or "\\" in relative or ":" in relative:
        raise ValueError("Invalid relative artifact path")
    rel = PurePosixPath(relative)
    if rel.is_absolute() or any(p in ("..", ".") for p in relative.split("/")):
        raise ValueError("Path traversal rejected")
    base = Path(base).resolve()
    result = (base / relative).resolve()
    if not result.is_relative_to(base):
        raise ValueError("Path traversal rejected")
    return result

def escape(value):
    return str(value).replace("~", "~0").replace("/", "~1")

def pointer(value, path):
    if path == "":
        return value
    if not isinstance(path, str) or not path.startswith("/"):
        raise ValueError("Invalid JSON Pointer")
    for token in path[1:].split("/"):
        token = token.replace("~1", "/").replace("~0", "~")
        value = value[int(token)] if isinstance(value, list) else value[token]
    return value

def body(response):
    if response.get("error") or response.get("result", {}).get("isError"):
        raise ValueError("Failed MCP response")
    blocks = response["result"].get("content", [])
    texts = [b["text"] for b in blocks if b.get("type") == "text"]
    parsed = []
    for text in texts:
        try:
            parsed.append(json.loads(text))
        except json.JSONDecodeError:
            pass  # Non-JSON guidance remains preserved in the Raw response.
    if len(parsed) != 1:
        raise ValueError("Expected exactly one JSON content block")
    return parsed[0]

def read_calls(manifest_path):
    manifest_path = Path(manifest_path)
    manifest = load(manifest_path)
    validate_schema(manifest, "raw-manifest.schema.json")
    if manifest.get("failures"):
        raise ValueError("Capture contains failed transport attempts")
    files = {f["path"]: f for f in manifest["files"]}
    if len(files) != len(manifest["files"]):
        raise ValueError("Duplicate Raw file path")
    documents = {}
    for name, item in files.items():
        path = safe_path(manifest_path.parent, name)
        raw = path.read_bytes()
        if len(raw) != item["bytes"] or digest(raw) != item["sha256"]:
            raise ValueError("Raw hash mismatch: " + name)
        documents[name] = None
        if name in {c["response"]["path"] for c in manifest["calls"]}:
            documents[name] = json.loads(raw)
    result = {}
    for call in manifest["calls"]:
        if call["callId"] in result:
            raise ValueError("Duplicate call ID")
        ref = call["response"]
        response = pointer(documents[ref["path"]], ref["pointer"])
        if call["metadataSource"] == "captured-request-and-response":
            request_path = ref["path"].removesuffix(".json") + ".request.json"
            if request_path not in files:
                raise ValueError("Captured request file missing")
            request = load(safe_path(manifest_path.parent, request_path))
            params = {"name":call["tool"],"arguments":call["arguments"]} if call["method"] == "tools/call" else call["arguments"]
            if request.get("method") != call["method"] or request.get("params") != params or request.get("id") != response.get("id"):
                raise ValueError("Captured request metadata mismatch")
        if value_hash(response) != ref["sha256"]:
            raise ValueError("Response hash mismatch")
        if value_hash(response.get("result")) != call["payloadSha256"]:
            raise ValueError("Payload hash mismatch")
        parsed = body(response) if call["method"] == "tools/call" else response.get("result")
        if value_hash(parsed) != call["bodySha256"]:
            raise ValueError("Body hash mismatch")
        result[call["callId"]] = (call, parsed)
    return manifest, result

def validate_schema(value, name):
    """Validate the exact JSON Schema keyword subset used by this package.

    Schemas are standard draft-2020-12 documents. Unsupported keywords fail
    closed, so this dependency-free checker never silently skips a constraint.
    """
    schema = load(safe_path(ROOT / "schemas", name))
    allowed = {"$schema", "$id", "title", "description", "$defs", "$ref", "type", "properties",
               "required", "additionalProperties", "items", "enum", "const", "pattern", "minLength",
               "minimum", "minItems", "anyOf"}
    def check(v, s, where):
        unknown = set(s) - allowed
        if unknown:
            raise ValueError("Unsupported schema keywords: " + str(unknown))
        if "$ref" in s:
            return check(v, pointer(schema, s["$ref"][1:]), where)
        if "anyOf" in s:
            for candidate in s["anyOf"]:
                try:
                    check(v, candidate, where)
                    return
                except (ValueError, KeyError, TypeError):
                    pass
            raise ValueError("Schema anyOf: " + where)
        types = {"object": isinstance(v, dict), "array": isinstance(v, list), "string": isinstance(v, str),
                 "integer": type(v) is int, "number": type(v) in (int, float) and math.isfinite(v),
                 "boolean": type(v) is bool, "null": v is None}
        if "type" in s:
            wanted = s["type"] if isinstance(s["type"], list) else [s["type"]]
            if not any(types.get(t, False) for t in wanted):
                raise ValueError("Schema type: " + where)
        if "const" in s and (v != s["const"] or type(v) is not type(s["const"])):
            raise ValueError("Schema const: " + where)
        if "enum" in s and v not in s["enum"]:
            raise ValueError("Schema enum: " + where)
        if isinstance(v, dict):
            if set(s.get("required", [])) - set(v):
                raise ValueError("Schema required: " + where)
            props = s.get("properties", {})
            for k, item in v.items():
                if k in props:
                    check(item, props[k], where + "/" + escape(k))
                elif s.get("additionalProperties") is False:
                    raise ValueError("Schema additional property: " + where + "/" + k)
                elif isinstance(s.get("additionalProperties"), dict):
                    check(item, s["additionalProperties"], where + "/" + escape(k))
        if isinstance(v, list):
            if len(v) < s.get("minItems", 0):
                raise ValueError("Schema minItems: " + where)
            for i, item in enumerate(v):
                check(item, s.get("items", {}), where + "/" + str(i))
        if isinstance(v, str) and (len(v) < s.get("minLength", 0) or
                ("pattern" in s and re.search(s["pattern"], v) is None)):
            raise ValueError("Schema string: " + where)
        if type(v) in (int, float) and "minimum" in s and v < s["minimum"]:
            raise ValueError("Schema minimum: " + where)
    check(value, schema, name)


def read_artifacts(index_path, names):
    index_path = Path(index_path)
    index = load(index_path)
    validate_schema(index, "package-index.schema.json")
    documents = {}
    for name in names:
        artifact = index["artifacts"][name]
        raw = safe_path(index_path.parent, artifact["path"]).read_bytes()
        if len(raw) != artifact["bytes"] or digest(raw) != artifact["sha256"]:
            raise ValueError("Canonical artifact hash mismatch: " + name)
        document = json.loads(raw)
        validate_schema(document, artifact["schema"])
        documents[name] = document
    return index, documents
