import argparse
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse
from urllib.request import Request, urlopen


PROJECT = Path(__file__).resolve().parents[3]
WORKSPACE_SUPPORT = PROJECT.parent / "Source"
if str(WORKSPACE_SUPPORT) not in sys.path:
    sys.path.insert(0, str(WORKSPACE_SUPPORT))
from ArtifactLifecycle import fingerprint, verify_execution_capability

os.environ.setdefault("D2C_RUNTIME_ROOT", str(PROJECT / "build/tools/layout"))
os.environ.setdefault("D2C_WORK_ROOT", str(PROJECT))
os.environ.setdefault("NODE_EXE", str(PROJECT / "build/runtime/node/node.exe"))
os.environ.setdefault("D2C_NODE_MODULES", str(PROJECT / "build/runtime/node_modules"))
os.environ.setdefault("D2C_BROWSER_EXE", str(PROJECT / "build/runtime/browser/chrome.exe"))
RUNTIME = Path(os.environ["D2C_RUNTIME_ROOT"]).resolve()
COMPILER = RUNTIME / "bin/v2_ui_compiler.exe"
UIC = RUNTIME / "bin/uic.exe"
LIFECYCLE = RUNTIME / "scripts/ArtifactLifecycle.py"
CAPTURE_MCP = PROJECT / "Source/HtmlDsl/pipeline/capture/capture_mcp.py"
LAYOUT_REFERENCE = PROJECT / "Source/HtmlDsl/pipeline/capture/layout_reference.py"
IMPORT_D2C = PROJECT / "Source/HtmlDsl/pipeline/capture/import_d2c.py"
MANIFEST_TOOL = PROJECT / "Source/HtmlDsl/pipeline/capture/capture_manifest.py"
ADAPTER = PROJECT / "Source/HtmlDsl/tools/adapt_v2.cjs"
NODE = Path(os.environ["NODE_EXE"]).resolve()
NODE_MODULES = Path(os.environ["D2C_NODE_MODULES"]).resolve()
BROWSER = Path(os.environ["D2C_BROWSER_EXE"]).resolve()
DEFAULT_MAPPING = PROJECT / "docs/fixtures/layout/mapping"
DEFAULT_FONT = PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"
CHECK_DELIVERY = PROJECT / "docs/validation/check_delivery.py"


def verify_runtime_matches_source():
    """Reject a direct Python audit when its controlled EXE predates Source.

    This duplicate guard is intentional: RunPageAudit.py is callable with
    Python directly, so relying only on the PowerShell wrapper would leave a
    bypass for constrained Agents.  A standalone released package does not
    execute this source-side audit script.
    """
    manifest_path = RUNTIME / "tool-manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError("LAYOUT_TOOL_SOURCE_PROVENANCE_MISSING")
    try:
        acceptance = json.loads(manifest_path.read_text(encoding="utf-8"))["acceptance"]
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise RuntimeError("LAYOUT_TOOL_SOURCE_PROVENANCE_INVALID") from error
    published = str(acceptance.get("sourceFingerprint") or "")
    if not published:
        raise RuntimeError("LAYOUT_TOOL_SOURCE_PROVENANCE_MISSING")
    current = fingerprint(PROJECT)
    if published != current:
        raise RuntimeError("LAYOUT_TOOL_STALE_SOURCE")
    try:
        verify_execution_capability(acceptance.get("executionCapabilityProfile"))
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise RuntimeError("LAYOUT_TOOL_AGENT_BASELINE_MISSING") from error

GENERIC_FONTS = {
    "serif", "sans-serif", "monospace", "cursive", "fantasy", "system-ui",
    "inherit", "initial", "unset", "ui-sans-serif", "ui-serif", "ui-monospace",
}
LOCAL_HOSTS = {"localhost", "127.0.0.1", "::1"}
RESOURCE_ERROR_MARKERS = (
    "HTML_RENDER_FAILED",
    "net::ERR_FAILED",
    "RESOURCE_NOT_IN_SOURCE",
    "CAPTURE_RESOURCE_MISSING",
    "D2C_DOWNLOAD_TOO_LARGE",
    "D2C_URL_INVALID",
)
URL_PATTERN = re.compile(r"https?://[^\s\"'<>()]+", re.IGNORECASE)


