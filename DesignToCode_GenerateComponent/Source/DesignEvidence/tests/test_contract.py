"""Contract regression tests use isolated copies; never alter audit evidence."""
import copy
import json
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from contract_io import load, write, digest, value_hash, body, encoded, pointer, validate_schema
from normalize import build
from verify import verify, verify_canonical
PACKAGE = ROOT / "packages" / "DesignTest-2-145"
MANIFEST = PACKAGE / load(PACKAGE / "current.json")["sourceManifest"]
INDEX = PACKAGE / "canonical" / "index.json"

class ContractTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
    def clone_canonical(self):
        target = self.base / "canonical"
        shutil.copytree(INDEX.parent, target)
        return target / "index.json"
    def update_artifact(self, index_path, name, mutate):
        index = load(index_path)
        artifact = index["artifacts"][name]
        p = index_path.parent / artifact["path"]
        data = load(p)
        mutate(data)
        artifact.update(write(p,data))
        write(index_path,index)
    def clone_run(self):
        target = self.base / "run"
        shutil.copytree(MANIFEST.parent, target)
        return target / "manifest.json"
    def mutate_body(self, manifest_path, role, mutate):
        manifest = load(manifest_path)
        call = next(c for c in manifest["calls"] if c["role"] == role)
        path = manifest_path.parent / call["response"]["path"]
        document = load(path)
        response = pointer(document,call["response"]["pointer"])
        parsed = body(response)
        mutate(parsed)
        block = next(b for b in response["result"]["content"] if b.get("type")=="text" and b["text"].lstrip().startswith(("{","[")))
        block["text"] = json.dumps(parsed,ensure_ascii=False)
        write(path,document)
        for item in manifest["files"]:
            if item["path"] == call["response"]["path"]:
                item["sha256"] = digest(path.read_bytes());item["bytes"] = path.stat().st_size
        call["response"]["sha256"] = value_hash(response)
        call["payloadSha256"] = value_hash(response["result"])
        call["bodySha256"] = value_hash(parsed)
        write(manifest_path,manifest)
    def test_full_audit_and_corrected_counts(self):
        result = verify(INDEX,MANIFEST,PACKAGE/"current.json")
        self.assertTrue(result["rawAudited"])
        self.assertEqual(result["counts"]["componentSets"],11)
        self.assertEqual(result["counts"]["instances"],45)
        self.assertEqual(result["counts"]["variants"],824)
        self.assertEqual(result["counts"]["pageReferencedVariables"],32)
        self.assertEqual(result["counts"]["referencedVariables"],32)
        self.assertEqual(result["counts"]["pageBindings"],256)
        self.assertEqual(result["counts"]["supplementalBindings"],7)
        self.assertEqual(result["counts"]["resolvedModeValues"],256)
    def test_deterministic_bytes(self):
        first=build(MANIFEST);second=build(MANIFEST)
        self.assertEqual(encoded(first[1]),encoded(second[1]))
        for name,doc in first[1].items():
            self.assertEqual(encoded(doc),(INDEX.parent/(name+".json")).read_bytes())
    def test_duplicate_variable_bodies_have_one_contract(self):
        manifest=load(MANIFEST)
        calls={c["role"]:c for c in manifest["calls"] if c["role"] in ("variable-sets-full","variables-full")}
        self.assertEqual(calls["variable-sets-full"]["bodySha256"],calls["variables-full"]["bodySha256"])
        self.assertNotEqual(calls["variable-sets-full"]["payloadSha256"],calls["variables-full"]["payloadSha256"])
        self.assertEqual(len(load(INDEX.parent/"variables-used.json")["variables"]),32)
    def test_aggregate_calls_are_individually_locatable(self):
        manifest=load(MANIFEST)
        for role in ("state-group-variants","state-group-dsl"):
            calls=[c for c in manifest["calls"] if c["role"]==role]
            self.assertEqual(len(calls),3)
            self.assertEqual(len({c["response"]["pointer"] for c in calls}),3)
            self.assertEqual(len({c["response"]["sha256"] for c in calls}),3)
            self.assertTrue(all(c["capturedAt"] is None for c in calls))
    def test_raw_tamper_rejected(self):
        manifest=self.clone_run();m=load(manifest)
        p=manifest.parent/m["files"][0]["path"]
        p.write_bytes(p.read_bytes()+b" ")
        with self.assertRaisesRegex(ValueError,"Raw hash"):
            build(manifest)
    def test_artifact_tamper_rejected(self):
        index=self.clone_canonical();p=index.parent/"components.json"
        p.write_bytes(p.read_bytes()+b" ")
        with self.assertRaisesRegex(ValueError,"hash"):
            verify_canonical(index)
    def test_path_traversal_rejected(self):
        index=self.clone_canonical();data=load(index)
        data["artifacts"]["components"]["path"]="../components.json";write(index,data)
        with self.assertRaisesRegex(ValueError,"traversal"):
            verify_canonical(index)
    def test_schema_version_rejected(self):
        index=self.clone_canonical();data=load(index);data["schemaVersion"]=2;write(index,data)
        with self.assertRaisesRegex(ValueError,"Schema"):
            verify_canonical(index)
    def test_wrong_identity_rejected(self):
        manifest=self.clone_run()
        self.mutate_body(manifest,"file-identity",lambda d:d.update(fileKey="other"))
        with self.assertRaisesRegex(ValueError,"identity"):
            build(manifest)
    def test_missing_variant_key_rejected(self):
        manifest=self.clone_run()
        vid=load(INDEX.parent/"component-variants.json")["variants"][0]["variantNodeId"]
        def change(items):
            next(c for c in items if c.get("node_id")==vid)["component_key"]=""
        self.mutate_body(manifest,"components-full",change)
        with self.assertRaisesRegex(ValueError,"Variant key"):
            build(manifest)
    def test_alias_cycle_missing_target_and_mode_rejected(self):
        variable_id=load(INDEX.parent/"variables-used.json")["variables"][0]["variableId"]
        for failure in ("cycle","missing-target","missing-mode"):
            with self.subTest(failure=failure):
                target=self.base/failure;shutil.copytree(MANIFEST.parent,target);manifest=target/"manifest.json"
                def change(groups):
                    item=next(v for items in groups.values() for v in items if v["id"]==variable_id)
                    mid=next(iter(item["modeValues"]))
                    if failure=="missing-mode":
                        item["modeValues"]["unregistered-mode"]=item["modeValues"].pop(mid)
                    else:
                        item["modeValues"][mid]["value"]={"id":variable_id if failure=="cycle" else "missing:target"}
                self.mutate_body(manifest,"variables-full",change)
                with self.assertRaises(ValueError):
                    build(manifest)
    def test_broken_provenance_rejected(self):
        index=self.clone_canonical()
        self.update_artifact(index,"provenance",lambda d:d["mappings"][2]["sources"].append({"callId":"missing-call","bodyPointer":""}))
        with self.assertRaisesRegex(ValueError,"provenance call"):
            verify_canonical(index)
    def test_semantic_tamper_rejected_even_with_updated_artifact_hash(self):
        index=self.clone_canonical()
        self.update_artifact(index,"variables-used",lambda d:d["variables"][0]["modes"][0].update(finalValue=999999))
        with self.assertRaisesRegex(ValueError,"Alias final"):
            verify_canonical(index)
    def test_capture_records_each_request_and_exact_response(self):
        from capture import Capture
        from contract_io import read_calls
        class Response:
            headers = {}
            def __init__(self,raw): self.raw=raw
            def __enter__(self): return self
            def __exit__(self,*args): pass
            def read(self): return self.raw
        raw=b'{"jsonrpc":"2.0","id":1,"result":{"content":[{"type":"text","text":"{}"}]}}'
        session=Capture(self.base/"fresh","http://test","file","2:145","test")
        with patch("capture.urlopen",return_value=Response(raw)):
            session.tool("get_variables",{},"variables-full")
        manifest=load(session.output/"manifest.json")
        call=manifest["calls"][0]
        self.assertEqual((session.output/call["response"]["path"]).read_bytes(),raw)
        self.assertIsNotNone(call["capturedAt"])
        self.assertEqual(call["tool"],"get_variables")
        self.assertEqual(len(manifest["files"]),2)
        read_calls(session.output/"manifest.json")
    def test_capture_keeps_sse_transport(self):
        from capture import Capture
        from contract_io import read_calls
        class Response:
            headers={}
            def __enter__(self):return self
            def __exit__(self,*args):pass
            def read(self):return b'data: {"id":1,"result":{"content":[{"type":"text","text":"{}"}]}}\n\n'
        session=Capture(self.base/"sse","http://test","file","2:145","test")
        with patch("capture.urlopen",return_value=Response()):session.tool("get_variables",{},"variables-full")
        manifest=load(session.output/"manifest.json")
        self.assertEqual(sum(f["path"].endswith(".sse") for f in manifest["files"]),1)
        read_calls(session.output/"manifest.json")
    def test_failed_transport_is_client_diagnostic(self):
        from capture import Capture
        session=Capture(self.base/"failed","http://test","file","2:145","test")
        with patch("capture.urlopen",side_effect=OSError("test failure")),self.assertRaises(OSError):
            session.tool("get_variables",{},"variables-full")
        manifest=load(session.output/"manifest.json")
        self.assertEqual(manifest["failures"][0]["status"],"client-transport-error")
        self.assertEqual(manifest["calls"],[])
        self.assertFalse(any("response" in f for f in manifest["files"]))
    def test_malformed_response_is_preserved_as_client_diagnostic(self):
        from capture import Capture
        class Response:
            headers={}
            def __enter__(self):return self
            def __exit__(self,*args):pass
            def read(self):return b"malformed MCP transport"
        session=Capture(self.base/"malformed","http://test","file","2:145","test")
        with patch("capture.urlopen",return_value=Response()),self.assertRaises(Exception):
            session.tool("get_variables",{},"variables-full")
        manifest=load(session.output/"manifest.json")
        failure=manifest["failures"][0]
        self.assertEqual(failure["status"],"client-response-decode-error")
        self.assertEqual((session.output/failure["responseFile"]).read_bytes(),b"malformed MCP transport")
    def test_instance_variant_separation(self):
        docs=load(INDEX.parent/"design-identity.json")
        self.assertFalse({v["variantNodeId"] for v in docs["variants"]}&{v["instanceNodeId"] for v in docs["instances"]})
        self.assertTrue(all("nativeComponentKey" not in s for s in docs["componentSets"]))

if __name__=="__main__":
    unittest.main()
