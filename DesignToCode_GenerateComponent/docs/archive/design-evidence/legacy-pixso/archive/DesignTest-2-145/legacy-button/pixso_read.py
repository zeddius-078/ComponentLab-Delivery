"""Read-only Pixso MCP capture, without D2C+ or canvas mutations."""
import argparse
import json
from pathlib import Path
from urllib.request import Request, urlopen


def capture(endpoint, node_id):
    headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}

    def rpc(method, params, request_id=None):
        payload = {"jsonrpc": "2.0", "method": method, "params": params}
        if request_id is not None:
            payload["id"] = request_id
        with urlopen(Request(endpoint, json.dumps(payload).encode(), headers), timeout=30) as response:
            session = response.headers.get("Mcp-Session-Id")
            if session:
                headers["Mcp-Session-Id"] = session
            body = response.read().decode()
        if not body.strip():
            return None
        if body.lstrip().startswith("{"):
            return json.loads(body)
        messages = [json.loads(line[5:].strip()) for line in body.splitlines() if line.startswith("data:")]
        return next(x for x in messages if x.get("id") == request_id)

    rpc("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                       "clientInfo": {"name": "component-lab-reader", "version": "1"}}, 1)
    rpc("notifications/initialized", {})

    def call(name, arguments):
        response = rpc("tools/call", {"name": name, "arguments": arguments}, 2)
        if "error" in response:
            raise ValueError(response["error"])
        result = response["result"]
        if result.get("isError"):
            raise ValueError(result)
        value = json.loads(next(x["text"] for x in result["content"] if x["type"] == "text"))
        if isinstance(value, dict) and "error" in value:
            raise ValueError(value["error"])
        return value

    identity = call("eval_script", {"script": 'return {fileKey:pixso.fileKey};'})
    if identity.get("fileKey") != "4S0Ktm81WnvzuJmZHvRuOg":
        raise ValueError("Open the registered DesignTest file in Pixso first")
    dsl = call("get_node_dsl", {"guid": node_id, "simplify": True})
    return {"provider": "pixso", "fileKey": identity["fileKey"], "nodeId": node_id, "dsl": dsl}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--node", default="3:5616")
    parser.add_argument("--endpoint", default="http://127.0.0.1:3667/mcp")
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Output exists; choose a new snapshot path")
    result = capture(args.endpoint, args.node)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as stream:
        json.dump(result, stream, ensure_ascii=False, indent=2)
    print(f"Captured {result['fileKey']} / {args.node}: {args.output}")
