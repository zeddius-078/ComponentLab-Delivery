"""Capture read-only Pixso MCP calls and Raw Manifest; never normalize or map Qt."""
import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from urllib.request import Request, urlopen
from contract_io import body, digest, encoded, load, value_hash, write

COLLECTION_SCRIPT = ("const c = await pixso.variables.getLocalVariableCollectionsAsync(); "
                    "return c.map(x => ({id:x.id,name:x.name,defaultModeId:x.defaultModeId,"
                    "modes:(x.modes || []).map(m => ({modeId:m.modeId,name:m.name}))}));")

def now():
    return datetime.now(timezone.utc).isoformat()

class Capture:
    def __init__(self, output, endpoint, file_key, page_id, package_id):
        self.output = Path(output)
        if self.output.exists() and any(self.output.iterdir()):
            raise ValueError("Capture output must be empty")
        self.output.mkdir(parents=True, exist_ok=True)
        (self.output / "raw").mkdir(exist_ok=True)
        self.endpoint = endpoint
        self.headers = {"Content-Type":"application/json", "Accept":"application/json, text/event-stream"}
        self.sequence = 0
        self.manifest = {"schemaVersion":1, "packageId":package_id, "provider":"pixso", "fileKey":file_key,
                         "pageId":page_id, "captureId":self.output.name, "capturedAt":now(),
                         "captureTimeEvidence":"UTC capture start; each call records UTC request start",
                         "files":[], "calls":[], "notifications":[]}
    def checkpoint(self):
        target = self.output / "manifest.json"
        temporary = self.output / "manifest.pending.json"
        write(temporary, self.manifest)
        temporary.replace(target)
    def rpc(self, method, arguments, role, tool=None):
        self.sequence += 1
        call_id = "call-" + str(self.sequence).zfill(4)
        started = now()
        params = {"name":tool, "arguments":arguments} if method == "tools/call" else arguments
        request_body = encoded({"jsonrpc":"2.0", "id":self.sequence, "method":method, "params":params})
        request_file = self.output / "raw" / (role + "-" + call_id + ".request.json")
        request_file.write_bytes(request_body)
        self.manifest["files"].append({"path":"raw/"+request_file.name,"bytes":len(request_body),"sha256":digest(request_body)})
        try:
            with urlopen(Request(self.endpoint, request_body, self.headers), timeout=90) as response:
                session = response.headers.get("Mcp-Session-Id")
                if session:
                    self.headers["Mcp-Session-Id"] = session
                raw = response.read()
        except Exception as error:
            self.manifest.setdefault("failures", []).append({"callId":call_id,"role":role,"method":method,
                "tool":tool,"arguments":arguments,"capturedAt":started,"requestFile":"raw/"+request_file.name,
                "status":"client-transport-error","errorType":type(error).__name__,"message":str(error)})
            self.checkpoint()
            raise
        transport = self.output / "raw" / (role + "-" + call_id + ".transport.txt")
        transport.write_bytes(raw)
        self.manifest["files"].append({"path":"raw/"+transport.name,"bytes":len(raw),"sha256":digest(raw)})
        try:
            if raw.lstrip().startswith(b"{"):
                parsed = json.loads(raw)
                envelope_bytes = raw
                transport_name = role + "-" + call_id + ".json"
            else:
                events = [json.loads(line[5:].strip()) for line in raw.decode("utf-8").splitlines() if line.startswith("data:")]
                parsed = next(e for e in events if e.get("id") == self.sequence)
                envelope_bytes = encoded(parsed)
                transport_name = role + "-" + call_id + ".sse"
            failed = bool(parsed.get("error") or parsed.get("result", {}).get("isError"))
            value = parsed.get("result") if method != "tools/call" or failed else body(parsed)
        except Exception as error:
            self.manifest.setdefault("failures", []).append({"callId":call_id,"role":role,"method":method,
                "tool":tool,"arguments":arguments,"capturedAt":started,"requestFile":"raw/"+request_file.name,
                "responseFile":"raw/"+transport.name,"status":"client-response-decode-error",
                "errorType":type(error).__name__,"message":str(error)})
            self.checkpoint()
            raise
        target = self.output / "raw" / transport_name
        transport.rename(target)
        self.manifest["files"][-1]["path"] = "raw/"+target.name
        if target.suffix == ".sse":
            target = self.output / "raw" / (role + "-" + call_id + ".json")
            target.write_bytes(envelope_bytes)
            self.manifest["files"].append({"path":"raw/"+target.name,"bytes":len(envelope_bytes),"sha256":digest(envelope_bytes)})
        self.manifest["calls"].append({"callId":call_id,"role":role,"method":method,"tool":tool,"arguments":arguments,
                                      "capturedAt":started,"metadataSource":"captured-request-and-response",
                                      "response":{"path":"raw/"+target.name,"pointer":"","sha256":value_hash(parsed)},
                                      "payloadSha256":value_hash(parsed.get("result")),"bodySha256":value_hash(value)})
        self.checkpoint()
        if failed:
            raise ValueError("MCP call failed; Raw response preserved: " + call_id)
        return value
    def initialize(self):
        self.rpc("initialize", {"protocolVersion":"2024-11-05","capabilities":{},
                               "clientInfo":{"name":"pixso-evidence-capture","version":"2"}}, "initialize")
        payload = encoded({"jsonrpc":"2.0","method":"notifications/initialized","params":{}})
        target = self.output / "raw" / "initialized-notification.request.json"
        target.write_bytes(payload)
        self.manifest["files"].append({"path":"raw/"+target.name,"bytes":len(payload),"sha256":digest(payload)})
        with urlopen(Request(self.endpoint, payload, self.headers), timeout=30):
            pass
        self.manifest["notifications"].append({"method":"notifications/initialized","arguments":{},"capturedAt":now(),"requestFile":"raw/"+target.name})
        self.checkpoint()
    def tool(self, name, arguments, role):
        return self.rpc("tools/call", arguments, role, name)

