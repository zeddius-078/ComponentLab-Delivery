"""Compare preserved exports; normalize hash-verified url(...) text only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import stat
import sys

from acquire import new_output, relative_name


def read_inside(root, name):
    rel = relative_name(name)
    target = root.joinpath(*rel.parts)
    current = root
    for part in (None,) + rel.parts:
        if part is not None:
            current = current / part
        metadata = current.lstat()
        if current.is_symlink() or getattr(metadata, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT:
            raise ValueError("REPARSE_INPUT_REFUSED")
    target.resolve().relative_to(root.resolve())
    return target.read_bytes()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def normalize_urls(data, resources):
    text = data.decode("utf-8")
    def replacement(match):
        value = match.group(1).strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "'\"":
            value = value[1:-1]
        return ('url("sha256:' + resources[value] + '")') if value in resources else match.group(0)
    return re.sub(r"url\(([^()]*)\)", replacement, text)


def compare(attachment, live):
    manifest = json.loads((live / "input-manifest.json").read_bytes())
    live_urls, attachment_urls, assets = {}, {}, []
    for item in manifest["assets"]:
        raw = read_inside(live / "localized", item["path"])
        if digest(raw) != item["sha256"]:
            raise ValueError("LIVE_ASSET_HASH_MISMATCH: " + item["path"])
        try:
            other = read_inside(attachment, item["path"])
        except FileNotFoundError:
            assets.append({"path": item["path"], "equal": False, "reason": "missing"})
            continue
        equal = digest(other) == item["sha256"]
        assets.append({"path": item["path"], "equal": equal})
        if equal:
            live_urls[item["url"]] = item["sha256"]
            if not item["path"].startswith("src/"):
                raise ValueError("UNEXPECTED_RESOURCE_PATH")
            attachment_urls["@/" + item["path"][4:]] = item["sha256"]
    files = []
    for item in manifest["files"]:
        raw = read_inside(live / "raw", item["path"])
        if digest(raw) != item["sha256"]:
            raise ValueError("LIVE_CODE_HASH_MISMATCH: " + item["path"])
        try:
            other = read_inside(attachment, item["path"])
        except FileNotFoundError:
            files.append({"path": item["path"], "comparison": "missing"})
            continue
        equal = raw == other
        normalized_equal = normalize_urls(raw, live_urls) == normalize_urls(other, attachment_urls)
        files.append({"path": item["path"], "liveSha256": digest(raw),
                      "attachmentSha256": digest(other),
                      "comparison": "byte-equal" if equal else
                          "verified-resource-url-text-only" if normalized_equal else "different"})
    return {"scope": "current preserved files; url(...) text normalization, not Vue semantic proof",
            "sameBatch": "unverified", "files": files, "assets": assets,
            "completeVueBuild": "not-verified"}


def main():
    if sys.version_info[:2] != (3, 9):
        raise RuntimeError("PYTHON39_REQUIRED")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("attachment", "live", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    for path in (args.attachment, args.live, args.output):
        if not path.is_absolute():
            raise ValueError("ABSOLUTE_PATH_REQUIRED")
    report = compare(args.attachment, args.live)
    output = new_output(args.output)
    (output / "comparison.json").write_bytes(
        (json.dumps(report, indent=2, ensure_ascii=False) + "\n").encode("utf-8"))
    counts = {}
    for file in report["files"]:
        counts[file["comparison"]] = counts.get(file["comparison"], 0) + 1
    print(json.dumps({"files": counts, "assets": len(report["assets"]),
                      "assetDifferences": sum(not x["equal"] for x in report["assets"]),
                      "sameBatch": report["sameBatch"], "report": str(output / "comparison.json")}))
    return 2 if counts.get("different") or counts.get("missing") or any(
        not item["equal"] for item in report["assets"]) else 0


if __name__ == "__main__":
    sys.exit(main())
