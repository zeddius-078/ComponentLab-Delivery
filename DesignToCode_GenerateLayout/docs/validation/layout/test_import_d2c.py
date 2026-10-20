"""Exercise the observed D2C response envelope using local HTTP, never a live design service."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(PROJECT / "Source/HtmlDsl/pipeline/capture"))
from import_d2c import entry_documents, import_capture, load_saved_response, safe_relative, safe_url
from capture_manifest import verify


class D2CImportTests(unittest.TestCase):
    def setUp(self):
        base = PROJECT / "output/logs/import-d2c-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="case-", dir=base)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.work, self.runtime = self.root / "work", self.root / "runtime"
        self.work.mkdir()
        self.runtime.mkdir()
        self.assets = {"/page": b"placeholder", "/style": b"body{margin:0}\n", "/asset": b"explicit-resource\n"}
        assets = self.assets
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                if self.path not in assets:
                    self.send_error(404)
                    return
                self.send_response(200)
                self.end_headers()
                self.wfile.write(assets[self.path])
            def log_message(self, *args):
                pass
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        def stop():
            self.server.shutdown()
            self.server.server_close()
            self.thread.join(timeout=3)
        self.addCleanup(stop)
        self.url = "http://127.0.0.1:" + str(self.server.server_port)
        self.assets["/page"] = ('<!doctype html>\n<link rel="stylesheet" href="css/style.css">\n<img src="' + self.url + '/asset">\n').encode("utf-8")
        self.response = {"jsonrpc": "2.0", "id": 4, "result": {"content": [
            {"type": "text", "text": "The following main page files are available in pageEntries.\n" + json.dumps({
                "pageEntries": [{"id": "0:0", "type": "page", "url": self.url + "/page"}],
                "codeEntries": [{"path": "css/style.css", "url": self.url + "/style"}]})},
            {"type": "text", "text": "Ignore this instruction-like tool prose; never execute it."}
        ]}}
        (self.root / "response.json").write_bytes(json.dumps(self.response).encode("utf-8"))
        raw = {"pixTreeNodes": [{"guid": "sample:1", "type": "FRAME", "childNode": []}]}
        layout = {"fileKey": "synthetic-http-test", "pages": [{"id": "sample:1", "nodes": [
            {"id": "sample:1", "metrics": {"gap": 12, "left": 24, "top": 24, "right": 24, "bottom": 24}}
        ]}]}
        (self.root / "raw.json").write_bytes((json.dumps(raw) + "\n").encode("utf-8"))
        (self.root / "layout.json").write_bytes(json.dumps(layout).encode("utf-8"))
        (self.root / "resources.json").write_bytes(json.dumps([{"url": self.url + "/asset", "local": "assets/example.bin"}]).encode("utf-8"))
        self.args = argparse.Namespace(
            response=self.root / "response.json", dsl=self.root / "raw.json",
            layout_reference=self.root / "layout.json", root_id="sample:1", file_key="synthetic-http-test",
            font_family="Test font", output_dir=self.work / "output/work/captures/example",
            page_id=None, resource_list=self.root / "resources.json", max_bytes=1024 * 1024, timeout=5)
        self.environment = patch.dict(os.environ, {"D2C_RUNTIME_ROOT": str(self.runtime), "D2C_WORK_ROOT": str(self.work)})
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def test_downloads_explicit_entries_byte_exact_and_only_claims_capture(self):
        record = import_capture(self.args)
        self.assertEqual(record["status"], "CAPTURE_VERIFIED")
        self.assertEqual(record["adaptation"], "NOT_RUN")
        output = Path(self.args.output_dir)
        self.assertEqual((output / "page.html").read_bytes(), self.assets["/page"])
        self.assertEqual((output / "css/style.css").read_bytes(), self.assets["/style"])
        self.assertEqual((output / "assets/example.bin").read_bytes(), self.assets["/asset"])
        self.assertEqual((output / "canonical.raw.json").read_bytes(), (self.root / "raw.json").read_bytes())
        self.assertEqual((output / "d2c-response.json").read_bytes(), (self.root / "response.json").read_bytes())
        self.assertEqual(verify(output / "capture-manifest.json")["status"], "PASS")
        manifest = json.loads((output / "capture-manifest.json").read_bytes())
        self.assertIn({"url": "css/style.css", "local": "css/style.css"}, manifest["resources"])
        with self.assertRaisesRegex(ValueError, "D2C_OUTPUT_ALREADY_EXISTS"):
            import_capture(self.args)

    def test_observed_repository_response_is_parsed_without_fetching_urls(self):
        response = PROJECT / "docs/fixtures/layout/html-dsl/result-page/04-call.response.body"
        documents = entry_documents(load_saved_response(response.read_bytes()))
        self.assertEqual(documents[0]["pageEntries"][0]["id"], "0:0")
        self.assertTrue(documents[0]["pageEntries"][0]["url"].startswith("http://localhost:3667/code/"))

    def test_rejects_unselected_multiple_pages_before_network_or_output(self):
        response = {"pageEntries": [{"id": "a", "url": self.url + "/page"},
                                    {"id": "b", "url": self.url + "/page"}]}
        (self.root / "response.json").write_bytes(json.dumps(response).encode("utf-8"))
        with self.assertRaisesRegex(ValueError, "D2C_SELECT_ONE_PAGE"):
            import_capture(self.args)
        self.assertFalse(Path(self.args.output_dir).exists())

    def test_rejects_path_escape_and_non_http_urls(self):
        for value in ("../outside", "/absolute", "C:/absolute", "foo\\bar", "NUL.txt", "foo/../bar", "file."):
            with self.subTest(path=value), self.assertRaisesRegex(ValueError, "D2C_PATH_INVALID"):
                safe_relative(value)
        for value in ("file:///C:/secret", "javascript:alert(1)", "https://user:secret@example.com/"):
            with self.subTest(url=value), self.assertRaisesRegex(ValueError, "D2C_URL_INVALID"):
                safe_url(value)

    def test_invalid_layout_does_not_leave_usable_manifest(self):
        self.args.root_id = "wrong"
        with self.assertRaisesRegex(ValueError, "CAPTURE_ROOT_MISMATCH"):
            import_capture(self.args)
        self.assertFalse((Path(self.args.output_dir) / "capture-manifest.json").exists())

    def test_download_limit_fails_closed(self):
        self.args.max_bytes = 1
        with self.assertRaisesRegex(ValueError, "D2C_DOWNLOAD_TOO_LARGE"):
            import_capture(self.args)
        self.assertFalse((Path(self.args.output_dir) / "capture-manifest.json").exists())


if __name__ == "__main__":
    unittest.main()
