"""Read-only, byte-exact checks for maintained capture fixtures (Python 3.9)."""
import hashlib
import json
from pathlib import Path
import sys
import unittest


PROJECT = Path(__file__).resolve().parents[2]
FIXTURES = PROJECT / "docs/fixtures/layout"
sys.path.insert(0, str(PROJECT / "Source/HtmlDsl/pipeline/capture"))
from capture_mcp import decode_rpc
import capture_manifest
import verify_capture


def digest(data):
    return hashlib.sha256(data).hexdigest()


class FixtureIntegrityTests(unittest.TestCase):
    def test_all_recorded_response_bytes(self):
        metadata_files = sorted(FIXTURES.rglob("*.response-meta.json"))
        self.assertTrue(metadata_files, "No captured response metadata found")
        for metadata_file in metadata_files:
            with self.subTest(path=str(metadata_file.relative_to(FIXTURES))):
                metadata = json.loads(metadata_file.read_bytes())
                body = metadata_file.with_name(
                    metadata_file.name.replace(".response-meta.json", ".response.body")
                ).read_bytes()
                self.assertEqual(digest(body), metadata["sha256"])
                self.assertEqual(len(body), metadata["bytes"])

    def test_saved_extracted_text_is_verbatim(self):
        for request_file in sorted(FIXTURES.rglob("04-call.request.json")):
            with self.subTest(capture=str(request_file.parent.relative_to(FIXTURES))):
                request = json.loads(request_file.read_bytes())
                body = (request_file.parent / "04-call.response.body").read_bytes()
                metadata = json.loads(
                    (request_file.parent / "04-call.response-meta.json").read_bytes()
                )
                self.assertEqual(digest(body), metadata["sha256"])
                response = decode_rpc(body, request["id"])
                blocks = response.get("content", [])
                tool = request["params"]["name"]
                for index, block in enumerate(blocks):
                    if block.get("type") != "text":
                        continue
                    content = block["text"]
                    try:
                        json.loads(content)
                        suffix = ".json"
                    except json.JSONDecodeError:
                        suffix = ".txt"
                    stem = ("canonical.raw" if tool == "get_node_dsl" and len(blocks) == 1
                            else "content-{:02d}".format(index))
                    target = request_file.parent / (stem + suffix)
                    # Some maintained captures retain only a subset of text blocks.
                    # Check every saved extraction; do not invent missing payloads.
                    if target.exists():
                        with self.subTest(file=str(target.relative_to(FIXTURES))):
                            self.assertEqual(digest(target.read_bytes()),
                                             digest(content.encode("utf-8")))

    def test_dsl_capture_verifiers(self):
        captures = [p.parent for p in sorted(FIXTURES.rglob("canonical.raw.json"))
                    if (p.parent / "04-call.request.json").exists()]
        self.assertTrue(captures, "No DSL capture fixtures found")
        for directory in captures:
            with self.subTest(path=str(directory.relative_to(FIXTURES))):
                self.assertEqual(verify_capture.verify(directory)["status"], "PASS")

    def test_page_manifests_and_recorded_hashes(self):
        manifests = []
        for path in sorted(FIXTURES.rglob("capture.json")):
            manifest = json.loads(path.read_bytes())
            if manifest.get("schemaVersion") != "capture-manifest.v1":
                continue
            manifests.append(path)
            with self.subTest(path=str(path.relative_to(FIXTURES))):
                self.assertTrue(manifest.get("sourceHashes"))
                mode = "preview" if manifest["manifestType"] == "preview-fixture" else "formal"
                self.assertEqual(capture_manifest.verify(path, mode)["status"], "PASS")
        self.assertTrue(manifests, "No page capture manifests found")

    def test_saved_verification_hashes_remain_valid(self):
        for path in sorted(FIXTURES.rglob("verification.json")):
            record = json.loads(path.read_bytes())
            for field, name in (("rawSha256", "canonical.raw.json"),
                                ("responseSha256", "04-call.response.body")):
                if field in record:
                    with self.subTest(record=str(path.relative_to(FIXTURES)), field=field):
                        self.assertEqual(digest((path.parent / name).read_bytes()), record[field])


if __name__ == "__main__":
    unittest.main()
