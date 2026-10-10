import base64
import copy
import importlib.util
import json
import shutil
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DESIGN=ROOT/"mapping/design"
INDEX=ROOT.parents[1]/"DesignEvidence/packages/DesignTest-2-145/canonical/index.json"
spec=importlib.util.spec_from_file_location("pixso_button",DESIGN/"pixso_button.py")
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class PixsoButtonTests(unittest.TestCase):
    def setUp(self):
        self.rule=module.load(DESIGN/"pixso-button/rule.json")
    def test_canonical_conversion(self):
        result=module.convert(INDEX,self.rule)
        button=result["page"]["children"][0]
        self.assertEqual(button["properties"]["text"],"执行工艺")
        self.assertEqual(button["design"]["nativeComponentKey"],"5db2a212943f439833dd3b8d6f8957d687a6edb1")
        self.assertNotIn("instanceNodeId",button["design"])
        self.assertEqual(result,module.convert(INDEX,self.rule))
        self.assertFalse(result["source"]["reviewed"])
        self.assertEqual(result["source"]["kind"],"pixsoCanonical")
    def test_explicit_review_binds_compact_canonical_proof(self):
        proof,_=module.evidence(INDEX,self.rule)
        sha=module.digest(module.encoded(proof))
        result=module.convert(INDEX,self.rule,sha)
        self.assertTrue(result["source"]["reviewed"])
        self.assertEqual(base64.b64decode(result["source"]["snapshotBase64"]),module.encoded(proof))
        self.assertLess(len(module.encoded(proof)),200000)
        with self.assertRaisesRegex(ValueError,"Review hash"):
            module.convert(INDEX,self.rule,"0"*64)
    def test_design_tree_keeps_bindings_and_variant_identity(self):
        tree=module.design_tree(INDEX,self.rule)
        root,text=tree["nodes"]
        self.assertEqual(root["nodeId"],self.rule["variantId"])
        self.assertEqual(text["parentLocator"],root["locator"])
        self.assertEqual(len(tree["tokenBindings"]),7)
        self.assertEqual(len(tree["variables"]),7)
        self.assertNotIn("qtClass",root)
    def test_canonical_only_isolated_input(self):
        with tempfile.TemporaryDirectory() as tmp:
            canonical=Path(tmp)/"canonical";shutil.copytree(INDEX.parent,canonical)
            for name in ("design-identity.json","variable-collections.json","validation.json"):
                (canonical/name).unlink() # Adapter needs only its declared five artifact files.
            result=module.convert(canonical/"index.json",self.rule)
            self.assertEqual(result,module.convert(INDEX,self.rule))
            self.assertFalse((Path(tmp)/"runs").exists())
            self.assertFalse((Path(tmp)/"archive").exists())
    def test_reject_identity_and_property_drift(self):
        for key,value in (("fileKey","other"),("variantId","other"),("componentSetId","other")):
            rule=copy.deepcopy(self.rule);rule[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):module.convert(INDEX,rule)
        rule=copy.deepcopy(self.rule);rule["expected"]["state"]="unsupported"
        with self.assertRaisesRegex(ValueError,"Unsupported"):module.convert(INDEX,rule)
    def test_old_raw_snapshot_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"snapshot.json";p.write_text(json.dumps({"provider":"pixso","dsl":{"roots":[]}}))
            with self.assertRaises(ValueError):module.convert(p,self.rule)

if __name__=="__main__":unittest.main()