class Runner:
    def __init__(self, args, target):
        self.args = args
        self.target = target
        self.steps = []
        self.artifacts = {}
        self.start = time.perf_counter()
        self.started_at = datetime.now(timezone.utc).isoformat()
        self.batch = target["batch"]
        self.page = target["page"]
        self.root_name = ""
        self.env = self.build_environment()
        self.run_dir = PROJECT / "output/work/audit" / self.batch
        self.logs_dir = PROJECT / "output/logs/audit" / self.batch
        self.run_dir.mkdir(parents=True, exist_ok=True)
        self.logs_dir.mkdir(parents=True, exist_ok=True)
        self.log_path = self.logs_dir / "run.log"
        self.log_stream = self.log_path.open("w", encoding="utf-8")
        self.dsl_dir = self.run_dir / "captures/dsl"
        self.d2c_dir = self.run_dir / "captures/d2c"
        self.layout_dir = self.run_dir / "captures/layout"
        self.page_dir = self.run_dir / "captures/page"
        self.capture_dir = None
        self.manifest = None
        self.layout_reference = None
        self.d2c_response = None
        self.page_html = None
        self.resources = []
        self.font_family = ""
        self.font = None
        self.hybrid = None
        self.structure_ui = None
        self.strict_ui = None
        self.structure_base = None
        self.delivery_page = None
        self.retried = False
        self.mcp_ok = False
        self.preflight_ok = False
        self.integrity_ok = False

    def build_environment(self):
        env = os.environ.copy()
        env.setdefault("D2C_RUNTIME_ROOT", str(RUNTIME))
        env.setdefault("D2C_WORK_ROOT", str(PROJECT))
        env.setdefault("NODE_EXE", str(NODE))
        env.setdefault("D2C_NODE_MODULES", str(NODE_MODULES))
        env.setdefault("D2C_BROWSER_EXE", str(BROWSER))
        env["NODE_PATH"] = env.get("NODE_PATH") or str(NODE_MODULES)
        env["PYTHONDONTWRITEBYTECODE"] = "1"
        env["QT_QPA_PLATFORM"] = "offscreen"
        env["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(RUNTIME / "bin/platforms")
        env["QT_PLUGIN_PATH"] = str(RUNTIME / "bin")
        return env

    def log(self, text):
        self.log_stream.write(text)
        if not text.endswith("\n"):
            self.log_stream.write("\n")
        self.log_stream.flush()

    def run_command(self, command, timeout=120, cwd=None, quiet_stdout=False):
        cmd = [str(item) for item in command]
        start = time.perf_counter()
        try:
            if quiet_stdout:
                process = subprocess.run(
                    cmd, cwd=str(cwd or PROJECT), env=self.env,
                    stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                    text=True, encoding="utf-8", errors="replace", timeout=timeout,
                )
                stdout = ""
                stderr = process.stderr or ""
            else:
                process = subprocess.run(
                    cmd, cwd=str(cwd or PROJECT), env=self.env,
                    capture_output=True, text=True, encoding="utf-8",
                    errors="replace", timeout=timeout,
                )
                stdout = process.stdout or ""
                stderr = process.stderr or ""
            elapsed = int(round((time.perf_counter() - start) * 1000))
            return {
                "status": "PASS" if process.returncode == 0 else "FAIL",
                "command": cmd,
                "exitCode": process.returncode,
                "stdout": stdout,
                "stderr": stderr,
                "commandElapsedMs": elapsed,
            }
        except subprocess.TimeoutExpired:
            elapsed = int(round((time.perf_counter() - start) * 1000))
            return {
                "status": "FAIL", "command": cmd, "exitCode": None,
                "stdout": "", "stderr": f"TIMEOUT after {timeout}s",
                "commandElapsedMs": elapsed,
            }
        except Exception as exc:
            elapsed = int(round((time.perf_counter() - start) * 1000))
            return {
                "status": "FAIL", "command": cmd, "exitCode": None,
                "stdout": "", "stderr": f"{type(exc).__name__}: {exc}",
                "commandElapsedMs": elapsed,
            }

    def execute(self, name, action):
        start = time.perf_counter()
        try:
            outcome = action()
            if not isinstance(outcome, dict):
                outcome = {"status": "PASS"}
        except Exception as exc:
            outcome = {
                "status": "FAIL",
                "error": f"{type(exc).__name__}: {exc}",
            }
        elapsed = int(round((time.perf_counter() - start) * 1000))
        record = {
            "name": name,
            "status": outcome.get("status", "PASS"),
            "elapsedMs": elapsed,
            "error": outcome.get("error", ""),
            "command": outcome.get("command", []),
            "exitCode": outcome.get("exitCode"),
            "stdout": self.cap(outcome.get("stdout", "")),
            "stderr": self.cap(outcome.get("stderr", "")),
            "outputs": outcome.get("outputs", []),
            "notes": outcome.get("notes", ""),
        }
        self.steps.append(record)
        self.log_step(record, outcome)
        print(f"{record['status']:4} {record['elapsedMs']:6} ms  {record['name']}")
        return record

    def cap(self, value, limit=4000):
        text = str(value or "")
        if len(text) <= limit:
            return text
        return text[:limit] + f"\n...[truncated {len(text) - limit} chars]"

    def log_step(self, record, outcome):
        self.log(
            f"[{datetime.now(timezone.utc).isoformat()}] {record['name']} "
            f"{record['status']} {record['elapsedMs']}ms"
        )
        command = record.get("command") or []
        if command:
            self.log("command: " + json.dumps(command, ensure_ascii=False))
        if record.get("exitCode") is not None:
            self.log("exit: " + str(record["exitCode"]))
        stdout = str(outcome.get("stdout", "") or "")
        stderr = str(outcome.get("stderr", "") or "")
        if stdout:
            self.log("stdout:\n" + stdout[:12000])
        if stderr:
            self.log("stderr:\n" + stderr[:12000])
        if record.get("error"):
            self.log("error: " + record["error"])
        self.log("")

    def skip(self, reason):
        return {"status": "SKIP", "error": reason}

    def not_requested(self, reason):
        return {"status": "NOT_RUN", "notes": reason}

    def require(self, condition, reason):
        if not condition:
            return self.skip(reason)
        return None

    def step_preflight(self):
        mapping = Path(self.args.mapping)
        checks = {
            "python39": sys.version_info[:2] == (3, 9),
            "compiler": COMPILER.is_file(),
            "uic": UIC.is_file(),
            "lifecycle": LIFECYCLE.is_file(),
            "node": NODE.is_file(),
            "browser": BROWSER.is_file(),
            "playwright": (NODE_MODULES / "playwright").is_dir(),
            "mapping": (mapping / "component-mapping.json").is_file(),
            "extensions": (mapping / "extensions.json").is_file(),
            "layoutMapping": (mapping / "layout-mapping.json").is_file(),
            "libraryProfiles": (mapping / "library-profiles.json").is_file(),
        }
        missing = [key for key, value in checks.items() if not value]
        self.preflight_ok = not missing
        if missing:
            return {
                "status": "FAIL",
                "error": "MISSING_PREFLIGHT: " + ", ".join(missing),
                "notes": json.dumps(checks, ensure_ascii=False),
            }
        return {"status": "PASS", "notes": json.dumps(checks, ensure_ascii=False)}

    def step_tool_integrity(self):
        if not LIFECYCLE.is_file():
            return self.skip("MISSING_ARTIFACT_LIFECYCLE")
        outcome = self.run_command(
            [sys.executable, "-B", LIFECYCLE, "verify-tools", "--root", RUNTIME],
            timeout=120,
        )
        if outcome["status"] == "PASS":
            try:
                verify_runtime_matches_source()
            except RuntimeError as error:
                outcome["status"] = "FAIL"
                outcome["error"] = str(error)
        self.integrity_ok = outcome["status"] == "PASS"
        return outcome

    def step_mcp_probe(self):
        try:
            names = probe_mcp(self.args.endpoint)
            self.mcp_ok = True
            required = ["get_node_dsl", "design_to_code", "eval_script"]
            missing = [name for name in required if name not in names]
            if missing:
                self.mcp_ok = False
                return {
                    "status": "FAIL",
                    "error": "MCP_TOOLS_MISSING: " + ", ".join(missing),
                    "notes": ", ".join(names),
                }
            return {"status": "PASS", "notes": ", ".join(names)}
        except Exception as exc:
            self.mcp_ok = False
            return {"status": "FAIL", "error": f"{type(exc).__name__}: {exc}"}

    def step_dsl_capture(self):
        blocked = self.require(self.mcp_ok, "MCP_PROBE_NOT_PASS")
        if blocked:
            return blocked
        arguments = json.dumps({
            "guid": self.target["rootId"],
            "simplify": False,
            "isDetachInstance": False,
        })
        outcome = self.run_command([
            sys.executable, "-B", CAPTURE_MCP, "--tool", "get_node_dsl",
            "--arguments", arguments, "--output", self.dsl_dir,
            "--endpoint", self.args.endpoint,
        ], timeout=180)
        if outcome["status"] != "PASS":
            return outcome
        dsl_path = self.dsl_dir / "canonical.raw.json"
        if not dsl_path.is_file():
            outcome["status"] = "FAIL"
            outcome["error"] = "DSL_OUTPUT_MISSING"
            return outcome
        raw = json.loads(dsl_path.read_text(encoding="utf-8"))
        roots = raw.get("pixTreeNodes") or []
        if len(roots) != 1 or roots[0].get("guid") != self.target["rootId"]:
            outcome["status"] = "FAIL"
            outcome["error"] = "DSL_ROOT_MISMATCH"
            return outcome
        self.root_name = str(roots[0].get("name") or "")
        if not self.args.page:
            self.page = normalize_page_name(self.root_name, self.target["rootId"])
        self.artifacts["dsl"] = str(dsl_path)
        self.artifacts["rootName"] = self.root_name
        outcome["outputs"] = [str(dsl_path)]
        outcome["notes"] = f"root={self.root_name}; nodes={count_nodes(roots[0])}"
        return outcome

    def step_d2c_capture(self):
        blocked = self.require(self.mcp_ok, "MCP_PROBE_NOT_PASS")
        if blocked:
            return blocked
        arguments = json.dumps({"guids": [self.target["rootId"]]})
        outcome = self.run_command([
            sys.executable, "-B", CAPTURE_MCP, "--tool", "design_to_code",
            "--arguments", arguments, "--output", self.d2c_dir,
            "--endpoint", self.args.endpoint,
        ], timeout=300)
        if outcome["status"] != "PASS":
            return outcome
        response = self.d2c_dir / "04-call.response.body"
        if not response.is_file():
            outcome["status"] = "FAIL"
            outcome["error"] = "D2C_RESPONSE_MISSING"
            return outcome
        self.d2c_response = response
        self.artifacts["d2cResponse"] = str(response)
        outcome["outputs"] = [str(response)]
        return outcome

    def step_layout_reference(self):
        blocked = self.require((self.dsl_dir / "canonical.raw.json").is_file(), "DSL_OUTPUT_MISSING")
        if blocked:
            return blocked
        outcome = self.run_command([
            sys.executable, "-B", LAYOUT_REFERENCE,
            "--guid", self.target["rootId"],
            "--file-key", self.target["fileKey"],
            "--dsl", self.dsl_dir / "canonical.raw.json",
            "--output", self.layout_dir,
            "--endpoint", self.args.endpoint,
        ], timeout=180)
        if outcome["status"] != "PASS":
            return outcome
        path = self.layout_dir / "layout-reference.json"
        if not path.is_file():
            outcome["status"] = "FAIL"
            outcome["error"] = "LAYOUT_REFERENCE_MISSING"
            return outcome
        self.layout_reference = path
        self.artifacts["layoutReference"] = str(path)
        outcome["outputs"] = [str(path)]
        return outcome

    def step_import_page(self):
        blocked = self.require(
            (self.dsl_dir / "canonical.raw.json").is_file()
            and self.d2c_response is not None
            and self.layout_reference is not None,
            "CAPTURE_INPUTS_MISSING",
        )
        if blocked:
            return blocked
        command = [
            sys.executable, "-B", IMPORT_D2C,
            "--response", self.d2c_response,
            "--dsl", self.dsl_dir / "canonical.raw.json",
            "--layout-reference", self.layout_reference,
            "--root-id", self.target["rootId"],
            "--file-key", self.target["fileKey"],
            "--font-family", self.args.font_family or "Arial",
            "--output-dir", self.page_dir,
        ]
        if self.args.page_id:
            command += ["--page-id", self.args.page_id]
        outcome = self.run_command(command, timeout=300)
        if outcome["status"] != "PASS":
            return outcome
        self.page_html = self.page_dir / "page.html"
        if not self.page_html.is_file():
            outcome["status"] = "FAIL"
            outcome["error"] = "PAGE_HTML_MISSING"
            return outcome
        self.artifacts["pageHtml"] = str(self.page_html)
        outcome["outputs"] = [str(self.page_html)]
        return outcome

    def step_resource_scan(self):
        blocked = self.require(self.page_html is not None and self.page_html.is_file(), "PAGE_HTML_MISSING")
        if blocked:
            return blocked
        resource_path = self.run_dir / "resource-list.json"
        resources = scan_resources(self.page_html, resource_path)
        self.resources = resources
        self.font_family = self.args.font_family or detect_font_family(
            self.page_html.read_text(encoding="utf-8")
        )
        self.artifacts["resourceList"] = str(resource_path)
        return {
            "status": "PASS",
            "outputs": [str(resource_path)],
            "notes": f"resources={len(resources)}; fontFamily={self.font_family}",
        }

    def step_import_final(self):
        blocked = self.require(self.page_html is not None and self.page_html.is_file(), "PAGE_HTML_MISSING")
        if blocked:
            return blocked
        resource_path = self.run_dir / "resource-list.json"
        if not resource_path.is_file():
            return self.skip("RESOURCE_LIST_MISSING")
        capture_dir = self.run_dir / "captures/capture"
        command = [
            sys.executable, "-B", IMPORT_D2C,
            "--response", self.d2c_response,
            "--dsl", self.dsl_dir / "canonical.raw.json",
            "--layout-reference", self.layout_reference,
            "--root-id", self.target["rootId"],
            "--file-key", self.target["fileKey"],
            "--font-family", self.font_family or self.args.font_family or "Arial",
            "--resource-list", resource_path,
            "--output-dir", capture_dir,
        ]
        if self.args.page_id:
            command += ["--page-id", self.args.page_id]
        outcome = self.run_command(command, timeout=300)
        if outcome["status"] == "PASS":
            self.capture_dir = capture_dir
            self.manifest = capture_dir / "capture-manifest.json"
            self.artifacts["captureDirectory"] = str(capture_dir)
            self.artifacts["manifest"] = str(self.manifest)
            outcome["outputs"] = [str(self.manifest)]
            return outcome
        staging_manifest = self.page_dir / "capture-manifest.json"
        if staging_manifest.is_file():
            self.capture_dir = self.page_dir
            self.manifest = staging_manifest
            self.artifacts["captureDirectory"] = str(self.page_dir)
            self.artifacts["manifest"] = str(staging_manifest)
            outcome["notes"] = "FINAL_IMPORT_FAILED_USING_STAGING_MANIFEST"
        return outcome

    def step_manifest_verify(self):
        blocked = self.require(self.manifest is not None and self.manifest.is_file(), "MANIFEST_MISSING")
        if blocked:
            return blocked
        outcome = self.run_command([
            sys.executable, "-B", MANIFEST_TOOL, "verify",
            "--manifest", self.manifest, "--mode", "formal",
        ], timeout=120)
        if outcome["status"] == "PASS":
            outcome["outputs"] = [str(self.manifest)]
        return outcome

    def step_font_resolve(self):
        blocked = self.require(self.capture_dir is not None and self.capture_dir.is_dir(), "CAPTURE_DIRECTORY_MISSING")
        if blocked:
            return blocked
        if not self.font_family and self.page_html and self.page_html.is_file():
            self.font_family = self.args.font_family or detect_font_family(
                self.page_html.read_text(encoding="utf-8")
            )
        font, source = resolve_font(
            self.capture_dir, self.font_family, self.args.font, DEFAULT_FONT
        )
        if font is None:
            return {"status": "FAIL", "error": "FONT_NOT_FOUND"}
        self.font = font
        self.artifacts["font"] = str(font)
        self.artifacts["fontFamily"] = self.font_family
        return {
            "status": "PASS",
            "outputs": [str(font)],
            "notes": f"source={source}; fontFamily={self.font_family}",
        }

    def step_html_adapt(self):
        blocked = self.require(
            self.capture_dir is not None
            and self.manifest is not None
            and self.manifest.is_file()
            and NODE.is_file()
            and BROWSER.is_file(),
            "ADAPT_INPUTS_MISSING",
        )
        if blocked:
            return blocked
        adapt_dir = self.run_dir / "adapt"
        report_dir = self.logs_dir / "adaptation"
        outcome = self.run_command([
            NODE, ADAPTER, adapt_dir, report_dir, self.manifest,
        ], timeout=180)
        if outcome["status"] != "PASS":
            return outcome
        hybrid = adapt_dir / "hybrid-result.json"
        if not hybrid.is_file():
            outcome["status"] = "FAIL"
            outcome["error"] = "HYBRID_RESULT_MISSING"
            return outcome
        self.hybrid = hybrid
        self.artifacts["hybrid"] = str(hybrid)
        outcome["outputs"] = [str(hybrid)]
        summary = last_json_line(outcome.get("stdout", ""))
        if summary:
            outcome["notes"] = json.dumps(summary, ensure_ascii=False)
        return outcome

    def step_html_adapt_retry(self):
        if self.hybrid is not None:
            return self.skip("NOT_NEEDED")
        adapt_step = find_step(self.steps, "HTML_ADAPT")
        if not adapt_step or adapt_step["status"] == "SKIP":
            return self.skip("HTML_ADAPT_NOT_RUN")
        text = (adapt_step.get("stderr", "") + "\n" + adapt_step.get("stdout", "")).lower()
        if not any(marker.lower() in text for marker in RESOURCE_ERROR_MARKERS):
            return self.skip("NOT_A_RESOURCE_ERROR")
        if self.page_html is None or not self.page_html.is_file():
            return self.skip("PAGE_HTML_MISSING")
        if self.d2c_response is None or self.layout_reference is None:
            return self.skip("CAPTURE_INPUTS_MISSING")
        if self.retried:
            return self.skip("RETRY_ALREADY_ATTEMPTED")
        self.retried = True
        retry_id = "-retry"
        resource_path = self.run_dir / f"resource-list{retry_id}.json"
        scan_resources(self.page_html, resource_path)
        retry_capture = self.run_dir / f"captures/capture{retry_id}"
        command = [
            sys.executable, "-B", IMPORT_D2C,
            "--response", self.d2c_response,
            "--dsl", self.dsl_dir / "canonical.raw.json",
            "--layout-reference", self.layout_reference,
            "--root-id", self.target["rootId"],
            "--file-key", self.target["fileKey"],
            "--font-family", self.font_family or self.args.font_family or "Arial",
            "--resource-list", resource_path,
            "--output-dir", retry_capture,
        ]
        if self.args.page_id:
            command += ["--page-id", self.args.page_id]
        imported = self.run_command(command, timeout=300)
        if imported["status"] != "PASS":
            imported["error"] = "RETRY_IMPORT_FAILED: " + tail(imported.get("stderr", ""))
            return imported
        manifest = retry_capture / "capture-manifest.json"
        if not manifest.is_file():
            return {"status": "FAIL", "error": "RETRY_MANIFEST_MISSING"}
        adapt_dir = self.run_dir / f"adapt{retry_id}"
        report_dir = self.logs_dir / f"adaptation{retry_id}"
        adapted = self.run_command([
            NODE, ADAPTER, adapt_dir, report_dir, manifest,
        ], timeout=180)
        if adapted["status"] != "PASS":
            adapted["error"] = "RETRY_ADAPT_FAILED: " + tail(adapted.get("stderr", ""))
            return adapted
        hybrid = adapt_dir / "hybrid-result.json"
        if not hybrid.is_file():
            return {"status": "FAIL", "error": "RETRY_HYBRID_MISSING"}
        self.capture_dir = retry_capture
        self.manifest = manifest
        self.hybrid = hybrid
        self.artifacts["captureDirectory"] = str(retry_capture)
        self.artifacts["manifest"] = str(manifest)
        self.artifacts["hybrid"] = str(hybrid)
        font, source = resolve_font(
            retry_capture, self.font_family, self.args.font, DEFAULT_FONT
        )
        if font is not None:
            self.font = font
            self.artifacts["font"] = str(font)
        adapted["outputs"] = [str(hybrid)]
        adapted["notes"] = f"recovered=true; fontSource={source}"
        return adapted

    def compile_command(self, output, formal):
        command = [
            COMPILER,
            "--raw", self.hybrid,
            "--mapping", self.args.mapping,
            "--extensions", Path(self.args.mapping) / "extensions.json",
            "--font", self.font,
            "--output", output,
            "--tool-root", RUNTIME,
            "--work-root", PROJECT,
        ]
        if formal:
            command += ["--strict-layout", "--formal"]
        else:
            command.append("--layout-only")
        if self.args.library_profile:
            command += ["--library-profile", self.args.library_profile]
        return command

    def step_structure_compile(self):
        blocked = self.require(
            self.hybrid is not None
            and self.hybrid.is_file()
            and self.font is not None
            and COMPILER.is_file()
            and self.integrity_ok,
            "STRUCTURE_INPUTS_OR_TOOL_INTEGRITY_MISSING",
        )
        if blocked:
            return blocked
        self.structure_base = PROJECT / "output/work/structure-only" / self.batch / self.page
        self.structure_base.parent.mkdir(parents=True, exist_ok=True)
        outcome = self.run_command(self.compile_command(self.structure_base, False), timeout=300)
        ui_path = Path(str(self.structure_base) + ".ui")
        if outcome["status"] == "PASS":
            if not ui_path.is_file():
                outcome["status"] = "FAIL"
                outcome["error"] = "STRUCTURE_UI_MISSING"
                return outcome
            self.structure_ui = ui_path
            self.artifacts["structureUi"] = str(ui_path)
            outcome["outputs"] = [str(ui_path)]
        return outcome

    def step_uic_structure(self):
        blocked = self.require(
            self.structure_ui is not None and self.structure_ui.is_file() and UIC.is_file(),
            "STRUCTURE_UI_OR_UIC_MISSING",
        )
        if blocked:
            return blocked
        outcome = self.run_command([UIC, self.structure_ui], timeout=120, quiet_stdout=True)
        if outcome["status"] == "PASS":
            outcome["outputs"] = [str(self.structure_ui)]
        return outcome

    def step_strict_compile(self):
        blocked = self.require(
            self.hybrid is not None
            and self.hybrid.is_file()
            and self.font is not None
            and COMPILER.is_file()
            and self.integrity_ok,
            "STRICT_INPUTS_OR_TOOL_INTEGRITY_MISSING",
        )
        if blocked:
            return blocked
        self.delivery_page = PROJECT / "output/delivery" / self.batch / self.page
        if self.delivery_page.exists():
            return self.skip("FORMAL_OUTPUT_ALREADY_EXISTS")
        self.delivery_page.parent.mkdir(parents=True, exist_ok=True)
        outcome = self.run_command(self.compile_command(self.delivery_page, True), timeout=300)
        if outcome["status"] == "PASS":
            uis = sorted(self.delivery_page.rglob("*.ui"))
            if not uis:
                outcome["status"] = "FAIL"
                outcome["error"] = "STRICT_UI_MISSING"
                return outcome
            self.strict_ui = uis[0]
            self.artifacts["strictUi"] = str(self.strict_ui)
            outcome["outputs"] = [str(path) for path in uis]
            outcome["notes"] = f"uiCount={len(uis)}"
            return outcome
        parsed = parse_error_json(outcome.get("stderr", "") + "\n" + outcome.get("stdout", ""))
        if parsed:
            outcome["error"] = json.dumps(parsed, ensure_ascii=False)
        else:
            outcome["error"] = tail(outcome.get("stderr", "") or outcome.get("stdout", ""))
        return outcome

    def step_uic_strict(self):
        blocked = self.require(
            self.strict_ui is not None and self.strict_ui.is_file() and UIC.is_file(),
            "STRICT_UI_OR_UIC_MISSING",
        )
        if blocked:
            return blocked
        outcome = self.run_command([UIC, self.strict_ui], timeout=120, quiet_stdout=True)
        if outcome["status"] == "PASS":
            outcome["outputs"] = [str(self.strict_ui)]
        return outcome

    def step_acceptance(self):
        if not CHECK_DELIVERY.is_file():
            return self.skip("CHECK_DELIVERY_MISSING")
        outcome = self.run_command([
            sys.executable, "-B", CHECK_DELIVERY, "--formal-batch", self.batch,
        ], timeout=180)
        text = (outcome.get("stdout", "") + "\n" + outcome.get("stderr", "")).strip()
        if outcome["status"] == "PASS":
            return outcome
        if "THREE_COMPLETE_PAGES_REQUIRED" in text:
            outcome["status"] = "SKIP"
            outcome["error"] = "THREE_COMPLETE_PAGES_REQUIRED"
            outcome["notes"] = "single-page batch is outside the current checker contract"
            return outcome
        outcome["error"] = tail(text)
        return outcome

    def run(self):
        self.log(json.dumps({
            "batch": self.batch,
            "page": self.page,
            "url": self.args.url,
            "fileKey": self.target["fileKey"],
            "rootId": self.target["rootId"],
            "generationMode": self.args.generation_mode,
            "startedAtUtc": self.started_at,
        }, ensure_ascii=False))
        self.execute("PREFLIGHT_ENV", self.step_preflight)
        self.execute("TOOL_INTEGRITY", self.step_tool_integrity)
        self.execute("MCP_PROBE", self.step_mcp_probe)
        self.execute("DSL_CAPTURE", self.step_dsl_capture)
        self.execute("D2C_CAPTURE", self.step_d2c_capture)
        self.execute("LAYOUT_REFERENCE", self.step_layout_reference)
        self.execute("IMPORT_PAGE", self.step_import_page)
        self.execute("RESOURCE_SCAN", self.step_resource_scan)
        self.execute("IMPORT_FINAL", self.step_import_final)
        self.execute("MANIFEST_VERIFY", self.step_manifest_verify)
        self.execute("FONT_RESOLVE", self.step_font_resolve)
        self.execute("HTML_ADAPT", self.step_html_adapt)
        self.execute("HTML_ADAPT_RETRY", self.step_html_adapt_retry)
        self.execute("STRUCTURE_ONLY_COMPILE", self.step_structure_compile)
        self.execute("UIC_STRUCTURE", self.step_uic_structure)
        if self.args.generation_mode == "strict":
            self.execute("STRICT_COMPILE", self.step_strict_compile)
            self.execute("UIC_STRICT", self.step_uic_strict)
            self.execute("ACCEPTANCE", self.step_acceptance)
        else:
            self.execute("STRICT_COMPILE", lambda: self.not_requested("STRICT_NOT_REQUESTED"))
            self.execute("UIC_STRICT", lambda: self.not_requested("STRICT_NOT_REQUESTED"))
            self.execute("ACCEPTANCE", lambda: self.not_requested("STRICT_NOT_REQUESTED"))
        result = self.build_result()
        write_reports(self.run_dir, result)
        self.log_stream.close()
        print(json.dumps({
            "overallStatus": result["overallStatus"],
            "resultJson": str(self.run_dir / "result.json"),
            "report": str(self.run_dir / "REPORT.md"),
            "structureOnly": result["summary"]["structureOnly"],
            "strict": result["summary"]["strict"],
        }, ensure_ascii=False))
        if self.args.fail_on_error and result["overallStatus"] != "PASS":
            return 1
        return 0

    def build_result(self):
        steps = self.steps
        structure = find_step(steps, "STRUCTURE_ONLY_COMPILE")
        strict = find_step(steps, "STRICT_COMPILE")
        uic_structure = find_step(steps, "UIC_STRUCTURE")
        uic_strict = find_step(steps, "UIC_STRICT")
        acceptance = find_step(steps, "ACCEPTANCE")
        structure_status = structure["status"] if structure else "SKIP"
        strict_status = strict["status"] if strict else "NOT_RUN"
        if not self.preflight_ok or not self.integrity_ok or not self.mcp_ok:
            overall = "BLOCKED"
        elif self.args.generation_mode == "structure-only" and structure_status == "PASS":
            overall = "PASS"
        elif structure_status == "PASS" and strict_status == "PASS":
            overall = "PASS"
        elif structure_status == "PASS":
            overall = "PARTIAL"
        else:
            overall = "FAIL"
        finished_at = datetime.now(timezone.utc).isoformat()
        total_ms = int(round((time.perf_counter() - self.start) * 1000))
        return {
            "schema": "page-audit/v1",
            "batch": self.batch,
            "page": self.page,
            "url": self.args.url,
            "fileKey": self.target["fileKey"],
            "rootId": self.target["rootId"],
            "rootName": self.root_name,
            "generationMode": self.args.generation_mode,
            "startedAtUtc": self.started_at,
            "finishedAtUtc": finished_at,
            "totalElapsedMs": total_ms,
            "overallStatus": overall,
            "summary": {
                "structureOnly": {
                    "status": structure_status,
                    "ui": str(self.structure_ui) if self.structure_ui else "",
                    "uic": uic_structure["status"] if uic_structure else "SKIP",
                },
                "strict": {
                    "status": strict_status,
                    "ui": str(self.strict_ui) if self.strict_ui else "",
                    "uic": uic_strict["status"] if uic_strict else "SKIP",
                    "error": strict.get("error", "") if strict else "",
                },
                "acceptance": {
                    "status": acceptance["status"] if acceptance else "SKIP",
                    "error": acceptance.get("error", "") if acceptance else "",
                },
                "failedSteps": [step["name"] for step in steps if step["status"] == "FAIL"],
                "skippedSteps": [step["name"] for step in steps if step["status"] == "SKIP"],
                "notRunSteps": [step["name"] for step in steps if step["status"] == "NOT_RUN"],
            },
            "artifacts": self.artifacts,
            "steps": steps,
        }


def find_step(steps, name):
    for step in steps:
        if step["name"] == name:
            return step
    return None


def tail(text, limit=800):
    value = str(text or "").strip()
    if len(value) <= limit:
        return value
    return value[-limit:]


def parse_error_json(text):
    for line in reversed(str(text or "").splitlines()):
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict) and value.get("code"):
            return value
    return None