def capture(output, endpoint, file_key, page_id, package_id, variants):
    session = Capture(output, endpoint, file_key, page_id, package_id)
    try:
        session.initialize()
        listing = session.rpc("tools/list", {}, "tools-list")
        wanted = {"eval_script","get_node_dsl","get_variable_sets","get_variables","get_all_components","get_local_styles","get_variants"}
        if not wanted.issubset({t["name"] for t in listing["tools"]}):
            raise ValueError("Required MCP tools unavailable")
        identity = session.tool("eval_script", {"script":"return {fileKey:pixso.fileKey};"}, "file-identity")
        if identity.get("fileKey") != file_key:
            raise ValueError("Wrong active Pixso file")
        page = session.tool("get_node_dsl", {"guid":page_id,"simplify":True}, "page-dsl")
        for variant_id in variants:
            session.tool("get_node_dsl", {"guid":variant_id,"simplify":True}, "component-variant-dsl")
        session.tool("get_variable_sets", {}, "variable-sets-full")
        session.tool("get_variables", {}, "variables-full")
        session.tool("get_all_components", {}, "components-full")
        session.tool("get_local_styles", {}, "local-styles")
        session.tool("eval_script", {"script":COLLECTION_SCRIPT}, "variable-collections")
        defined = {s["id"] for s in page.get("refsIndex",{}).get("componentSets",[])}
        referenced = set()
        def walk(node):
            if node.get("componentSetRef"):
                referenced.add(node["componentSetRef"])
            for child in node.get("children",[]):
                walk(child)
        for node in page["roots"]:
            walk(node)
        for set_id in sorted(referenced - defined):
            session.tool("get_variants", {"guid":set_id}, "state-group-variants")
            session.tool("get_node_dsl", {"guid":set_id,"simplify":True}, "state-group-dsl")
    finally:
        session.checkpoint()
    return session.manifest

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", default="http://127.0.0.1:3667/mcp")
    parser.add_argument("--package", default="DesignTest-2-145")
    parser.add_argument("--file-key", required=True)
    parser.add_argument("--page-id", required=True)
    parser.add_argument("--variant-id", action="append", default=[])
    parser.add_argument("--output", required=True, type=Path, help="Fresh runs/<capture-id> directory")
    args = parser.parse_args()
    result = capture(args.output,args.endpoint,args.file_key,args.page_id,args.package,args.variant_id)
    print(json.dumps({"captureId":result["captureId"],"calls":len(result["calls"])}))
