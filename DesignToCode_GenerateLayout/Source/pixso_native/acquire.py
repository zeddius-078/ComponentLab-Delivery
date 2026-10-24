"""Preserve native Vue exports without invoking the HTML/DSL route (Python 3.9)."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import sys
import urllib.parse
import urllib.request
import zipfile


MAX_FILE = 64 * 1024 * 1024
MAX_TOTAL = 256 * 1024 * 1024


def digest(data):
    return hashlib.sha256(data).hexdigest()


def relative_name(value):
    """ZIP and D2C names are untrusted, including on case-insensitive Windows."""
    path = PurePosixPath(value)
    if (not value or "\\" in value or path.is_absolute()
            or any(part in ("", ".", "..") or ":" in part
                   or part.rstrip(" .") != part for part in value.split("/"))):
        raise ValueError("UNSAFE_INPUT_PATH: " + value)
    reserved = {"CON", "PRN", "AUX", "NUL", *(f"COM{i}" for i in range(1, 10)),
                *(f"LPT{i}" for i in range(1, 10))}
    if any(p.split(".")[0].upper() in reserved for p in path.parts):
        raise ValueError("RESERVED_INPUT_PATH: " + value)
    return path


def checked_work_path(path):
    if ".." in path.parts:
        raise ValueError("OUTPUT_TRAVERSAL_REFUSED")
    path = path.absolute()
    # Win32 strips trailing dots/spaces and interprets colons as streams. Reject
    # ambiguous segments before checking their apparent directory names.
    if any(part.rstrip(" .") != part or ":" in part for part in path.parts[1:]):
        raise ValueError("OUTPUT_AMBIGUOUS_PATH")
    source_project = Path(__file__).absolute().parents[2]
    for ancestor in (path, *path.parents):
        if ancestor.exists():
            attrs = getattr(ancestor.lstat(), "st_file_attributes", 0)
            if ancestor.is_symlink() or attrs & 0x400:
                raise ValueError("OUTPUT_REDIRECTED")
        if ancestor.name.casefold() in {"source", "docs", "build"}:
            project = ancestor.parent
            # A matching suffix is not permission to write into a maintained
            # tree. Recognize this project and relocated/sibling projects by
            # their directory contract, without importing their implementations.
            if project == source_project or (
                    (project / "Source").is_dir() and (project / "docs").is_dir()):
                raise ValueError("OUTPUT_IN_PROTECTED_TREE: " + str(ancestor))
    parts = [p.casefold() for p in path.parts]
    if not any(parts[i:i + 3] == ["output", "work", "pixso-native"]
               for i in range(len(parts) - 2)):
        raise ValueError("OUTPUT_MUST_BE_PIXSO_NATIVE_WORK")
    return path


def new_output(path):
    path = checked_work_path(path)
    if path.exists():
        raise ValueError("OUTPUT_ALREADY_EXISTS")
    path.mkdir(parents=True)
    return path


def write_json(path, value):
    path.write_bytes((json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))


def unpack(archive, destination):
    files, names, total = [], set(), 0
    with zipfile.ZipFile(archive) as source:
        for member in source.infolist():
            if member.is_dir():
                continue
            rel = relative_name(member.filename)
            key = str(rel).casefold()
            if key in names or stat.S_ISLNK(member.external_attr >> 16):
                raise ValueError("DUPLICATE_OR_LINK_ARCHIVE_MEMBER: " + str(rel))
            total += member.file_size
            if member.file_size > MAX_FILE or total > MAX_TOTAL:
                raise ValueError("ARCHIVE_SIZE_LIMIT")
            names.add(key)
            files.append((member, rel))
        output = new_output(destination)
        inventory = []
        for member, rel in files:
            data = source.read(member)
            target = output / "raw" / str(rel)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            inventory.append({"path": str(rel), "bytes": len(data), "sha256": digest(data)})
    write_json(output / "input-manifest.json", {
        "schemaVersion": 1, "kind": "attachment-archive", "archiveName": archive.name,
        "archiveSha256": digest(archive.read_bytes()), "files": inventory,
        "sameBatchAsLiveExport": "unverified",
    })
    return inventory


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise ValueError("EXPORT_REDIRECT_REFUSED")


def fetch(url, port):
    parsed = urllib.parse.urlsplit(url)
    if (parsed.scheme != "http" or parsed.hostname not in ("localhost", "127.0.0.1")
            or parsed.port != port or parsed.username or parsed.password
            or not parsed.path.startswith(("/code/", "/assets/"))):
        raise ValueError("UNEXPECTED_EXPORT_URL")
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    with opener.open(url, timeout=30) as response:
        data = response.read(MAX_FILE + 1)
    if len(data) > MAX_FILE:
        raise ValueError("EXPORT_FILE_TOO_LARGE")
    return data


def download(entries_path, destination, port):
    payload = json.loads(entries_path.read_bytes())
    entries = payload.get("pageEntries", []) + payload.get("codeEntries", [])
    if not entries:
        raise ValueError("EMPTY_D2C_ENTRIES")
    output = new_output(destination)
    sources, inventory, names, total = {}, [], set(), 0
    for entry in entries:
        rel = str(relative_name(entry["path"]))
        if rel.casefold() in names:
            raise ValueError("DUPLICATE_EXPORT_PATH")
        names.add(rel.casefold())
        data = fetch(entry["url"], port)
        total += len(data)
        if total > MAX_TOTAL:
            raise ValueError("EXPORT_TOTAL_TOO_LARGE")
        raw = output / "raw" / rel
        raw.parent.mkdir(parents=True, exist_ok=True)
        raw.write_bytes(data)
        sources[rel] = data.decode("utf-8-sig")
        inventory.append({"path": rel, "url": entry["url"], "bytes": len(data), "sha256": digest(data)})
    urls = sorted(set(url for text in sources.values() for url in
                      re.findall(r"http://(?:localhost|127\.0\.0\.1):\d+/assets/[^\s'\"<>)]+", text)))
    localized, assets = {}, []
    for url in urls:
        data = fetch(url, port)
        total += len(data)
        if total > MAX_TOTAL:
            raise ValueError("EXPORT_TOTAL_TOO_LARGE")
        basename = urllib.parse.unquote(urllib.parse.urlsplit(url).path.rsplit("/", 1)[-1])
        relative_name(basename)
        suffix = Path(basename).suffix.lower()
        category = "fonts" if suffix in (".ttf", ".otf", ".woff", ".woff2") else "images"
        rel = "src/assets/" + category + "/" + basename
        if rel.casefold() in names:
            raise ValueError("ASSET_BASENAME_COLLISION")
        names.add(rel.casefold())
        target = output / "localized" / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        localized[url] = rel
        assets.append({"url": url, "path": rel, "sha256": digest(data), "bytes": len(data)})
    patches = []
    for rel, text in sources.items():
        original = text
        for url, asset in localized.items():
            target = os.path.relpath(asset, str(PurePosixPath(rel).parent)).replace("\\", "/")
            if not target.startswith("."):
                target = "./" + target
            text = text.replace(url, target)
        dest = output / "localized" / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(text.encode("utf-8"))
        if text != original:
            patches.append({"path": rel, "reason": "temporary resource URLs localized",
                            "sha256": digest(text.encode("utf-8"))})
    write_json(output / "input-manifest.json", {
        "schemaVersion": 1, "kind": "live-native-vue-export", "files": inventory,
        "assets": assets, "localization": patches, "standaloneVueBuild": "not_verified",
        "note": "Only files actually returned by D2C are present; missing entry/router files are not fabricated.",
    })
    return inventory


def main():
    if sys.version_info[:2] != (3, 9):
        raise ValueError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("unpack", "download"))
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--port", type=int, default=3667)
    args = parser.parse_args()
    result = (unpack(args.input, args.output) if args.action == "unpack"
              else download(args.input, args.output, args.port))
    print(json.dumps({"status": "通过", "scope": "input preservation only", "files": len(result)}))


if __name__ == "__main__":
    main()
