import sys
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Source"))

from AgentCapabilityProfile import binding, identity
from ArtifactLifecycle import verify_execution_capability


class AgentCapabilityProfileTests(unittest.TestCase):
    def test_fixed_execution_sku_is_valid_and_portable(self):
        profile = identity()
        self.assertEqual(profile["canonicalId"], "DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK")
        self.assertEqual(profile["executionClass"], "controlled-low-autonomy")
        self.assertEqual(len(profile["profileSha256"]), 64)
        receipt_value = binding()
        self.assertNotIn("profilePath", receipt_value)
        self.assertEqual(verify_execution_capability(receipt_value), receipt_value)


if __name__ == "__main__":
    unittest.main()
