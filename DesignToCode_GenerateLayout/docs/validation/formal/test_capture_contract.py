"""独立采集合同：浏览器默认值不能冒充设计属性，离线资源失败必须阻断。"""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[3]
ADAPTER = PROJECT / "Source/HtmlDsl/tools/adapt_v2.cjs"
DEFAULT_NODE_MODULES = PROJECT / "build/runtime/node_modules"


def node_environment():
    """为每次 Node 适配调用建立独立、可复现的模块搜索路径。"""
    environment = dict(os.environ)
    module_root = Path(environment.get("D2C_NODE_MODULES", DEFAULT_NODE_MODULES))
    if not (module_root / "playwright").is_dir():
        raise RuntimeError(
            "Playwright module missing. Set D2C_NODE_MODULES to its node_modules directory: "
            f"{module_root}"
        )
    environment["NODE_PATH"] = str(module_root)
    environment["D2C_RUNTIME_ROOT"] = str(PROJECT / "build/tools/layout")
    environment["D2C_WORK_ROOT"] = str(PROJECT)
    environment.setdefault("D2C_BROWSER_EXE", str(PROJECT / "build/runtime/browser/chrome.exe"))
    return environment


class CaptureContractTests(unittest.TestCase):
    def setUp(self):
        base = PROJECT / "output/work/capture-contract-tests"
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="layout-capture-", dir=base)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.html = """<!doctype html><meta charset="utf-8"><style>
body{margin:0} #fixture_1{box-sizing:border-box;width:80px;height:60px;
display:flex;flex-direction:row;gap:8px;padding:0px;align-items:flex-start;
justify-content:flex-start} #fixture_2{width:20px;height:20px}
</style><div id="fixture_1"><div id="fixture_2">A</div></div>"""
        self.raw = {"pixTreeNodes": [{"guid": "fixture:1", "type": "FRAME",
            "width": 80, "height": 60, "childNode": [{"guid": "fixture:2",
                "type": "PARAGRAPH", "width": 20, "height": 20}]}]}
        self.metrics = dict(gap=8, left=0, top=0, right=0, bottom=0)
        self.capture = dict(sourceHtml=str(self.root / "page.html"),
            sourceDsl=str(self.root / "raw.json"), expectedRootId="fixture:1",
            resources=[], fontFamily="Arial", fileKey="capture-fixture",
            layoutReference=str(self.root / "reference.json"))

    def run_capture(self):
        (self.root / "page.html").write_text(self.html, encoding="utf-8")
        (self.root / "raw.json").write_text(json.dumps(self.raw), encoding="utf-8")
        reference = {"fileKey": "capture-fixture", "pages": [{"id": "fixture:1",
            "nodes": [{"id": "fixture:1", "metrics": self.metrics}]}]}
        (self.root / "reference.json").write_text(json.dumps(reference), encoding="utf-8")
        (self.root / "capture.json").write_text(json.dumps(self.capture), encoding="utf-8")
        result = subprocess.run([os.environ.get("NODE_EXE", str(PROJECT / "build/runtime/node/node.exe")), str(ADAPTER), str(self.root / "working"),
            str(self.root / "report"), str(self.root / "capture.json")],
            capture_output=True, timeout=45, env=node_environment())
        report = json.loads((self.root / "report/html-result.json").read_bytes())
        return result, report

    def assert_failure(self, code):
        process, report = self.run_capture()
        self.assertNotEqual(process.returncode, 0)
        self.assertEqual(report["status"], "FAIL")
        self.assertIn(code, report["error"])
        self.assertFalse((self.root / "working/hybrid-result.json").exists())

    def test_explicit_zero_is_proven_and_preserved(self):
        process, report = self.run_capture()
        self.assertEqual(process.returncode, 0, process.stderr.decode(errors="replace"))
        self.assertEqual(report["status"], "PASS")
        value = json.loads((self.root / "working/hybrid-result.json").read_bytes())
        layout = value["pixTreeNodes"][0]["autoLayout"]
        self.assertEqual(layout["autoLayoutPaddingLeft"], 0)
        self.assertEqual(layout["autoLayoutItemSpacing"], 8)
        self.assertEqual(len(value["_provenance"]["layoutReferenceSha256"]), 64)

    def test_missing_design_value_is_not_browser_default(self):
        del self.metrics["left"]
        self.assert_failure("MISSING_DESIGN_LAYOUT_EVIDENCE")

    def test_conflicting_design_value_does_not_override_html(self):
        self.metrics["gap"] = 6
        self.assert_failure("DESIGN_HTML_LAYOUT_MISMATCH")

    def test_wrong_root_identity_is_rejected(self):
        self.capture["expectedRootId"] = "other:1"
        self.assert_failure("CAPTURE_ROOT_MISMATCH")

    def test_network_resource_cannot_silently_fill_cache_gap(self):
        self.html += '<img src="https://example.invalid/unexported.png">'
        self.assert_failure("HTML_RENDER_FAILED")


if __name__ == "__main__":
    unittest.main()