def last_json_line(text):
    for line in reversed(str(text or "").splitlines()):
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict):
            return value
    return None


def count_nodes(node):
    total = 1
    for child in node.get("childNode") or []:
        total += count_nodes(child)
    return total


def normalize_page_name(root_name, root_id):
    parts = re.findall(r"[A-Za-z0-9]+", root_name or "")
    if parts:
        name = parts[0][0].lower() + parts[0][1:]
        for part in parts[1:]:
            name += part[0].upper() + part[1:]
        if re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", name):
            return name[:80]
    fallback = re.sub(r"[^A-Za-z0-9]+", "_", root_id).strip("_")
    return ("page_" + fallback)[:80]


def normalize_batch(value):
    value = re.sub(r"[^A-Za-z0-9_-]+", "-", value or "").strip("-_")
    if not value:
        value = "audit-" + datetime.now().strftime("%Y%m%d-%H%M%S")
    if not value[0].isalnum():
        value = "audit-" + value
    return value[:80]


def unique_batch(value):
    base = normalize_batch(value)
    candidate = base
    for index in range(2, 100):
        work = PROJECT / "output/work/audit" / candidate
        logs = PROJECT / "output/logs/audit" / candidate
        delivery = PROJECT / "output/delivery" / candidate
        structure = PROJECT / "output/work/structure-only" / candidate
        if (
            not work.exists()
            and not logs.exists()
            and not delivery.exists()
            and not structure.exists()
        ):
            return candidate
        candidate = f"{base}-{index}"
    raise RuntimeError("BATCH_NAME_EXHAUSTED")


