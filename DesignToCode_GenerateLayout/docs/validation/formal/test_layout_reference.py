"""布局证据采集合同：探针快照 + DSL FRAME 匹配，产出可验证的 layout-reference.json。"""
import json
import os
import subprocess
import sys
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[3]
CAPTURE = PROJECT / "Source/HtmlDsl/pipeline/capture"
TOOL = CAPTURE / "layout_reference.py"


class StubMcp(BaseHTTPRequestHandler):
    """最小 MCP 服务：initialize / tools/list / tools/call(eval_script)。"""
    snapshot = {}
    calls = []

    def log_message(self, *args):
        pass

    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        self.calls.append(body)
        method = body.get("method")
        if method == "initialize":
            payload = {"jsonrpc": "2.0", "id": body["id"], "result": {"protocolVersion": "2024-11-05"}}
        elif method == "notifications/initialized":
            self.send_response(202)
            self.end_headers()
            return
        elif method == "tools/list":
            payload = {"jsonrpc": "2.0", "id": body["id"],
                       "result": {"tools": [{"name": "eval_script", "inputSchema": {}}]}}
        elif method == "tools/call":
            arguments = body["params"]["arguments"]
            if body["params"]["name"] != "eval_script" or "__ROOT_ID__" in arguments.get("script", ""):
                payload = {"jsonrpc": "2.0", "id": body["id"],
                           "result": {"isError": True, "content": []}}
            else:
                payload = {"jsonrpc": "2.0", "id": body["id"],
                           "result": {"content": [
                               {"type": "text", "text": json.dumps(self.snapshot)}]}}
        else:
            self.send_response(400)
            self.end_headers()
            return
        raw = json.dumps(payload).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)


def frame(guid, children=None):
    return {"guid": guid, "type": "FRAME", "childNode": children or []}


class LayoutReferenceTests(unittest.TestCase):
    def setUp(self):
        # 探针快照：fileKey 与两个 FRAME 的设计属性（含显式 0）。
        self.snapshot = {
            "schema": "design-structure/v1",
            "provenance": "pixso-plugin-resolved-scene",
            "fileKey": "capture-fixture",
            "root": {
                "id": "fixture:1", "type": "FRAME", "itemSpacing": 8,
                "paddingLeft": 0, "paddingTop": 0, "paddingRight": 0, "paddingBottom": 0,
                "children": [{
                    "id": "fixture:2", "type": "FRAME", "itemSpacing": 0,
                    "paddingLeft": 4, "paddingTop": 4, "paddingRight": 4, "paddingBottom": 4,
                    "children": [{"id": "fixture:3", "type": "PARAGRAPH"}]}]},
        }
        StubMcp.snapshot = self.snapshot
        StubMcp.calls = []
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), StubMcp)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        base = PROJECT / "output/work/layout-reference-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="layout-ref-", dir=base)
        # LIFO：先关临时目录，再停服务，最后关套接字，避免关闭后的 select 告警。
        self.addCleanup(self.server.server_close)
        self.addCleanup(self.server.shutdown)
        self.addCleanup(self.temp.cleanup)
        self.dsl = Path(self.temp.name) / "canonical.raw.json"
        self.dsl.write_text(json.dumps({
            "pixTreeNodes": [frame("fixture:1", [
                frame("fixture:2", [{"guid": "fixture:3", "type": "PARAGRAPH"}])])],
        }), encoding="utf-8")
        self.output = Path(self.temp.name) / "layout"

    def run_tool(self):
        environment = dict(os.environ)
        environment.pop("D2C_RUNTIME_ROOT", None)
        environment.pop("D2C_WORK_ROOT", None)
        return subprocess.run(
            [sys.executable, "-B", str(TOOL), "--guid", "fixture:1",
             "--file-key", "capture-fixture", "--dsl", str(self.dsl),
             "--endpoint", f"http://127.0.0.1:{self.server.server_port}/mcp",
             "--output", str(self.output)],
            capture_output=True, timeout=60, env=environment)

    def test_dsl_frames_map_to_probe_metrics(self):
        process = self.run_tool()
        self.assertEqual(process.returncode, 0, process.stderr.decode(errors="replace"))
        evidence = json.loads((self.output / "layout-reference.json").read_bytes())
        self.assertEqual(evidence["fileKey"], "capture-fixture")
        self.assertEqual(evidence["pages"][0]["id"], "fixture:1")
        self.assertEqual(evidence["pages"][0]["nodes"], [
            {"id": "fixture:1", "metrics": {"gap": 8, "left": 0, "top": 0, "right": 0, "bottom": 0}},
            {"id": "fixture:2", "metrics": {"gap": 0, "left": 4, "top": 4, "right": 4, "bottom": 4}},
        ])
        # 四步记录保留，探针占位符已完成替换。
        for name in ("01-initialize.request.json", "02-initialized.request.json",
                     "03-tools.request.json", "04-call.request.json",
                     "04-call.response.body"):
            self.assertTrue((self.output / name).is_file(), name)
        call = json.loads((self.output / "04-call.request.json").read_bytes())
        script = call["params"]["arguments"]["script"]
        self.assertIn('"fixture:1"', script)
        self.assertNotIn("__ROOT_ID__", script)
        self.assertNotIn("__FILE_KEY__", script)

    def test_missing_metric_fails_closed_without_zero_fill(self):
        del self.snapshot["root"]["children"][0]["itemSpacing"]
        StubMcp.snapshot = self.snapshot
        process = self.run_tool()
        self.assertNotEqual(process.returncode, 0)
        self.assertIn("LAYOUT_REFERENCE_METRIC_NOT_NUMERIC: fixture:2/gap",
                      process.stderr.decode(errors="replace"))
        self.assertFalse((self.output / "layout-reference.json").exists())

    def test_dsl_frame_missing_from_probe_fails_closed(self):
        self.dsl.write_text(json.dumps({
            "pixTreeNodes": [frame("fixture:1", [frame("fixture:9")])],
        }), encoding="utf-8")
        process = self.run_tool()
        self.assertNotEqual(process.returncode, 0)
        self.assertIn("LAYOUT_REFERENCE_FRAME_MISSING: fixture:9",
                      process.stderr.decode(errors="replace"))


if __name__ == "__main__":
    unittest.main()
