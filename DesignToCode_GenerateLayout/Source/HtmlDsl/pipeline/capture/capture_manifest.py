"""Create and verify the HTML + DSL capture manifest used by the layout pipeline."""

# 清单是跨采集工具的输入合同：它只引用已经落地的 HTML、DSL、布局证据和资源，
# 不负责连接 Pixso，也不负责生成页面。正式生成与预览生成都必须先通过本文件的校验。
import argparse
import hashlib
import json
import math
import os
from datetime import datetime, timezone
from pathlib import Path


SCHEMA_VERSION = "capture-manifest.v1"
METRIC_KEYS = ("gap", "left", "top", "right", "bottom")


def sha256(path):
    """按字节计算文件哈希，避免文本换行或 JSON 重排被忽略。"""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def resolve(manifest_path, value):
    """相对路径以清单所在目录为基准，绝对路径保持原样。"""
    path = Path(value)
    return path.resolve() if path.is_absolute() else (manifest_path.parent / path).resolve()


def fail(message):
    raise ValueError(message)


def load_json(path, label):
    if not path.is_file():
        fail(f"CAPTURE_FILE_MISSING: {label}: {path}")
    try:
        return json.loads(path.read_bytes())
    except json.JSONDecodeError as error:
        fail(f"CAPTURE_JSON_INVALID: {label}: {error}")


def frame_nodes(node):
    """返回 DSL 中需要布局证据的 FRAME 节点。"""
    result = []
    if node.get("type") == "FRAME":
        result.append(node)
    for child in node.get("childNode", []) or []:
        result.extend(frame_nodes(child))
    return result


def verify(manifest_path, mode="formal"):
    """验证清单及其引用文件，返回可写入报告的摘要。"""
    manifest_path = Path(manifest_path).resolve()
    manifest = load_json(manifest_path, "capture manifest")
    if manifest.get("schemaVersion") != SCHEMA_VERSION:
        fail("CAPTURE_SCHEMA_UNSUPPORTED")
    if mode not in ("formal", "preview"):
        fail("CAPTURE_MODE_UNSUPPORTED")
    if mode == "preview" and manifest.get("manifestType") != "preview-fixture":
        fail("PREVIEW_MANIFEST_TYPE_REQUIRED")
    if mode == "formal" and manifest.get("manifestType") == "preview-fixture":
        fail("FORMAL_CANNOT_USE_PREVIEW_FIXTURE")

    required = ("manifestType", "sourceHtml", "sourceDsl", "expectedRootId", "fileKey", "fontFamily", "resources")
    for field in required:
        if field not in manifest or manifest[field] in (None, ""):
            fail(f"CAPTURE_FIELD_REQUIRED: {field}")
    if not isinstance(manifest["resources"], list):
        fail("CAPTURE_RESOURCES_MUST_BE_ARRAY")

    html_path = resolve(manifest_path, manifest["sourceHtml"])
    dsl_path = resolve(manifest_path, manifest["sourceDsl"])
    if not html_path.is_file():
        fail(f"CAPTURE_FILE_MISSING: sourceHtml: {html_path}")
    raw = load_json(dsl_path, "sourceDsl")
    roots = raw.get("pixTreeNodes")
    if not isinstance(roots, list) or len(roots) != 1:
        fail("CAPTURE_DSL_EXPECTS_ONE_ROOT")
    root = roots[0]
    if root.get("guid") != manifest["expectedRootId"]:
        fail("CAPTURE_ROOT_MISMATCH")

    html = html_path.read_text(encoding="utf-8")
    resources = []
    seen_urls = set()
    for resource in manifest["resources"]:
        if not isinstance(resource, dict) or not resource.get("url") or not resource.get("local"):
            fail("CAPTURE_RESOURCE_ENTRY_INVALID")
        url = resource["url"]
        if url in seen_urls:
            fail(f"CAPTURE_RESOURCE_DUPLICATE: {url}")
        seen_urls.add(url)
        if url not in html:
            fail(f"CAPTURE_RESOURCE_NOT_IN_HTML: {url}")
        local = resolve(manifest_path, resource["local"])
        if not local.is_file():
            fail(f"CAPTURE_RESOURCE_MISSING: {local}")
        resources.append({"url": url, "local": str(local), "sha256": sha256(local)})

    layout_path = None
    layout = None
    if manifest.get("layoutReference"):
        layout_path = resolve(manifest_path, manifest["layoutReference"])
        layout = load_json(layout_path, "layoutReference")
    elif mode == "formal":
        fail("CAPTURE_LAYOUT_REFERENCE_REQUIRED")

    if layout is not None:
        if layout.get("fileKey") != manifest["fileKey"]:
            fail("LAYOUT_REFERENCE_FILE_MISMATCH")
        pages = layout.get("pages")
        if not isinstance(pages, list):
            fail("LAYOUT_REFERENCE_PAGES_REQUIRED")
        page = next((item for item in pages if item.get("id") == manifest["expectedRootId"]), None)
        if page is None or not isinstance(page.get("nodes"), list):
            fail("LAYOUT_REFERENCE_ROOT_REQUIRED")
        evidence = {item.get("id"): item for item in page["nodes"]}
        for frame in frame_nodes(root):
            item = evidence.get(frame.get("guid"))
            if item is None or not isinstance(item.get("metrics"), dict):
                fail(f"LAYOUT_REFERENCE_NODE_REQUIRED: {frame.get('guid')}")
            for key in METRIC_KEYS:
                value = item["metrics"].get(key)
                if not isinstance(value, (int, float)) or isinstance(value, bool) or not math.isfinite(value):
                    fail(f"LAYOUT_REFERENCE_METRIC_INVALID: {frame.get('guid')}/{key}")

    expected_hashes = manifest.get("sourceHashes", {})
    actual_hashes = {"htmlSha256": sha256(html_path), "dslSha256": sha256(dsl_path)}
    if layout_path:
        actual_hashes["layoutReferenceSha256"] = sha256(layout_path)
    for key, expected in expected_hashes.items():
        if actual_hashes.get(key) != expected:
            fail(f"CAPTURE_HASH_MISMATCH: {key}")

    return {
        "status": "PASS",
        "manifest": str(manifest_path),
        "mode": mode,
        "manifestType": manifest["manifestType"],
        "rootId": manifest["expectedRootId"],
        "fileKey": manifest["fileKey"],
        "html": str(html_path),
        "dsl": str(dsl_path),
        "layoutReference": str(layout_path) if layout_path else None,
        "resources": resources,
        "sourceHashes": actual_hashes,
    }


