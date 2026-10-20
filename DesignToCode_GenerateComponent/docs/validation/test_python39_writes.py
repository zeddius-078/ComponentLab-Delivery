"""Exercise real generator write loops with the Python 3.9 Path API signature."""
import importlib.util
import ast
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[2]
TOOLS = PROJECT / "Source/ComponentKit/tools"
sys.path.insert(0, str(TOOLS))


def python39_write_text(path, data, encoding=None, errors=None):
    # No newline keyword: this is the Python 3.9 pathlib call contract.
    with path.open("w", encoding=encoding, errors=errors) as stream:
        return stream.write(data)


class Python39WriteTests(unittest.TestCase):
    def test_generator_syntax_is_python39_compatible(self):
        for name in ("generate_native_components.py", "generate_design_component_catalog.py"):
            ast.parse((TOOLS / name).read_text(encoding="utf-8"), filename=name, feature_version=9)

    def test_real_generator_write_loops_preserve_utf8_lf(self):
        cache = PROJECT / "output/work/cache/python39-write-tests"
        cache.mkdir(parents=True, exist_ok=True)
        for name in ("generate_native_components", "generate_design_component_catalog"):
            with self.subTest(generator=name), tempfile.TemporaryDirectory(dir=str(cache)) as folder:
                spec = importlib.util.spec_from_file_location(name, TOOLS / (name + ".py"))
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                target = Path(folder) / "catalog.json"
                content = json.dumps({"canonicalLibraryEntryCount": 1, "componentCount": 1,
                                      "name": "兼容测试"}, ensure_ascii=False) + "\n"
                with patch.object(module, "ROOT", Path(folder)), \
                     patch.object(module, "expected_outputs", return_value={target: content}), \
                     patch.object(Path, "write_text", python39_write_text), \
                     patch.object(sys, "argv", [name]):
                    if name == "generate_native_components":
                        with patch.object(module, "load_inputs", return_value=([], {})):
                            module.main()
                    else:
                        with patch.object(module, "OUTPUT_JSON", target), \
                             patch.object(module, "load_figma_inventory", return_value={"pages": []}):
                            module.main()
                self.assertEqual(target.read_bytes(), content.encode("utf-8"))
                self.assertNotIn(b"\r\n", target.read_bytes())


if __name__ == "__main__":
    unittest.main()
