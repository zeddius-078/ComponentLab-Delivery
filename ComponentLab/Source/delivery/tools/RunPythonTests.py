"""Run one unittest suite and save explicit case counts with its input hashes."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import unittest

from CheckProject import SOURCE as ROOT, PROJECT, validation_inputs


class CaseResult(unittest.TextTestResult):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.passed_cases = set()

    def addSuccess(self, test):
        self.passed_cases.add(test.id())
        super().addSuccess(test)


def run(pattern, output):
    if not output.resolve().is_relative_to(PROJECT / "output/logs"):
        raise ValueError("TEST_OUTPUT_MUST_BE_IN_OUTPUT_LOGS")
    suite = unittest.defaultTestLoader.discover(str(ROOT/"delivery/tests"), pattern=pattern)
    discovered = suite.countTestCases()
    before = validation_inputs()
    result = unittest.TextTestRunner(verbosity=2, resultclass=CaseResult).run(suite)
    # Several failed subtests still belong to one test method.
    failed = {getattr(test, "test_case", test).id() for test, _ in result.failures}
    failed.update(test.id() for test in result.unexpectedSuccesses)
    unchanged = before == validation_inputs()
    summary = {"verifiedAtUtc": datetime.now(timezone.utc).isoformat(),
               "pattern": pattern, "files": len(list((ROOT/"delivery/tests").glob(pattern))),
               "discovered": discovered, "cases": result.testsRun,
               "passed": len(result.passed_cases), "failed": len(failed),
               "errors": len(result.errors), "skipped": len(result.skipped),
               "expectedFailures": len(result.expectedFailures),
               "notRun": max(0, discovered-result.testsRun),
               "status": "passed" if result.wasSuccessful() and unchanged else "failed",
               "validationInputs": before, "inputsUnchanged": unchanged}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    return 0 if summary["status"] == "passed" else 1


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("pattern")
    cli.add_argument("--output", type=Path, required=True)
    args = cli.parse_args()
    raise SystemExit(run(args.pattern, args.output))
