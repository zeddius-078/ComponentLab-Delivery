"""Negative coverage for actual-instance content checks (not visual acceptance)."""
import copy
import unittest

from content_readback import compare_content


class ContentReadbackTests(unittest.TestCase):
    def setUp(self):
        self.binding = {"objectName": "nav", "qtClass": "commonWidget::Z_Navbar",
                        "source": {"instancePath": "root/nav"},
                        "declaredProps": {"Density": "Comfortable"},
                        "boundSlots": {"profileName": "Actual override",
                                       "searchPlaceholder": "Search"}}
        self.snapshot = {"objects": [{"name": "nav", "class": "commonWidget::Z_Navbar",
            "properties": {"profileName": "Actual override", "searchPlaceholder": "Search",
                           "density": "Comfortable"}}]}

    def test_exact_content_and_state(self):
        result = compare_content([self.binding], self.snapshot)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(len(result["checks"]), 3)

    def test_sdk_default_or_wrong_widget_does_not_pass(self):
        for change in ("default", "missing", "class"):
            snapshot = copy.deepcopy(self.snapshot)
            if change == "default":
                snapshot["objects"][0]["properties"]["profileName"] = "SDK default"
            elif change == "missing":
                snapshot["objects"][0]["name"] = "some_probe"
            else:
                snapshot["objects"][0]["class"] = "QWidget"
            self.assertEqual(compare_content([self.binding], snapshot)["status"], "FAIL")

    def test_duplicate_objects_and_unknown_slot_fail_closed(self):
        self.snapshot["objects"].append(copy.deepcopy(self.snapshot["objects"][0]))
        with self.assertRaisesRegex(ValueError, "DUPLICATE_RUNTIME_OBJECT"):
            compare_content([self.binding], self.snapshot)
        self.snapshot["objects"].pop()
        self.binding["boundSlots"]["unknown"] = "not verified"
        with self.assertRaisesRegex(ValueError, "UNSUPPORTED_CONTENT_READBACK"):
            compare_content([self.binding], self.snapshot)


if __name__ == "__main__":
    unittest.main()
