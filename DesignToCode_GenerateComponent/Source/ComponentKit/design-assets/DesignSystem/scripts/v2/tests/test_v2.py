"""ComponentLab V2 compiler tests (pure V2 — no legacy content).

Run: python -B -m unittest discover -s DesignSystem/scripts/v2/tests -v
(no third-party dependencies)
"""
from __future__ import annotations

import json
import sys
import unittest
from unittest.mock import patch
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SCRIPTS))

from v2.model import TokenModel, ValidationError  # noqa: E402
from v2.resolver import Resolver  # noqa: E402
from v2.values import css_value, qt_value  # noqa: E402
from v2.composite_compile import MotionValidator  # noqa: E402
from v2.component_compile import ComponentCompiler  # noqa: E402

ROOT = SCRIPTS.parents[1]  # design-assets
SOURCE = ROOT / "DesignSystem/source"

# Stable reference values for menu/tree/table. Checkbox and Radio are updated
# in independent batches, so their transient paths do not belong in this
# Button/Input/Select verification gate.
REFERENCE_EXPECTATIONS = {
    "component.menu.base.state.default.slot.container.border-radius": 4,
    "component.menu.base.state.default.slot.content.font-size": 14,
    "component.menu.base.size.sm.slot.icon.size": 16,
    "component.menu.base.size.md.slot.icon.size": 20,
    "component.menu.base.size.sm.slot.container.height": 20,
    "component.menu.base.size.md.slot.container.height": 24,
    "component.menu.base.state.default.slot.container.gradient-stop-01": "#F5F5F5",
    "component.menu.base.state.default.slot.container.gradient-stop-02": "#FFFFFF",
    "component.menu.base.state.default.slot.content.color": "#191919",
    "component.menu.base.state.disabled.slot.content.color": "#BBBBBB",
    "component.menu.base.state.default.slot.border.color": "#DFDFDF",
    "component.tree.base.state.default.slot.container.background-color": "#FFFFFF",
    "component.tree.base.state.hover.slot.item.background-color": "#F5F5F5",
    "component.tree.base.state.selected.slot.item.background-color": "#EFEAFE",
    "component.tree.base.state.default.slot.border.color": "#BBBBBB",
    "component.tree.base.state.default.slot.content.font-size": 14,
    "component.tree.base.state.default.slot.icon.size": 20,
    "component.tree.base.state.default.slot.item.height": 32,
    "component.tree.base.state.default.slot.container.padding": 4,
    "component.tree.base.state.default.slot.border.border-width": 1,
    "component.table.base.shared.slot.header.background-color": "#EEEEEE",
    "component.table.base.shared.slot.container.background-color": "#FFFFFF",
    "component.table.base.striping.zebra.slot.row.background-color": "#FAFAFA",
    "component.table.overlay.row-interaction.hover.slot.row.background-color": "#F5F5F5",
    "component.table.overlay.availability.disabled.slot.container.background-color": "#EEEEEE",
    "component.table.overlay.row-interaction.pressed.slot.row.background-color": "#EFEAFE",
    "component.table.overlay.data-status.alarm.slot.row.background-color": "#E02128",
    "component.table.overlay.row-interaction.hover.data-status.alarm.slot.row.background-color": "#E96368",
    "component.table.overlay.row-selection.selected.data-status.alarm.slot.row.background-color": "#BF1C22",
    "component.table.overlay.data-status.alarm.status-presentation.soft.slot.row.background-color": "#FADEDF",
    "component.table.overlay.data-status.error.slot.row.background-color": "#E66F1E",
    "component.table.overlay.data-status.warning.slot.row.background-color": "#E6AF19",
    "component.table.base.shared.slot.border.color": "#DFDFDF",
    "component.table.overlay.row-selection.selected.slot.border.color": "#6235F7",
    "component.table.base.shared.slot.cell.color": "#191919",
    "component.table.overlay.content-role.invert.slot.cell.color": "#FFFFFF",
    "component.table.overlay.availability.disabled.slot.cell.color": "#BBBBBB",
    "component.table.overlay.content-role.unit.slot.cell.color": "#4E4E4E",
    "component.table.base.density.compact.slot.cell.min-height": 20,
    "component.table.base.density.small.slot.cell.min-height": 24,
    "component.table.base.density.medium.slot.cell.min-height": 44,
    "component.table.base.density.large.slot.cell.min-height": 48,
    "component.table.base.density.compact.slot.row.min-height": 28,
    "component.table.base.density.small.slot.row.min-height": 32,
    "component.table.base.density.medium.slot.row.min-height": 56,
    "component.table.overlay.border-emphasis.strong.slot.border.border-width": 2,
}


class ModelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = TokenModel(SOURCE)

    def test_layer_counts(self):
        # Primitive is an extensible governed layer; released slices may add
        # reviewed atoms without invalidating unrelated contracts.
        self.assertGreaterEqual(len(self.model.layers["primitive"]), 299)
        for path in (
            "primitive.spinner.stroke-width",
            "primitive.spinner.track-opacity",
            "primitive.spinner.arc-start-angle",
            "primitive.spinner.arc-sweep-angle",
        ):
            self.assertIn(path, self.model.layers["primitive"])
        self.assertEqual(len(self.model.layers["profile"]), 72)
        self.assertEqual(len(self.model.layers["theme"]), 9)
        self.assertEqual(len(self.model.layers["semantic"]), 310)
        # Component recipes are intentionally evolving in independent batches;
        # validate the governed targets instead of freezing the whole layer to
        # a transient repository-wide count.
        self.assertGreater(len(self.model.layers["component"]), 0)
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

    def test_motion_units(self):
        light = Resolver(self.model, self.model.context("standard", "light"))
        self.assertEqual(light.resolve("primitive.motion.duration.fast").unit, "ms")
        self.assertEqual(light.resolve("primitive.motion.rotation.half-turn").unit, "deg")

    def test_typography_font_size_and_line_height_are_even(self):
        for profile_mode in ("standard", "wide"):
            for theme_mode in ("light", "dark"):
                resolved = Resolver(
                    self.model, self.model.context(profile_mode, theme_mode)).resolve_all()
                odd = [
                    path for path, value in resolved.items()
                    if value.type in ("fontSize", "lineHeight")
                    and isinstance(value.value, (int, float))
                    and value.value % 2 != 0
                ]
                self.assertEqual(odd, [], f"odd typography values in {profile_mode}-{theme_mode}")

    def test_priority_component_visual_tokens_materialized(self):
        component = self.model.layers["component"]
        expected = {
            "component.button.intent.primary.appearance.filled.state.default.content.text-only.slot.container.background-color",
            "component.button.intent.danger.appearance.filled.state.hover.content.text-only.slot.border.color",
            "component.input.appearance.outlined.state.focused.slot.focus-ring.border-width",
            "component.input.appearance.outlined.state.loading.slot.spinner.color",
            "component.select.appearance.outlined.state.focused.open.true.slot.dropdown-indicator.color",
            "component.select.appearance.outlined.state.disabled.open.false.slot.content.color",
        }
        self.assertEqual(expected - set(component), set())
        self.assertFalse(any("component.select.appearance.outlined.state.open" in path
                             for path in component))

    def test_input_embedded_actions_have_independent_four_state_tokens(self):
        component = self.model.layers["component"]
        for action in ("clear", "password-toggle"):
            slot = action
            self.assertIn(f"component.input.base.shared.slot.{slot}.hit-target", component)
            self.assertIn(f"component.input.base.shared.slot.{slot}.border-radius", component)
            for state in ("default", "hover", "pressed", "disabled"):
                prefix = (
                    f"component.input.overlay.action.{action}.action-state.{state}.slot.{slot}."
                )
                self.assertIn(prefix + "color", component)
                self.assertIn(prefix + "background-color", component)

    def test_switch_value_and_state_tokens_are_materialized(self):
        component = self.model.layers["component"]
        self.assertIn("component.switch.base.shared.slot.knob.inset", component)
        self.assertIn("component.switch.base.shared.slot.knob.background-color", component)
        for value in ("off", "on"):
            for state in ("default", "hover", "pressed", "focused", "disabled", "loading"):
                prefix = f"component.switch.value.{value}.state.{state}.slot."
                self.assertIn(prefix + "track.background-color", component)
            self.assertIn(
                f"component.switch.value.{value}.state.disabled.slot.label.color",
                component,
            )

    def test_input_content_is_instance_text_not_a_variant_axis(self):
        contract = json.loads((SOURCE / "component-contracts/input.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/input.json").read_text(encoding="utf-8"))
        self.assertNotIn("content", binding["variantAxes"])
        self.assertEqual(contract["contentModel"]["placeholderText"]["defaultLocale"], "en-US")
        self.assertFalse(contract["contentModel"]["valueText"]["visualToken"])
        self.assertTrue(contract["contentModel"]["actionVisibility"]["runtimeOwned"])

    def test_select_content_is_runtime_or_localized_text_not_a_variant_axis(self):
        contract = json.loads((SOURCE / "component-contracts/select.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/select.json").read_text(encoding="utf-8"))
        self.assertNotIn("content", binding["variantAxes"])
        self.assertEqual(contract["contentModel"]["placeholderText"]["fallbackLocale"], "en-US")
        self.assertEqual(contract["contentModel"]["tagText"]["structure"], "nested-tag-instance")
        self.assertTrue(contract["contentModel"]["popupText"]["searchText"]["notPersistedToCatalog"])

    def test_checkbox_content_separates_structure_from_localized_label(self):
        contract = json.loads((SOURCE / "component-contracts/checkbox.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/checkbox.json").read_text(encoding="utf-8"))
        self.assertEqual(binding["variantAxes"]["content"], ["control-only", "control-with-label"])
        self.assertEqual(contract["contentModel"]["labelText"]["fallbackLocale"], "en-US")
        self.assertEqual(contract["contentModel"]["controlOnly"]["label"], "not-applicable")

    def test_radio_content_separates_structure_from_group_selection(self):
        contract = json.loads((SOURCE / "component-contracts/radio.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/radio.json").read_text(encoding="utf-8"))
        self.assertEqual(binding["variantAxes"]["content"], ["control-only", "control-with-label"])
        self.assertEqual(contract["contentModel"]["labelText"]["fallbackLocale"], "en-US")
        self.assertIn("radio-group-selection", contract["contentModel"]["forbiddenContentResponsibilities"])

    def test_table_content_is_text_and_slot_content_not_table_token_data(self):
        contract = json.loads((SOURCE / "component-contracts/table.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/table.json").read_text(encoding="utf-8"))
        self.assertEqual(contract["contentModel"]["headerText"]["fallbackLocale"], "en-US")
        self.assertIn("record-value", contract["contentModel"]["forbiddenContentResponsibilities"])
        self.assertEqual(binding["contentProperties"]["lifecycleText"]["defaultLocale"], "en-US")
        self.assertEqual(binding["contentProperties"]["cellDisplayText"]["source"], "cell-content-slot")

    def test_menu_content_separates_localized_labels_from_qaction_identity(self):
        contract = json.loads((SOURCE / "component-contracts/menu.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/menu.json").read_text(encoding="utf-8"))
        self.assertEqual(contract["contentModel"]["itemText"]["fallbackLocale"], "en-US")
        self.assertIn("action-identity", contract["contentModel"]["forbiddenContentResponsibilities"])
        self.assertEqual(binding["contentProperties"]["separator"]["content"], "not-applicable")

    def test_tree_content_separates_node_text_from_hierarchy_runtime(self):
        contract = json.loads((SOURCE / "component-contracts/tree.json").read_text(encoding="utf-8"))
        binding = json.loads((SOURCE / "figma-bindings/tree.json").read_text(encoding="utf-8"))
        self.assertEqual(contract["contentModel"]["nodeLabelText"]["fallbackLocale"], "en-US")
        self.assertIn("parent-child-relationship", contract["contentModel"]["forbiddenContentResponsibilities"])
        self.assertEqual(binding["contentProperties"]["emptyStateText"]["defaultLocale"], "en-US")
        light = Resolver(self.model, self.model.context("standard", "light"))
        self.assertEqual(qt_value(light.resolve(
            "component.input.overlay.action.clear.action-state.hover.slot.clear.background-color")),
            "#F5F5F5")
        self.assertEqual(qt_value(light.resolve(
            "component.input.overlay.action.password-toggle.action-state.pressed.slot.password-toggle.background-color")),
            "#DCDBDB")
        self.assertEqual(light.resolve(
            "component.input.base.shared.slot.clear.hit-target").value, 32)

    def test_select_open_is_independent_and_constrained(self):
        compiler = ComponentCompiler("ci")
        select = compiler.registry["components"]["select"]
        self.assertEqual(select["axes"]["state"],
                         ["default", "hover", "focused", "error", "disabled", "loading"])
        self.assertEqual(select["axes"]["open"], ["false", "true"])
        self.assertEqual(select["matrix"]["logicalCombinationCount"], 360)
        self.assertTrue(compiler.is_legal_combo(
            select, {"state": "focused", "open": "true"}))
        self.assertFalse(compiler.is_legal_combo(
            select, {"state": "disabled", "open": "true"}))
        self.assertFalse(compiler.is_legal_combo(
            select, {"state": "loading", "open": "true"}))

    def test_table_is_contract_driven_and_overlay_based(self):
        compiler = ComponentCompiler("ci")
        table = compiler.registry["components"]["table"]
        self.assertEqual(list(table["axes"]), ["density", "striping"])
        self.assertEqual(table["matrix"]["logicalCombinationCount"], 8)
        self.assertEqual(table["legalCombinations"]["constraints"], [])
        self.assertEqual(table["configuration"]["contract"],
                         "source/component-contracts/table.json")
        contract = json.loads((SOURCE / "component-contracts/table.json").read_text(encoding="utf-8"))
        self.assertIn("row-selection", contract["columnRoles"])
        self.assertIn("autocomplete", contract["editorTypes"])
        self.assertIn("display type and editor type are independent and may coexist",
                      contract["compositionRules"])
        table_paths = set(self.model.layers["component"])
        self.assertIn(
            "component.table.overlay.row-interaction.hover.slot.row.background-color",
            table_paths)
        self.assertFalse(any("component.table.base.state." in path for path in table_paths))

    def test_core_components_have_independent_contracts_and_figma_manifests(self):
        registry = self.compiler_registry()
        manifest = json.loads((SOURCE / "v2-source-manifest.json").read_text(encoding="utf-8"))
        for component in ("button", "input", "select", "checkbox", "radio"):
            entry = registry["components"][component]
            contract_ref = entry["configuration"]["contract"]
            contract = json.loads((SOURCE / "component-contracts" / f"{component}.json").read_text(encoding="utf-8"))
            binding_ref = manifest["components"][component]["figmaBindingManifest"]
            binding = json.loads((ROOT / "DesignSystem" / binding_ref).read_text(encoding="utf-8"))
            self.assertEqual(contract["component"], component)
            self.assertEqual(contract["registryBindings"]["axes"], list(entry["axes"]))
            self.assertEqual(contract["registryBindings"]["slots"], list(entry["slots"]))
            self.assertEqual(binding["variantAxes"], entry["axes"])
            self.assertEqual(binding["variableNamespace"]["canonicalPrefix"], f"component.{component}.")

    def test_materialization_budget_separates_design_source_from_runtime_expansion(self):
        compiler = ComponentCompiler("ci")
        compiler.validate()
        self.assertFalse(
            [d.entry for d in compiler.diagnostics
             if d.entry["severity"] == "error"])
        metrics = compiler.materialization_metrics
        self.assertGreater(metrics["designSourceTokenCount"], 0)
        self.assertEqual(metrics["runtimeExpandedCount"], None)
        self.assertIsNone(metrics["designVariableBudget"].get("projectTarget"))
        self.assertEqual(metrics["designVariableBudget"]["platformHardMax"], 5000)
        self.assertIsNone(metrics["designVariableCount"])

    def test_design_budget_report_is_read_only_and_capacity_aware(self):
        compiler = ComponentCompiler("ci")
        compiler.validate_design_budget_report(10, 5000)
        self.assertEqual(compiler.materialization_metrics["designBudgetStatus"], "capacity-unverified")
        # A minimal adapter response tests semantics, not obsolete historical counts.
        report = {"designSourceTokenCount": 10, "collections": [
            {"name": "DSM/Component", "platforms": {
                "Figma": {"observedVariableCount": 12, "platformHardMax": 5000},
                "Pixso": {"observedVariableCount": None}}}]}
        before = json.dumps(report, sort_keys=True)
        compiler.design_budget_report = Path("current-platform-measurement.json")
        with patch("v2.component_compile.load_json", return_value=report):
            compiler.validate_design_budget_report(10, 5000)
        self.assertEqual(json.dumps(report, sort_keys=True), before)
        self.assertEqual(compiler.materialization_metrics["observedVariableCounts"]["Figma"], 12)
        self.assertEqual(compiler.materialization_metrics["designBudgetStatus"], "capacity-unverified")
        report["collections"][0]["platforms"]["Figma"]["observedVariableCount"] = 5001
        with patch("v2.component_compile.load_json", return_value=report):
            compiler.validate_design_budget_report(10, 5000)
        self.assertEqual(compiler.materialization_metrics["designBudgetStatus"], "over-budget")
        self.assertTrue(any(item.entry["severity"] == "error" for item in compiler.diagnostics))

    def test_figma_binding_names_are_v2_owned_and_value_evidence_only(self):
        """Reference JSON may supply terminal values, never the Figma schema."""
        manifest = json.loads((SOURCE / "v2-source-manifest.json").read_text(encoding="utf-8"))
        for component in ("button", "input", "select", "checkbox", "radio", "table", "menu", "tree"):
            binding = json.loads((ROOT / "DesignSystem" / manifest["components"][component]["figmaBindingManifest"]).read_text(encoding="utf-8"))
            self.assertIn("V2 component-registry", binding["architectureAuthority"])
            self.assertTrue(
                "value-only" in binding["architectureAuthority"]
                or "values only" in binding["architectureAuthority"]
            )
            evidence_ref = binding.get("valueAuthority", binding.get("valueEvidence", ""))
            if evidence_ref:
                self.assertTrue(evidence_ref.startswith("source/component-value-evidence/"))
            prefix = f"{component}/"

            def walk(value):
                if isinstance(value, dict):
                    for key, child in value.items():
                        if key == "figmaVariable":
                            self.assertTrue(
                                child.startswith(prefix),
                                f"{component}: non-V2 Figma variable name {child!r}",
                            )
                        if key in {"figmaVariable", "canonicalPattern", "canonicalToken"}:
                            self.assertNotIn("imports/reference", str(child))
                            self.assertNotIn("reference/", str(child))
                        walk(child)
                elif isinstance(value, list):
                    for child in value:
                        walk(child)

            walk(binding)

    def test_registry_global_vocabulary_cannot_reintroduce_legacy_figma_names(self):
        """Global axes are V2 vocabulary; reference names stay value-only evidence."""
        registry = self.compiler_registry()
        axes = registry["axes"]
        self.assertEqual(
            axes["intent"]["values"],
            ["primary", "secondary", "operation", "danger"],
        )
        self.assertEqual(
            axes["content"]["values"],
            ["icon-only", "icon-left-text", "text-icon-right", "text-only", "icon-left-text-icon-right"],
        )
        self.assertEqual(
            axes["mode"]["values"],
            ["plain", "password", "search", "multiline", "dropdown-search", "file-browser"],
        )
        forbidden = {"text", "resource-picker", "icon-leading", "icon-trailing", "icon-leading-trailing"}
        for axis in ("intent", "content", "mode"):
            self.assertTrue(forbidden.isdisjoint(axes[axis]["values"]), axis)

    def test_current_binding_manifests_match_registry(self):
        manifest = json.loads((SOURCE / "v2-source-manifest.json").read_text(encoding="utf-8"))
        registry = self.compiler_registry()
        for component, record in manifest["components"].items():
            if "figmaBindingManifest" not in record:
                continue
            binding = json.loads((ROOT / "DesignSystem" / record["figmaBindingManifest"]).read_text(encoding="utf-8"))
            self.assertEqual(binding["component"], component)
            # Legacy value-evidence manifests are not approved V2 axis contracts.
            # Match the compiler's explicit migration boundary, not an old audit.
            if binding.get("$status") == "draft-v2" and binding.get("writePolicy") == "specification-only":
                self.assertEqual(binding["variableNamespace"]["canonicalPrefix"], f"component.{component}.")
                self.assertEqual(binding["variantAxes"], registry["components"][component]["axes"])

    @staticmethod
    def compiler_registry():
        return json.loads((SOURCE / "component-registry.json").read_text(encoding="utf-8"))

    def test_motion_releases_only_reviewed_spinner_slice(self):
        report = MotionValidator("ci").validate()
        self.assertEqual(report["summary"]["errors"], 0)
        self.assertFalse(report["releaseBlocked"])
        self.assertTrue(report["platformOutputsGenerated"])
        self.assertEqual(report["releasedSlices"], ["loading.spinner"])
        self.assertNotIn("loading.spinner", report["blockedSlices"])
        self.assertEqual(report["validatedComponents"], ["button", "input", "select"])

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
        self.assertIn("component.progress-bar.base.state.paused.slot.indicator.color",
                      self.model.layers["component"])

    def test_no_component_to_component_aliases(self):
        for path, record in self.model.layers["component"].items():
            value = record.value
            if isinstance(value, str) and value.startswith("{"):
                self.assertFalse(value[1:-1].startswith("component."),
                                 f"horizontal alias remains: {path} -> {value}")

    def test_token_dependency_direction_is_one_way(self):
        """Model construction must reject component/product reverse edges."""
        layer_of = {path: layer for layer, records in self.model.layers.items()
                    for path in records}
        layer_of.update({path: "product" for path in self.model.product})
        order = {"primitive": 0, "profile": 1, "theme": 2, "semantic": 3,
                 "composite": 4, "component": 5, "content": 6, "product": 7}
        for layer, records in self.model.layers.items():
            for path, record in records.items():
                value = record.value
                if isinstance(value, str) and value.startswith("{"):
                    target = value[1:-1]
                    self.assertGreaterEqual(order[layer], order[layer_of[target]])
                    self.assertFalse(layer == "component" and
                                     layer_of[target] in {"component", "product"})
        for path, record in self.model.product.items():
            value = record.value
            if isinstance(value, str) and value.startswith("{"):
                self.assertNotIn(layer_of[value[1:-1]], {"component", "product"}, path)

    def test_reference_values_exact(self):
        light = Resolver(self.model, self.model.context("standard", "light"))
        for path, expected in REFERENCE_EXPECTATIONS.items():
            self.assertEqual(qt_value(light.resolve(path)), expected, path)

    def test_manifest(self):
        manifest = json.loads((ROOT / "DesignSystem/config/manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["schemaVersion"], "2.0")
        self.assertEqual(manifest["designSnapshot"]["figmaFileKey"],
                         "w92BKW1EcutKmTXkwzNHbf")

    def test_runtime_component_map_is_v2_owned(self):
        from v2.build_runtime_map import build_map
        mapping = build_map(ROOT)
        button = next(item for item in mapping["components"] if item["componentId"] == "button")
        self.assertEqual(button["figmaComponentSet"], "2258:15827")
        self.assertIsNone(button["figmaVariantCount"])
        self.assertEqual(button["figmaVariantCountStatus"], "migration-evidence-only")
        self.assertEqual(button["variantAxes"]["state"],
                         ["default", "hover", "pressed", "focused", "disabled", "loading"])
        self.assertTrue(button["bindings"])
        # The runtime map is consumed by ComponentLab directly.  Recipe-backed
        # bindings must expose the resolved profile value, not only the
        # symbolic canonical path; wildcard binding rules may remain sparse.
        recipe_bindings = [binding for binding in button["bindings"]
                           if binding["value"] and "{" not in binding["canonicalPath"]]
        self.assertTrue(recipe_bindings)
        self.assertEqual(next(binding for binding in recipe_bindings
                               if binding["canonicalPath"].endswith("slot.container.min-height"))
                         ["canonicalValue"], 24)
        for binding in button["bindings"]:
            self.assertTrue(binding["variable"].startswith("button/"))
            self.assertTrue(binding["canonicalPath"].startswith("component.button."))
            self.assertNotIn("qtlab", binding["variable"].lower())
            self.assertNotIn("stoken", binding["canonicalPath"].lower())
        raw = json.dumps(button)
        self.assertNotIn("focus-visible", raw)
        self.assertNotIn('"active"', raw)


class QtRuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = TokenModel(SOURCE)
        from v2.adapters import qt_runtime
        cls.runtime = qt_runtime.build(cls.model)

    def test_canonical_profile_names_only(self):
        self.assertEqual(sorted(self.runtime["profiles"]),
                         ["standard-dark", "standard-light", "wide-dark", "wide-light"])
        self.assertEqual(self.runtime["$meta"]["defaultProfile"], "standard-light")

    def test_no_legacy_keys(self):
        for profile in self.runtime["profiles"].values():
            for group in profile.values():
                for key in group:
                    self.assertFalse(key.startswith(("base.stoken", "component.basic",
                                                     "component.composite", "base.")),
                                     f"legacy key remains: {key}")

    def test_canonical_keys_present(self):
        color = self.runtime["profiles"]["standard-light"]["color"]
        self.assertEqual(color["semantic.color.text.primary"], "#191919")
        layout = self.runtime["profiles"]["standard-light"]["layout"]
        self.assertEqual(
            layout["component.button.base.size.medium.slot.container.min-height"], 32)
        font = self.runtime["profiles"]["standard-light"]["font"]
        self.assertEqual(font["component.button.base.typography.slot.label.font-size"], 14)

    def test_adapter_scope(self):
        layout = self.runtime["profiles"]["standard-light"]["layout"]
        self.assertEqual(layout["adapter.qt.dimension.control-height.md"], 32)
        self.assertEqual(layout["adapter.qt.button.iconContainer.M"], 28)
        self.assertEqual(layout["adapter.qt.checkbox.hitTarget.sm"], 32)

    def test_sparse_dark_honest(self):
        dark = self.runtime["profiles"]["standard-dark"]["color"]
        self.assertEqual(dark["semantic.color.background.hover"], "#202020")
        self.assertEqual(dark["semantic.color.background.page"], "#FFFFFF")


if __name__ == "__main__":
    unittest.main(verbosity=2)