def parse_target(url, file_key, root_id):
    parsed = urlparse(url or "")
    if not file_key:
        match = re.search(r"/design/([^/?#]+)", parsed.path)
        if match:
            file_key = unquote(match.group(1))
    if not root_id:
        query = parse_qs(parsed.query)
        values = query.get("item-id") or query.get("itemId") or []
        if values:
            root_id = values[0]
    if not file_key:
        raise ValueError("FILE_KEY_REQUIRED: pass --file-key or a Pixso design URL")
    if not root_id:
        raise ValueError("ROOT_ID_REQUIRED: pass --root-id or a Pixso URL with item-id")
    return {"fileKey": file_key, "rootId": root_id}


def probe_mcp(endpoint, timeout=10):
    headers = {
        "Content-Type": "application/json",
        "Accept": "application/json, text/event-stream",
    }
    session = {"id": None}

    def post(payload, notification=False):
        request_headers = dict(headers)
        if session["id"]:
            request_headers["Mcp-Session-Id"] = session["id"]
        request = Request(
            endpoint, json.dumps(payload).encode("utf-8"), request_headers
        )
        with urlopen(request, timeout=timeout) as response:
            session["id"] = response.headers.get("Mcp-Session-Id") or session["id"]
            return response.read().decode("utf-8", "replace")

    def messages(text):
        if text.lstrip().startswith("{"):
            return [json.loads(text)]
        result = []
        for event in text.replace("\r\n", "\n").split("\n\n"):
            data = "\n".join(
                line[5:].lstrip(" ")
                for line in event.splitlines()
                if line.startswith("data:")
            )
            if data:
                result.append(json.loads(data))
        return result

    def call(payload, request_id, notification=False):
        text = post(payload, notification)
        if notification:
            return None
        for message in messages(text):
            if message.get("id") == request_id:
                if "error" in message:
                    raise RuntimeError(message["error"])
                return message["result"]
        raise RuntimeError("MCP_RESPONSE_MISSING")

    call({
        "jsonrpc": "2.0", "id": 1, "method": "initialize",
        "params": {
            "protocolVersion": "2024-11-05", "capabilities": {},
            "clientInfo": {"name": "page-audit", "version": "1"},
        },
    }, 1)
    call({
        "jsonrpc": "2.0", "method": "notifications/initialized", "params": {},
    }, None, notification=True)
    result = call({
        "jsonrpc": "2.0", "id": 2, "method": "tools/list", "params": {},
    }, 2)
    return [tool.get("name") for tool in result.get("tools", [])]


