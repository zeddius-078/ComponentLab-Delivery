"""ComponentLab V2 compiler tests.

Run: python -B -m unittest discover -s DesignSystem/scripts/v2/tests -v
(no third-party dependencies)
"""
from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SCRIPTS))

from v2.model import TokenModel, ValidationError  # noqa: E402
from v2.resolver import Resolver  # noqa: E402
from v2.values import css_value, qt_value  # noqa: E402

ROOT = SCRIPTS.parents[1]  # design-assets
SOURCE = ROOT / "DesignSystem/source"


class ModelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = TokenModel(SOURCE)

    def test_layer_counts(self):
        self.assertEqual(len(self.model.layers["primitive"]), 270)
        self.assertEqual(len(self.model.layers["profile"]), 72)
        self.assertEqual(len(self.model.layers["semantic"]), 310)
        self.assertEqual(len(self.model.layers["component"]), 539)
        self.assertEqual(len(self.model.product), 17)

    def test_unique_canonical_paths(self):
        seen: dict[str, str] = {}
        for layer, records in self.model.layers.items():
            for path in records:
                self.assertNotIn(path, seen, f"duplicate path {path}")
                seen[path] = layer

    def test_aliases_resolvable_and_typed(self):
        records = self.model.context("standard", "light")
        resolver = Resolver(self.model, records)
        resolved = resolver.resolve_all()
        self.assertEqual(len(resolved), len(records))

    def test_wide_profile_differs(self):
        records = self.model.context("wide", "light")
        resolver = Resolver(self.model, records)
        wide = resolver.resolve("profile.typography-scale.font-size.02")
        standard = Resolver(self.model, self.model.context("standard", "light")).resolve(
            "profile.typography-scale.font-size.02")
        self.assertEqual(standard.value, 14)
        self.assertEqual(wide.value, 18)

    def test_dark_mode_sparse_but_declared(self):
        dark = Resolver(self.model, self.model.context("standard", "dark"))
        self.assertEqual(dark.resolve("semantic.color.text.primary").value["components"],
                         [243, 243, 243])
        # a role without a design dark value stays light -> dark not publishable
        self.assertEqual(dark.resolve("semantic.color.background.page").value["components"],
                         [255, 255, 255])

    def test_opacity_unit_percent(self):
        light = Resolver(self.model, self.model.context("standard", "light"))
        self.assertEqual(light.resolve("primitive.opacity.90").unit, "%")
        self.assertAlmostEqual(light.opacity_ratio("primitive.opacity.90"), 0.9)

    def test_product_scope_has_wide_mode(self):
        records = self.model.context("wide", "light")
        resolver = Resolver(self.model, records)
        self.assertEqual(resolver.resolve("product.application-layout.visual-area.width").value,
                         1040)

    def test_color_formats(self):
        light = Resolver(self.model, self.model.context("standard", "light"))
        self.assertEqual(qt_value(light.resolve("primitive.color.highlight.purple-100")),
                         "#6235F7")
        self.assertEqual(qt_value(light.resolve("primitive.color.none")),
                         "rgba(255, 255, 255, 0)")
        self.assertEqual(css_value(light.resolve("primitive.dimension.16")), "16px")

    def test_figma_only_progress_states_present(self):
        self.assertIn("component.progress-bar.state.paused.indicator.color",
                      self.model.layers["component"])

    def test_no_component_to_component_aliases(self):
        for path, record in self.model.layers["component"].items():
            value = record.value
            if isinstance(value, str) and value.startswith("{"):
                self.assertFalse(value[1:-1].startswith("component."),
                                 f"horizontal alias remains: {path} -> {value}")

    def test_manifest_and_legacy_map(self):
        manifest = json.loads((ROOT / "DesignSystem/config/manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["schemaVersion"], "2.0")
        self.assertEqual(manifest["designSnapshot"]["figmaFileKey"],
                         "w92BKW1EcutKmTXkwzNHbf")
        legacy = json.loads((ROOT / "DesignSystem/config/legacy-map.json").read_text(encoding="utf-8"))
        self.assertEqual(len(legacy["decisions"]), 1055)
        pending = legacy["pendingConflicts"]
        self.assertEqual(len(pending), 8)
        for item in pending:
            self.assertEqual(item["status"], "pending-decision")


class LegacyReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = TokenModel(SOURCE)
        from v2 import adapters
        cls.adapters = adapters
        cls.contexts = adapters.qt_runtime.build_contexts(cls.model)
        decisions = json.loads((ROOT / "DesignSystem/config/legacy-map.json").read_text(encoding="utf-8"))
        cls.profiles, _, cls.conflicts = adapters.qt_runtime.build_legacy_profiles(
            cls.model, cls.contexts, decisions["decisions"],
            ROOT / "DesignSystem/archive/commerly")

    def test_all_four_runtime_profiles_populated(self):
        for name in ("light-comfortable", "light-compact", "dark-comfortable", "dark-compact"):
            self.assertIn(name, self.profiles)
            self.assertTrue(self.profiles[name], name)

    def test_canonical_keys_inside_runtime_profiles(self):
        color = self.profiles["light-comfortable"]["color"]
        self.assertIn("component.button.primary.default.label.color", color)
        self.assertEqual(color["component.button.primary.default.label.color"],
                         "#FFFFFF")

    def test_audited_values_preserved(self):
        color = self.profiles["light-comfortable"]["color"]
        self.assertEqual(color["semantic.color.text.primary"], "rgba(25, 25, 25, 1)")
        layout = self.profiles["light-comfortable"]["layout"]
        self.assertEqual(layout["semantic.dimension.control.height-xs"], 32)

    def test_adapter_scope(self):
        layout = self.profiles["light-comfortable"]["layout"]
        self.assertEqual(layout["adapter.qt.dimension.control-height.md"], 32)
        self.assertEqual(layout["adapter.qt.button.iconContainer.M"], 28)
        self.assertEqual(layout["adapter.qt.checkbox.hitTarget.sm"], 32)

    def test_dark_adaptation_on_canonical_roles(self):
        dark = self.profiles["dark-comfortable"]["color"]
        self.assertEqual(dark["semantic.color.background.hover"], "rgba(57, 57, 57, 1)")
        self.assertEqual(dark["component.input.base.default.placeholder.color"],
                         "rgba(147, 147, 147, 1)")


class ReferenceValuesTests(unittest.TestCase):
    """radio/menu/tree/table values must match DesignSystem/imports/reference exactly."""

    def test_reference_values_exact(self):
        import importlib.util
        path = Path(__file__).with_name("verify_reference_values.py")
        spec = importlib.util.spec_from_file_location("verify_reference_values", path)
        module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(module)
        self.assertIsNone(module.main())


if __name__ == "__main__":
    unittest.main(verbosity=2)
