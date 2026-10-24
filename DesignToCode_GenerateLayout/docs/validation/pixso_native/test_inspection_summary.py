"""A diagnostic inventory must not turn partial facts into acceptance."""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from inspect_page import blocking_summary


class InspectionSummaryTests(unittest.TestCase):
    def test_collects_independent_failures_and_preserves_scope(self):
        source = {"instancePath": "root/button"}
        report = {
            "layoutIssues": [{"source": {"instancePath": "root"}, "error": "UNLOWERED_CSS"}],
            "componentBoundaries": [{"boundaries": [source],
                                     "style": {"width": "40px", "color": "red"}}],
            "componentBindings": [{"source": source, "mappingKey": "button",
                                   "issues": ["COMPONENT_CONTRACT_NOT_VERIFIED",
                                              "UNMAPPED_COMPONENT_PROPS"],
                                   "unmappedProps": ["Content"]}],
            "componentContent": [{"source": source,
                                  "conflicts": [{"code": "DECLARED_CONTENT_CONFLICT"}]}]}
        summary = blocking_summary(report, {"components": [{"mappingKey": "button"}]})
        self.assertEqual(len(summary["findings"]), 5)
        self.assertEqual(summary["status"], "blocked")
        style = next(item for item in summary["findings"] if item["stage"] == "style")
        self.assertEqual(style["detail"]["observed"], {"color": "red"})
        self.assertFalse(summary["generationReady"])
        self.assertEqual(report["componentBindings"][0]["unmappedProps"], ["Content"])

    def test_no_mapping_is_not_success(self):
        report = {"componentBoundaries": [{"boundaries": [{"instancePath": "root/item"}]}]}
        self.assertEqual(blocking_summary(report)["counts"], {"COMPONENT_MAPPING_NOT_INSPECTED": 1})

    def test_empty_findings_never_certify_generation(self):
        summary = blocking_summary({})
        self.assertEqual(summary["status"], "no-observed-blockers")
        self.assertFalse(summary["generationReady"])
        self.assertEqual(summary["acceptance"], "not-run")


if __name__ == "__main__":
    unittest.main()
