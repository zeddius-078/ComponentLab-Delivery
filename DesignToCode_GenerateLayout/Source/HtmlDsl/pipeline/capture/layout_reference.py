"""Produce layout-reference.json from the existing read-only scene probe.

Alignment with the documented conversion rule (SKILL §5 / tutorial §2):
the probe ``capture_structure.js`` reads Pixso design properties; this tool
matches every DSL FRAME by GUID against that snapshot and writes the formal
layout evidence consumed by ``capture_manifest.py verify --mode formal``.

Mapping: gap <- itemSpacing, left/top/right/bottom <- paddingLeft/Top/Right/Bottom
(design pixels, not coordinates). Every value must be a finite number returned
by Pixso; explicit 0 is kept, a missing value stops the page (no silent zeros).
The probe locks the file via ``pixso.fileKey``; a mismatch stops the capture.

Command (from DesignToCode_GenerateLayout):
  py -3.9 -B Source/HtmlDsl/pipeline/capture/layout_reference.py \
      --guid '<root node id>' --file-key '<Pixso file id>' \
      --dsl output/work/captures/<name>/dsl/canonical.raw.json \
      --output output/work/captures/<name>/layout

The raw 01..04 MCP request/response files are kept next to the evidence file.
"""
import argparse
import json
import math
import sys
from pathlib import Path

from capture_mcp import Recorder
from output_paths import output_path

PROBE_PATH = Path(__file__).with_name("capture_structure.js")
METRIC_KEYS = ("gap", "left", "top", "right", "bottom")


def build_script(guid, file_key):
    """Fill the probe placeholders the same way the documented command does."""
    script = PROBE_PATH.read_text(encoding="utf-8")
    return script.replace("__FILE_KEY__", json.dumps(file_key)) \
                 .replace("__ROOT_ID__", json.dumps(guid))


def extract_snapshot(result):
    """Return the verbatim text of the single JSON content block."""
    if result.get("isError"):
        raise RuntimeError("MCP eval_script failed; original response retained")
    blocks = [block for block in result.get("content", []) if block.get("type") == "text"]
    if len(blocks) != 1:
        raise RuntimeError(f"LAYOUT_PROBE_SINGLE_BLOCK_REQUIRED: got {len(blocks)}")
    text = blocks[0].get("text", "")
    value = json.loads(text)
    if not isinstance(value, dict):
        raise RuntimeError("LAYOUT_PROBE_OBJECT_REQUIRED")
    if value.get("schema") != "design-structure/v1":
        raise RuntimeError("LAYOUT_PROBE_SCHEMA_REQUIRED")
    return text, value


def frame_guids(node, result):
    """Collect every FRAME GUID from the DSL tree in document order."""
    if node.get("type") == "FRAME":
        result.append(node.get("guid"))
    for child in node.get("childNode", []) or []:
        frame_guids(child, result)
    return result


def index_snapshot(node, index):
    """Index the probe tree by id; duplicate ids are ambiguous and rejected."""
    node_id = node.get("id")
    if node_id in index:
        raise RuntimeError("LAYOUT_PROBE_DUPLICATE_NODE: " + str(node_id))
    index[node_id] = node
    for child in node.get("children", []) or []:
        index_snapshot(child, index)


def finite_number(value, guid, key):
    """Accept only finite JSON numbers; booleans and strings are not evidence."""
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise RuntimeError(f"LAYOUT_REFERENCE_METRIC_NOT_NUMERIC: {guid}/{key}")
    return value


def convert(snapshot, dsl, guid, file_key, extra_metrics=None):
    """Build the formal layout-reference document from probe + DSL."""
    if snapshot.get("fileKey") != file_key:
        raise RuntimeError(
            f"LAYOUT_REFERENCE_WRONG_FILE: {snapshot.get('fileKey')} != {file_key}")
    roots = dsl.get("pixTreeNodes") or []
    if len(roots) != 1 or roots[0].get("guid") != guid:
        raise RuntimeError("LAYOUT_REFERENCE_DSL_ROOT_MISMATCH")
    index = {}
    index_snapshot(snapshot.get("root") or {}, index)
    nodes = []
    for frame in frame_guids(roots[0], []):
        probe = index.get(frame)
        if probe is None:
            # 实例展开后的内部节点不会出现在场景探针的 children 中；
            # 由第二次 eval_script 按 GUID 读取（见 fetch_expanded_frames）。
            metrics = (extra_metrics or {}).get(frame)
            if metrics is None:
                raise RuntimeError("LAYOUT_REFERENCE_FRAME_MISSING: " + str(frame))
            nodes.append({"id": frame, "metrics": metrics})
            continue
        metrics = {
            "gap": finite_number(probe.get("itemSpacing"), frame, "gap"),
            "left": finite_number(probe.get("paddingLeft"), frame, "left"),
            "top": finite_number(probe.get("paddingTop"), frame, "top"),
            "right": finite_number(probe.get("paddingRight"), frame, "right"),
            "bottom": finite_number(probe.get("paddingBottom"), frame, "bottom"),
        }
        nodes.append({"id": frame, "metrics": metrics})
    if not nodes:
        raise RuntimeError("LAYOUT_REFERENCE_FRAMES_REQUIRED")
    return {
        "fileKey": file_key,
        "evidenceType": "pixso-plugin-layout-read",
        "source": "pixso.eval_script capture_structure.js + getNodeById for expanded instance internals",
        "pages": [{"id": guid, "nodes": nodes}],
    }


