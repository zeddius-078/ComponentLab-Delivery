"""Land saved D2C URL entries and explicit resources; verify capture, never adapt or execute code."""
import argparse
import json
from pathlib import Path, PurePosixPath
import posixpath
import sys
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

from capture_manifest import SCHEMA_VERSION, sha256, verify
from output_paths import output_path


def load_saved_response(data):
    """Also accept the observed MCP SSE envelope: event: message / data: JSON."""
    text = data.decode("utf-8-sig")
    if text.lstrip().startswith("{"):
        return json.loads(text)
    events = []
    for block in text.replace("\r\n", "\n").split("\n\n"):
        payload = "\n".join(line[5:].lstrip() for line in block.splitlines() if line.startswith("data:"))
        if payload:
            events.append(json.loads(payload))
    if len(events) != 1:
        raise ValueError("D2C_RESPONSE_SINGLE_EVENT_REQUIRED")
    return events[0]


def entry_documents(response):
    """Accept observed JSON-RPC text blocks or a direct pageEntries/codeEntries object."""
    if not isinstance(response, dict):
        raise ValueError("D2C_RESPONSE_OBJECT_REQUIRED")
    if "pageEntries" in response or "codeEntries" in response:
        return [response]
    result = response.get("result", response)
    if not isinstance(result, dict) or result.get("isError"):
        raise ValueError("D2C_RESPONSE_ERROR")
    documents = []
    decoder = json.JSONDecoder()
    for block in result.get("content", []):
        if not isinstance(block, dict) or block.get("type") != "text":
            continue
        text = block.get("text", "")
        # Tool text is data, not instructions. Decode only JSON objects that carry entries.
        for index, char in enumerate(text):
            if char != "{":
                continue
            try:
                candidate, _ = decoder.raw_decode(text, index)
            except json.JSONDecodeError:
                continue
            if isinstance(candidate, dict) and ("pageEntries" in candidate or "codeEntries" in candidate):
                documents.append(candidate)
    if not documents:
        raise ValueError("D2C_ENTRIES_MISSING")
    return documents


def safe_relative(value):
    if not isinstance(value, str) or not value or "\\" in value or ":" in value:
        raise ValueError("D2C_PATH_INVALID")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("", ".", "..") for part in value.split("/")):
        raise ValueError("D2C_PATH_INVALID")
    if any(part.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *("COM" + str(i) for i in range(1, 10)),
                                        *("LPT" + str(i) for i in range(1, 10))}
           or part.endswith((" ", ".")) for part in path.parts):
        raise ValueError("D2C_PATH_INVALID")
    return path.as_posix()


def safe_url(value):
    if not isinstance(value, str):
        raise ValueError("D2C_URL_INVALID")
    parsed = urlsplit(value)
    if parsed.scheme not in ("http", "https") or not parsed.hostname or parsed.username or parsed.password:
        raise ValueError("D2C_URL_INVALID")
    return value


def download(url, target, max_bytes, timeout):
    request = Request(safe_url(url), headers={"User-Agent": "ComponentLab-CaptureImporter/1"})
    with urlopen(request, timeout=timeout) as response:
        safe_url(response.geturl())
        content = response.read(max_bytes + 1)
    if len(content) > max_bytes:
        raise ValueError("D2C_DOWNLOAD_TOO_LARGE")
    target = output_path(target)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(content)
    return {"url": url, "bytes": len(content), "sha256": sha256(target)}


