"""Regression checks for the four-layer DSM sToken design-mode contract."""

import copy
import importlib.util
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("stoken_import", Path(__file__).with_name("stoken_import.py"))
STOKEN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(STOKEN)


def source_collections():
    source = STOKEN.load(ROOT / "DesignSystem/imports/stoken.json")
    return {
        collection: {mode: values for item in modes for mode, values in item.items()}
        for entry in source for collection, modes in entry.items()
    }


class STokenModeContractTests(unittest.TestCase):
    def setUp(self):
        self.collections = source_collections()

    def test_current_source_has_exactly_four_layers_and_independent_mode_axes(self):
        STOKEN.validate_collection_axes(self.collections)
        self.assertEqual(STOKEN.LAYERS, tuple(self.collections))
        self.assertEqual(("Standard", "Wide"), tuple(self.collections["02.profile"]))
        self.assertEqual(("Light", "Dark"), tuple(self.collections["03.semantic"]))
        self.assertEqual("Standard", STOKEN.load(ROOT / "tokens/stoken-map.json")["deviceProfile"])
        self.assertEqual("var(--dimension/dimension-1920)",
                         self.collections["02.profile"]["Standard"]["--viewport/width"])
        self.assertEqual("var(--dimension/dimension-2560)",
                         self.collections["02.profile"]["Wide"]["--viewport/width"])

    def test_primitive_and_component_cannot_gain_hidden_extra_modes(self):
        for layer in ("01.primitive", "04.component"):
            with self.subTest(layer=layer):
                changed = copy.deepcopy(self.collections)
                changed[layer]["Extra"] = {}
                with self.assertRaisesRegex(ValueError, layer):
                    STOKEN.validate_collection_axes(changed)

    def test_fifth_design_token_layer_is_rejected(self):
        changed = copy.deepcopy(self.collections)
        changed["05.runtime"] = {"Value": {}}
        with self.assertRaisesRegex(ValueError, "collections must be exactly"):
            STOKEN.validate_collection_axes(changed)

    def test_profile_and_semantic_cannot_gain_cross_product_modes(self):
        for layer in ("02.profile", "03.semantic"):
            with self.subTest(layer=layer):
                changed = copy.deepcopy(self.collections)
                changed[layer]["Compact"] = {}
                with self.assertRaisesRegex(ValueError, layer):
                    STOKEN.validate_collection_axes(changed)

    def test_profile_modes_must_share_role_names(self):
        changed = copy.deepcopy(self.collections)
        changed["02.profile"]["Wide"].pop(next(iter(changed["02.profile"]["Wide"])))
        with self.assertRaisesRegex(ValueError, "Profile role names"):
            STOKEN.validate_collection_axes(changed)

    def test_sparse_dark_cannot_invent_a_role_without_light_counterpart(self):
        changed = copy.deepcopy(self.collections)
        changed["03.semantic"]["Dark"]["--color/text/unpaired"] = "var(--color/neutral/gray-5)"
        with self.assertRaisesRegex(ValueError, "Light counterparts"):
            STOKEN.validate_collection_axes(changed)

    def test_tool_button_profile_geometry_and_foreground_keep_independent_axes(self):
        for profile, icon_size, label_size in (("Standard", 20, 14), ("Wide", 24, 18)):
            for theme in ("Light", "Dark"):
                with self.subTest(profile=profile, theme=theme):
                    values = {
                        **self.collections["01.primitive"]["参数"],
                        **self.collections["02.profile"][profile],
                        **self.collections["03.semantic"]["Light"],
                        **self.collections["04.component"]["default"],
                    }
                    if theme == "Dark":
                        values.update(self.collections["03.semantic"]["Dark"])

                    def resolve(name, seen=()):
                        self.assertNotIn(name, seen, "cyclic source alias")
                        value = values[name]
                        match = STOKEN.VAR.fullmatch(str(value))
                        return resolve(match[1], (*seen, name)) if match else value

                    self.assertEqual(icon_size, resolve("--tool-button/base/default/icon/size"))
                    self.assertEqual(label_size, resolve("--tool-button/base/default/label/font-size"))
                    self.assertEqual(
                        "rgba(25, 25, 25, 1)" if theme == "Light" else "rgba(243, 243, 243, 1)",
                        resolve("--tool-button/base/default/foreground/color"),
                    )
                    self.assertEqual(
                        "rgba(243, 243, 243, 1)" if theme == "Light" else "rgba(48, 48, 48, 1)",
                        resolve("--tool-button/base/disabled/container/background"),
                    )
                    self.assertEqual(
                        "rgba(255, 255, 255, 1)" if theme == "Light" else "rgba(39, 39, 39, 1)",
                        resolve("--tool-button/base/default/container/background"),
                    )
                    self.assertEqual(
                        "rgba(167, 167, 167, 1)" if theme == "Light" else "rgba(103, 103, 103, 1)",
                        resolve("--tool-button/base/default/border/color"),
                    )
                    self.assertEqual(
                        "rgba(255, 255, 255, 0)",
                        resolve("--tool-button/base/auto-raise/container/background"),
                    )
                    self.assertEqual(
                        "rgba(255, 255, 255, 0)",
                        resolve("--tool-button/base/auto-raise/border/color"),
                    )

    def test_checkbox_and_review_color_roles_preserve_light_dark_under_both_profiles(self):
        expected = {
            "--color/text/muted": (
                "rgba(68, 68, 68, 1)", "rgba(198, 198, 198, 1)"),
            "--checkbox/base/default/label/color": (
                "rgba(25, 25, 25, 1)", "rgba(243, 243, 243, 1)"),
            "--checkbox/base/default/indicator/background": (
                "rgba(255, 255, 255, 1)", "rgba(39, 39, 39, 1)"),
            "--checkbox/base/default/indicator/border-color": (
                "rgba(167, 167, 167, 1)", "rgba(103, 103, 103, 1)"),
            "--checkbox/base/disabled/indicator/background": (
                "rgba(243, 243, 243, 1)", "rgba(48, 48, 48, 1)"),
            "--checkbox/base/disabled/indicator/border-color": (
                "rgba(208, 208, 208, 1)", "rgba(68, 68, 68, 1)"),
        }
        for profile in ("Standard", "Wide"):
            for theme_index, theme in enumerate(("Light", "Dark")):
                with self.subTest(profile=profile, theme=theme):
                    values = {
                        **self.collections["01.primitive"]["参数"],
                        **self.collections["02.profile"][profile],
                        **self.collections["03.semantic"]["Light"],
                        **self.collections["04.component"]["default"],
                    }
                    if theme == "Dark":
                        values.update(self.collections["03.semantic"]["Dark"])

                    def resolve(name, seen=()):
                        self.assertNotIn(name, seen, "cyclic source alias")
                        value = values[name]
                        match = STOKEN.VAR.fullmatch(str(value))
                        return resolve(match[1], (*seen, name)) if match else value

                    for role, mode_values in expected.items():
                        self.assertEqual(mode_values[theme_index], resolve(role), role)

    def test_menu_bar_color_roles_preserve_approved_light_dark_values(self):
        expected = {
            "--color/background/hover": (
                "rgba(243, 243, 243, 1)", "rgba(32, 32, 32, 1)"),
            "--menu-bar/base/default/container/color": (
                "rgba(255, 255, 255, 1)", "rgba(39, 39, 39, 1)"),
            "--menu-bar/base/default/border/color": (
                "rgba(208, 208, 208, 1)", "rgba(68, 68, 68, 1)"),
            "--menu-bar-item/base/default/label/color": (
                "rgba(25, 25, 25, 1)", "rgba(243, 243, 243, 1)"),
            "--menu-bar-item/base/disabled/label/color": (
                "rgba(138, 138, 138, 1)", "rgba(138, 138, 138, 1)"),
            "--menu-bar-item/base/hover/container/color": (
                "rgba(243, 243, 243, 1)", "rgba(32, 32, 32, 1)"),
            "--menu-bar-item/base/open/container/color": (
                "rgba(98, 53, 247, 1)", "rgba(98, 53, 247, 1)"),
            "--menu-bar-item/base/focused/focus-ring/color": (
                "rgba(98, 53, 247, 1)", "rgba(98, 53, 247, 1)"),
            "--menu-bar-item/base/default/mnemonic/color": (
                "rgba(98, 53, 247, 1)", "rgba(98, 53, 247, 1)"),
        }
        for profile in ("Standard", "Wide"):
            for theme_index, theme in enumerate(("Light", "Dark")):
                with self.subTest(profile=profile, theme=theme):
                    values = {
                        **self.collections["01.primitive"]["参数"],
                        **self.collections["02.profile"][profile],
                        **self.collections["03.semantic"]["Light"],
                        **self.collections["04.component"]["default"],
                    }
                    if theme == "Dark":
                        values.update(self.collections["03.semantic"]["Dark"])

                    def resolve(name, seen=()):
                        self.assertNotIn(name, seen, "cyclic source alias")
                        value = values[name]
                        match = STOKEN.VAR.fullmatch(str(value))
                        return resolve(match[1], (*seen, name)) if match else value

                    for role, mode_values in expected.items():
                        self.assertEqual(mode_values[theme_index], resolve(role), role)


if __name__ == "__main__":
    unittest.main()