EXPANDED_METRICS_SCRIPT = r"""
const ids = __IDS__;
const num = value => (typeof value === "number" && isFinite(value)) ? value : null;
return ids.map(id => {
  const node = pixso.getNodeById(id);
  if (!node) return { id: id, found: false };
  return { id: id, found: true, gap: num(node.itemSpacing), left: num(node.paddingLeft),
           top: num(node.paddingTop), right: num(node.paddingRight), bottom: num(node.paddingBottom) };
});
"""


def fetch_expanded_frames(recorder, frames):
    """Read layout metrics for expanded instance-internal frames by GUID."""
    script = EXPANDED_METRICS_SCRIPT.replace("__IDS__", json.dumps(frames))
    result = recorder.rpc("05-call", "tools/call", {
        "name": "eval_script", "arguments": {"script": script}})
    if result.get("isError"):
        raise RuntimeError("LAYOUT_REFERENCE_EXPANDED_READ_FAILED")
    blocks = [block for block in result.get("content", []) if block.get("type") == "text"]
    if len(blocks) != 1:
        raise RuntimeError("LAYOUT_REFERENCE_EXPANDED_BLOCK_REQUIRED")
    rows = json.loads(blocks[0]["text"])
    metrics = {}
    for row in rows:
        frame = row.get("id")
        if not row.get("found"):
            raise RuntimeError("LAYOUT_REFERENCE_FRAME_MISSING: " + str(frame))
        metrics[frame] = {
            key: finite_number(row.get(key), frame, key)
            for key in ("gap", "left", "top", "right", "bottom")
        }
    return metrics


def capture(args):
    script = build_script(args.guid, args.file_key)
    dsl_path = Path(args.dsl).resolve()
    dsl = json.loads(dsl_path.read_bytes())
    output = output_path(args.output)
    # 每次采集使用新目录，已有目录拒绝覆盖（与 capture_mcp.py 一致）。
    output.mkdir(parents=True, exist_ok=False)
    recorder = Recorder(args.endpoint, output)
    recorder.rpc("01-initialize", "initialize", {
        "protocolVersion": "2024-11-05", "capabilities": {},
        "clientInfo": {"name": "layout-reference-recorder", "version": "1"}})
    recorder.rpc("02-initialized", "notifications/initialized", {}, notification=True)
    tools = recorder.rpc("03-tools", "tools/list", {})
    if not any(tool.get("name") == "eval_script" for tool in tools.get("tools", [])):
        raise RuntimeError("eval_script tool unavailable on the capture endpoint")
    result = recorder.rpc("04-call", "tools/call", {
        "name": "eval_script", "arguments": {"script": script}})
    _text, snapshot = extract_snapshot(result)
    roots = dsl.get("pixTreeNodes") or []
    frames = frame_guids(roots[0], []) if roots else []
    index = {}
    if snapshot.get("root"):
        index_snapshot(snapshot["root"], index)
    missing = [frame for frame in frames if frame not in index]
    extra_metrics = fetch_expanded_frames(recorder, missing) if missing else {}
    evidence = convert(snapshot, dsl, args.guid, args.file_key, extra_metrics)
    (output / "layout-reference.json").write_text(
        json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "status": "PASS",
        "fileKey": evidence["fileKey"],
        "rootId": args.guid,
        "frames": len(evidence["pages"][0]["nodes"]),
        "output": str((output / "layout-reference.json").resolve()),
    }, ensure_ascii=False))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--guid", required=True, help="root design node id, e.g. 2:26696")
    parser.add_argument("--file-key", required=True, help="Pixso file id (also locks the probe)")
    parser.add_argument("--dsl", required=True, help="canonical.raw.json captured in step 1")
    parser.add_argument("--endpoint", default="http://127.0.0.1:3667/mcp")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        capture(args)
    except Exception as error:  # keep raw recorder files for diagnosis
        print(json.dumps({"status": "FAIL", "error": str(error)}, ensure_ascii=False), file=sys.stderr)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
