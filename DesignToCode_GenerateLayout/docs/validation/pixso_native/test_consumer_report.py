"""A failed consumer command must replace, not coexist with, a stale PASS."""
from pathlib import Path
import subprocess
import unittest
from unittest.mock import Mock, patch

import check_consumer


class ConsumerReportTests(unittest.TestCase):
    def test_failure_records_fail_before_raising(self):
        log = Mock()
        log.parent = Path("synthetic-not-written")
        result = subprocess.CompletedProcess(["fixture"], 1, b"", b"fixture failure")
        with patch.object(check_consumer.subprocess, "run", return_value=result), \
                patch.object(check_consumer, "write_json") as write:
            with self.assertRaisesRegex(RuntimeError, "COMMAND_FAILED"):
                check_consumer.run(["fixture"], {}, log)
            self.assertEqual(write.call_args[0][0], log.parent / "result.json")
            self.assertEqual(write.call_args[0][1]["status"], "FAIL")
            log.write_bytes.assert_called_once_with(b"fixture failure")


if __name__ == "__main__":
    unittest.main()
