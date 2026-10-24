from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from compare_inputs import normalize_urls


class InputComparisonTests(unittest.TestCase):
    def test_only_verified_resource_tokens_normalized(self):
        first = normalize_urls(b'.x {background:url(@/a.svg)}', {"@/a.svg": "abc"})
        second = normalize_urls(b'.x {background:url("http://localhost/a.svg")}',
                                {"http://localhost/a.svg": "abc"})
        self.assertEqual(first, second)
        self.assertNotEqual(first, normalize_urls(b'.x {background:url(@/a.svg)}', {}))

    def test_content_hash_difference_remains_visible(self):
        self.assertNotEqual(normalize_urls(b'url(@/a.svg)', {"@/a.svg": "first"}),
                            normalize_urls(b'url(@/a.svg)', {"@/a.svg": "second"}))

    def test_layout_and_property_differences_remain_visible(self):
        self.assertNotEqual(normalize_urls(b'width:10px', {}),
                            normalize_urls(b'width:11px', {}))
        self.assertNotEqual(normalize_urls(b'Content="icon-only"', {}),
                            normalize_urls(b'Content="text-only"', {}))


if __name__ == "__main__":
    unittest.main()