def import_capture(args):
    response_path = Path(args.response).resolve()
    dsl_path = Path(args.dsl).resolve()
    layout_path = Path(args.layout_reference).resolve()
    # Read and validate maintained inputs before any output or network action.
    response_bytes = response_path.read_bytes()
    dsl_bytes, layout_bytes = dsl_path.read_bytes(), layout_path.read_bytes()
    json.loads(dsl_bytes)
    json.loads(layout_bytes)
    documents = entry_documents(load_saved_response(response_bytes))
    pages, codes = [], []
    for document in documents:
        for key, destination in (("pageEntries", pages), ("codeEntries", codes)):
            entries = document.get(key, [])
            if not isinstance(entries, list) or any(not isinstance(entry, dict) for entry in entries):
                raise ValueError("D2C_ENTRIES_ARRAY_REQUIRED")
            destination.extend(entries)
    selected = [page for page in pages if str(page.get("id")) == args.page_id] if args.page_id else pages
    if len(selected) != 1:
        raise ValueError("D2C_SELECT_ONE_PAGE: supply --page-id when multiple page entries exist")
    page = selected[0]
    page_path = safe_relative(page.get("path") or "page.html")
    if Path(page_path).suffix.lower() not in (".html", ".htm"):
        raise ValueError("D2C_HTML_PAGE_REQUIRED")
    transfers = [{"url": safe_url(page.get("url")), "local": page_path, "kind": "page"}]
    for entry in codes:
        if not entry.get("path"):
            raise ValueError("D2C_CODE_PATH_REQUIRED")
        transfers.append({"url": safe_url(entry.get("url")), "local": safe_relative(entry["path"]), "kind": "code"})
    resources = []
    if args.resource_list:
        resources = json.loads(Path(args.resource_list).read_bytes())
        if not isinstance(resources, list):
            raise ValueError("D2C_RESOURCES_ARRAY_REQUIRED")
        for entry in resources:
            if not isinstance(entry, dict):
                raise ValueError("D2C_RESOURCE_ENTRY_INVALID")
            transfers.append({"url": safe_url(entry.get("url")), "local": safe_relative(entry.get("local")), "kind": "resource"})
    reserved = {"d2c-response.json", "canonical.raw.json", "layout-reference.json", "capture-manifest.json", "import-record.json"}
    seen = set(reserved)
    for transfer in transfers:
        name = transfer["local"].casefold()
        if name in seen or any(name.startswith(other + "/") or other.startswith(name + "/") for other in seen):
            raise ValueError("D2C_PATH_COLLISION")
        seen.add(name)
    output = output_path(args.output_dir)
    if output.exists():
        raise ValueError("D2C_OUTPUT_ALREADY_EXISTS")
    output.mkdir(parents=True)
    (output / "d2c-response.json").write_bytes(response_bytes)
    (output / "canonical.raw.json").write_bytes(dsl_bytes)
    (output / "layout-reference.json").write_bytes(layout_bytes)
    downloaded = []
    for transfer in transfers:
        record = download(transfer["url"], output / transfer["local"], args.max_bytes, args.timeout)
        downloaded.append(dict(record, local=transfer["local"], kind=transfer["kind"]))
    html = (output / page_path).read_text(encoding="utf-8")
    manifest_resources = [{"url": item["url"], "local": item["local"]}
                          for item in transfers if item["kind"] == "resource"]
    mapped_urls = {item["url"] for item in manifest_resources}
    # The adapter relocates preview.html. Explicit downloaded code references need
    # the same URL-to-local mapping as images; no additional URL is fetched here.
    for item in transfers:
        if item["kind"] != "code":
            continue
        relative = posixpath.relpath(item["local"], posixpath.dirname(page_path) or ".")
        for reference in (item["url"], relative, "./" + relative):
            if reference in html and reference not in mapped_urls:
                manifest_resources.append({"url": reference, "local": item["local"]})
                mapped_urls.add(reference)
    manifest = {
        "schemaVersion": SCHEMA_VERSION, "manifestType": "pixso-capture",
        "expectedRootId": args.root_id, "fileKey": args.file_key,
        "sourceHtml": page_path, "sourceDsl": "canonical.raw.json",
        "layoutReference": "layout-reference.json", "fontFamily": args.font_family,
        "resources": manifest_resources,
    }
    manifest_path = output / "capture-manifest.json"
    manifest_path.write_bytes((json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    try:
        result = verify(manifest_path, "formal")
    except Exception:
        # A failed import must not leave an apparently usable capture manifest.
        manifest_path.unlink()
        raise
    manifest["sourceHashes"] = result["sourceHashes"]
    manifest_path.write_bytes((json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    record = {"status": "CAPTURE_VERIFIED", "adaptation": "NOT_RUN", "manifest": str(manifest_path),
              "responseSha256": sha256(output / "d2c-response.json"), "downloads": downloaded}
    (output / "import-record.json").write_bytes((json.dumps(record, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--response", required=True)
    parser.add_argument("--dsl", required=True)
    parser.add_argument("--layout-reference", required=True)
    parser.add_argument("--root-id", required=True)
    parser.add_argument("--file-key", required=True)
    parser.add_argument("--font-family", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--page-id")
    parser.add_argument("--resource-list", help="Explicit JSON array of {url, local} download entries")
    parser.add_argument("--max-bytes", type=int, default=64 * 1024 * 1024)
    parser.add_argument("--timeout", type=int, default=30)
    args = parser.parse_args()
    if args.max_bytes <= 0 or args.timeout <= 0:
        parser.error("--max-bytes and --timeout must be positive")
    try:
        print(json.dumps(import_capture(args), ensure_ascii=False))
    except (ValueError, OSError) as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