def relative_path(target, base):
    """写入可迁移的相对路径，不把当前机器盘符固化进清单。"""
    return os.path.relpath(Path(target).resolve(), Path(base).resolve()).replace(os.sep, "/")


def create(args):
    from output_paths import output_path
    output = output_path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    html = Path(args.html).resolve()
    dsl = Path(args.dsl).resolve()
    resources = []
    for value in args.resource:
        if "=" not in value:
            fail("RESOURCE_FORMAT_MUST_BE_URL_EQUALS_LOCAL_PATH")
        url, local = value.split("=", 1)
        resources.append({"url": url, "local": relative_path(local, output.parent)})
    manifest = {
        "schemaVersion": SCHEMA_VERSION,
        "manifestType": args.manifest_type,
        "generatedAtUtc": datetime.now(timezone.utc).isoformat(),
        "expectedRootId": args.root_id,
        "fileKey": args.file_key,
        "sourceHtml": relative_path(html, output.parent),
        "sourceDsl": relative_path(dsl, output.parent),
        "fontFamily": args.font_family,
        "resources": resources,
    }
    if args.layout_reference:
        manifest["layoutReference"] = relative_path(args.layout_reference, output.parent)
    output.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    # 创建后立即复用同一校验器，避免产生一个看似完整但不能消费的清单。
    result = verify(output, "preview" if args.manifest_type == "preview-fixture" else "formal")
    manifest["sourceHashes"] = result["sourceHashes"]
    output.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "PASS", "manifest": str(output)}, ensure_ascii=False))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    verify_parser = subparsers.add_parser("verify")
    verify_parser.add_argument("--manifest", required=True, type=Path)
    verify_parser.add_argument("--mode", choices=("formal", "preview"), default="formal")
    create_parser = subparsers.add_parser("create")
    create_parser.add_argument("--output", required=True, type=Path)
    create_parser.add_argument("--html", required=True, type=Path)
    create_parser.add_argument("--dsl", required=True, type=Path)
    create_parser.add_argument("--root-id", required=True)
    create_parser.add_argument("--file-key", required=True)
    create_parser.add_argument("--font-family", required=True)
    create_parser.add_argument("--resource", action="append", default=[], help="URL=local path")
    create_parser.add_argument("--layout-reference", type=Path)
    create_parser.add_argument("--manifest-type", choices=("pixso-capture", "preview-fixture"), default="pixso-capture")
    args = parser.parse_args()
    if args.command == "verify":
        print(json.dumps(verify(args.manifest, args.mode), ensure_ascii=False))
    else:
        create(args)


if __name__ == "__main__":
    main()