def scan_resources(html_path, output_path):
    html = Path(html_path).read_text(encoding="utf-8")
    urls = []
    for match in URL_PATTERN.finditer(html):
        url = match.group(0).rstrip(");,")
        parsed = urlparse(url)
        if parsed.hostname not in LOCAL_HOSTS:
            continue
        if url not in urls:
            urls.append(url)
    resources = []
    used = set()
    for url in urls:
        basename = url.split("?")[0].rstrip("/").split("/")[-1] or "asset"
        basename = re.sub(r"[^A-Za-z0-9._-]+", "_", basename)
        name = basename
        index = 2
        while name in used:
            name = f"{index}-{basename}"
            index += 1
        used.add(name)
        resources.append({"url": url, "local": "assets/" + name})
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(resources, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    return resources


def detect_font_family(html):
    values = re.findall(r"font-family\s*:\s*([^;}{]+)", html, flags=re.IGNORECASE)
    counts = {}
    for value in values:
        for family in value.split(","):
            family = family.strip().strip("'\"").strip()
            if not family or family.lower() in GENERIC_FONTS:
                continue
            if family.lower().startswith(("var(", "inherit", "initial", "unset")):
                continue
            counts[family] = counts.get(family, 0) + 1
            break
    if not counts:
        return ""
    return max(counts, key=counts.get)


def resolve_font(capture_dir, family, explicit, default):
    if explicit:
        path = Path(explicit)
        if path.is_file():
            return path, "explicit"
    assets = Path(capture_dir) / "assets"
    candidates = []
    if assets.is_dir():
        candidates = sorted(
            (
                path for path in assets.iterdir()
                if path.suffix.lower() in (".ttf", ".otf") and path.is_file()
            ),
            key=lambda path: (
                0 if "regular" in path.stem.lower() else 1,
                path.name.lower(),
            ),
        )
    target = re.sub(r"[^a-z0-9]", "", (family or "").lower())
    if target:
        for path in candidates:
            stem = re.sub(r"[^a-z0-9]", "", path.stem.lower())
            if target in stem or stem in target:
                return path, "asset-match"
    if candidates:
        return candidates[0], "first-asset"
    default_path = Path(default)
    if default_path.is_file():
        return default_path, "default"
    return None, "missing"


def write_reports(run_dir, result):
    result_path = run_dir / "result.json"
    report_path = run_dir / "REPORT.md"
    result.setdefault("artifacts", {})["resultJson"] = str(result_path)
    result["artifacts"]["reportMarkdown"] = str(report_path)
    result["artifacts"]["runLog"] = str(PROJECT / "output/logs/audit" / result["batch"] / "run.log")
    result_path.write_text(
        json.dumps(result, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    strict_error = (result["summary"]["strict"]["error"] or "无").replace("\n", " ")
    if len(strict_error) > 300:
        strict_error = strict_error[:300] + "..."
    acceptance_error = (result["summary"]["acceptance"]["error"] or "无").replace("\n", " ")
    lines = [
        "# 页面生成运行结果",
        "",
        f"- 批次：`{result['batch']}`",
        f"- 页面：`{result['page']}`",
        f"- 根节点：`{result['rootId']}`",
        f"- Pixso 文件：`{result['fileKey']}`",
        f"- 总耗时：{result['totalElapsedMs']} ms",
        f"- 总体状态：**{result['overallStatus']}**",
        "",
        "## 结果摘要",
        "",
        f"- structure-only：**{result['summary']['structureOnly']['status']}**",
        f"  - `.ui`：`{result['summary']['structureOnly']['ui'] or '未生成'}`",
        f"  - uic：{result['summary']['structureOnly']['uic']}",
        f"- strict：**{result['summary']['strict']['status']}**",
        f"  - `.ui`：`{result['summary']['strict']['ui'] or '未生成'}`",
        f"  - 错误：{strict_error}",
        f"- 验收：**{result['summary']['acceptance']['status']}**",
        f"  - 说明：{acceptance_error}",
        f"- 失败步骤：{', '.join(result['summary']['failedSteps']) or '无'}",
        f"- 跳过步骤：{', '.join(result['summary']['skippedSteps']) or '无'}",
        "",
        "## 步骤",
        "",
        "| 步骤 | 状态 | 耗时 ms | 错误/说明 |",
        "| --- | --- | ---: | --- |",
    ]
    for step in result["steps"]:
        detail = (step.get("error") or step.get("notes") or "").replace("\n", " ")
        detail = detail.replace("|", "\\|")
        if len(detail) > 160:
            detail = detail[:160] + "..."
        lines.append(
            f"| {step['name']} | {step['status']} | {step['elapsedMs']} | {detail} |"
        )
    lines += [
        "",
        "## 产物",
        "",
        f"- 运行目录：`{run_dir}`",
        f"- 机器可读结果：`{result_path}`",
        f"- 原始日志：`{PROJECT / 'output/logs/audit' / result['batch'] / 'run.log'}`",
        "",
        "## 说明",
        "",
        "- 本结果允许部分成功；structure-only 成功不等于 strict 正式交付成功。",
        "- `uic` 通过只证明 `.ui` XML 可解析，不代表消费者编译、运行或视觉验收通过。",
        "- strict 失败不会删除已生成的 structure-only 产物。",
    ]
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(
        description="Run the HTML+DSL layout pipeline in a fixed order, continue on step failures, and write result.json."
    )
    parser.add_argument("--url", default="", help="Pixso design URL")
    parser.add_argument("--batch", default="", help="batch name; default is timestamped")
    parser.add_argument("--page", default="", help="page name; default derives from root name")
    parser.add_argument("--root-id", default="", help="Pixso root node id, e.g. 2:23506")
    parser.add_argument("--file-key", default="", help="Pixso file key")
    parser.add_argument("--page-id", default="", help="D2C page entry id when multiple pages exist")
    parser.add_argument("--mapping", default=str(DEFAULT_MAPPING), help="mapping directory")
    parser.add_argument("--font", default="", help="font file for Qt output")
    parser.add_argument("--font-family", default="", help="font family used by the capture manifest")
    parser.add_argument("--library-profile", default="token-sdk", help="library profile")
    parser.add_argument("--endpoint", default="http://127.0.0.1:3667/mcp", help="MCP endpoint")
    parser.add_argument(
        "--generation-mode", choices=("structure-only", "strict"), default="structure-only",
        help="default structure-only emits a default-appearance layout; strict additionally validates formal component contracts",
    )
    parser.add_argument("--fail-on-error", action="store_true", help="return non-zero when the final status is not PASS")
    args = parser.parse_args()

    if sys.version_info[:2] != (3, 9):
        print("PYTHON39_REQUIRED", file=sys.stderr)
        return 2
    args.url = args.url.strip()
    args.page = args.page.strip()
    args.batch = args.batch.strip()
    args.mapping = str(Path(args.mapping).resolve())
    if args.font:
        args.font = str(Path(args.font).resolve())
    try:
        target = parse_target(args.url, args.file_key, args.root_id)
    except ValueError as exc:
        parser.error(str(exc))
    args.batch = unique_batch(args.batch)
    if args.page and not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", args.page):
        parser.error("INVALID_PAGE_NAME")
    target["batch"] = args.batch
    target["page"] = args.page or normalize_page_name("", target["rootId"])
    runner = Runner(args, target)
    return runner.run()


if __name__ == "__main__":
    sys.exit(main())
