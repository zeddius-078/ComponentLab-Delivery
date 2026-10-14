import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "component-definitions/generated-design-component-catalog.json"
TRACEABILITY = ROOT.parents[1] / "docs/designRules/design-system/component-traceability-manifest.json"


class FigmaCatalogContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
        cls.traceability = json.loads(TRACEABILITY.read_text(encoding="utf-8"))

    def test_catalog_uses_figma_page_hierarchy(self):
        pages = self.catalog["figmaPages"]
        self.assertEqual(self.catalog["figmaSource"]["pageCount"], len(pages))
        self.assertEqual("20.1.01 · Navbar / 导航栏", pages[0]["name"])
        self.assertIn("20.10.03 · ModelAsset / 模型资产", {page["name"] for page in pages})

    def test_checkbox_sets_and_variants_match_figma(self):
        page = next(page for page in self.catalog["figmaPages"] if page["id"] == "234:2717")
        self.assertEqual(["DSM/Checkbox", "DSM/Checkbox/Group"],
                         [item["name"] for item in page["componentSets"]])
        checkbox = page["componentSets"][0]
        self.assertEqual(
            {
                "Size": ["LG", "SM"],
                "Value": ["Unchecked", "Checked", "Indeterminate"],
                "State": ["Default", "Hover", "Focus-visible", "Disabled", "Pressed"],
            },
            checkbox["variantAxes"],
        )
        self.assertEqual(30, len(checkbox["variants"]))

    def test_catalog_keeps_every_live_figma_component_set(self):
        sets = [item for page in self.catalog["figmaPages"] for item in page["componentSets"]]
        variants = [variant for item in sets for variant in item["variants"]]
        self.assertEqual(self.catalog["figmaSource"]["componentSetCount"], len(sets))
        self.assertEqual(self.catalog["figmaSource"]["variantCount"], len(variants))

    def test_every_component_set_is_canonical_or_explicitly_classified(self):
        governance = self.catalog["componentSetGovernance"]
        statistics = self.catalog["statistics"]
        all_set_ids = {
            item["id"]
            for page in self.catalog["figmaPages"]
            for item in page["componentSets"]
        }
        canonical_set_ids = {
            component_set_id
            for entry in self.catalog["entries"]
            for component_set_id in entry["componentSetId"].split(",")
            if component_set_id
        }
        classified_set_ids = {
            item["componentSetId"] for item in governance["unreferencedSets"]
        }
        self.assertFalse(canonical_set_ids & classified_set_ids)
        self.assertEqual(all_set_ids, canonical_set_ids | classified_set_ids)
        self.assertEqual(len(all_set_ids), governance["figmaComponentSetCount"])
        self.assertEqual(len(canonical_set_ids), governance["canonicalReferencedComponentSetCount"])
        self.assertEqual(len(classified_set_ids), governance["classifiedUnreferencedComponentSetCount"])
        self.assertEqual(17, statistics["internalComponentSetCount"])
        self.assertEqual(1, statistics["proposalComponentSetCount"])
        self.assertEqual(1, statistics["residueComponentSetCount"])

    def test_public_composites_do_not_claim_internal_anatomy_sets(self):
        entries = {entry["canonicalId"]: entry for entry in self.catalog["entries"]}
        expected = {
            "ComponentLab/Menu": ("482:491", "482:490", 20),
            "ComponentLab/MenuBar": ("487:23", "487:22", 5),
            "ComponentLab/TabWidget": ("500:849", "500:847", 20),
            "ComponentLab/ToolBox": ("505:555", "505:553", 5),
        }
        self.assertEqual(
            {canonical_id: values[0] for canonical_id, values in expected.items()},
            self.catalog["componentSetGovernance"]["publicIdentityNodes"],
        )
        for canonical_id, (node_id, variant_source_id, variant_count) in expected.items():
            entry = entries[canonical_id]
            self.assertEqual("", entry["componentSetId"])
            self.assertEqual([node_id], entry["figmaNodeIds"])
            self.assertEqual(node_id, entry["publicDesignNodeId"])
            self.assertEqual(variant_source_id, entry["variantSourceComponentSetId"])
            self.assertEqual(variant_count, entry["variantCount"])

        internal = {
            item["componentSetId"]: item
            for item in self.catalog["componentSetGovernance"]["unreferencedSets"]
            if item["role"] == "INTERNAL"
        }
        self.assertEqual("ComponentLab/Menu", internal["482:490"]["ownerCanonicalId"])
        self.assertEqual("ComponentLab/MenuBar", internal["487:22"]["ownerCanonicalId"])
        self.assertEqual("ComponentLab/TabWidget", internal["500:847"]["ownerCanonicalId"])
        self.assertEqual("ComponentLab/ToolBox", internal["505:553"]["ownerCanonicalId"])

    def test_proposal_and_residue_do_not_create_public_identities(self):
        governance = self.catalog["componentSetGovernance"]
        records = {
            item["componentSetId"]: item for item in governance["unreferencedSets"]
        }
        proposal = records["1260:738"]
        self.assertEqual("PROPOSAL", proposal["role"])
        self.assertEqual("ComponentLab/Button", proposal["ownerCanonicalId"])
        self.assertEqual(
            1,
            sum(entry["canonicalId"] == "ComponentLab/Button" for entry in self.catalog["entries"]),
        )
        self.assertNotIn(
            "1260:738",
            next(
                entry for entry in self.catalog["entries"]
                if entry["canonicalId"] == "ComponentLab/Button"
            )["componentSetId"].split(","),
        )

        residue = records["2016:18715"]
        self.assertEqual("RESIDUE", residue["role"])
        self.assertEqual("482:490", residue["duplicateOfComponentSetId"])
        self.assertEqual(
            "INTERNAL",
            records[residue["duplicateOfComponentSetId"]]["role"],
        )
        self.assertNotIn(
            "ComponentLab/MenuItem",
            {entry["canonicalId"] for entry in self.catalog["entries"]},
        )

    def test_library_counts_are_derived_by_kind_and_runtime_evidence(self):
        entries = self.catalog["entries"]
        components = self.catalog["components"]
        statistics = self.catalog["statistics"]
        self.assertEqual(len(entries), self.catalog["canonicalLibraryEntryCount"])
        self.assertEqual(len(components), self.catalog["componentCount"])
        self.assertEqual(
            len(components),
            statistics["widgetCount"] + statistics["compositeCount"],
        )
        self.assertEqual(
            len(entries),
            len(components) + statistics["layoutHelperCount"] + statistics["assetEntryCount"],
        )
        self.assertEqual(
            self.catalog["instantiableCount"],
            sum(component["instantiable"] for component in components),
        )
        self.assertEqual(
            self.catalog["contractOnlyCount"],
            sum(not component["instantiable"] for component in components),
        )
        self.assertEqual(2, statistics["implementedLayoutHelperCount"])
        self.assertEqual(0, statistics["unmappedAssetCount"])
        self.assertEqual(
            sum(component["engineeringStatus"] == "UNMAPPED" for component in components),
            statistics["componentEngineeringGapCount"],
        )
        self.assertEqual(
            statistics["engineeringGapCount"],
            statistics["componentEngineeringGapCount"] + statistics["unmappedAssetCount"],
        )
        self.assertEqual(set(), {
            component["designComponent"]
            for component in components
            if component["engineeringStatus"] == "REGISTERED"
            and (not component["instantiable"] or not component["sdkClass"])
        })
        self.assertEqual(set(), {
            component["designComponent"]
            for component in components
            if component["engineeringStatus"] == "UNMAPPED"
            and (component["instantiable"] or component["sdkClass"])
        })
        self.assertEqual(
            {"IMPLEMENTED"},
            {entry["engineeringStatus"] for entry in entries if entry["kind"] == "LayoutHelper"},
        )
        asset_status = {
            entry["canonicalId"]: entry["engineeringStatus"]
            for entry in entries if entry["kind"] == "Asset"
        }
        self.assertEqual("IMPLEMENTED", asset_status["ComponentLab/IconAsset"])
        self.assertEqual("IMPLEMENTED", asset_status["ComponentLab/ImageAsset"])
        self.assertEqual("IMPLEMENTED", asset_status["ComponentLab/ModelAsset"])
        self.assertEqual(
            "SDK_ASSETCATALOG_COMPONENTLAB_GENERATELAYOUT",
            next(entry for entry in entries
                 if entry["canonicalId"] == "ComponentLab/IconAsset")
                ["verificationStatus"]["runtimeScope"],
        )
        self.assertEqual(
            {"SDK_VISUALASSETCATALOG_COMPONENTLAB_GENERATELAYOUT"},
            {entry["verificationStatus"]["runtimeScope"]
             for entry in entries
             if entry["canonicalId"] in
             {"ComponentLab/ImageAsset", "ComponentLab/ModelAsset"}},
        )
        self.assertEqual(
            {"FACTORY_INSTANTIATION_ONLY", "SDK_FACTORY_COMPONENTLAB_GENERATELAYOUT"},
            {entry["verificationStatus"]["runtimeScope"]
             for entry in entries if entry["engineeringStatus"] == "REGISTERED"},
        )
        self.assertEqual(
            {"ComponentLab/Button", "ComponentLab/Checkbox", "ComponentLab/Input",
             "ComponentLab/Badge", "ComponentLab/StatusIndicator",
             "ComponentLab/Link", "ComponentLab/Tag", "ComponentLab/Tooltip",
             "ComponentLab/SearchInput", "ComponentLab/NumberInput",
             "ComponentLab/OverflowMenuButton", "ComponentLab/SegmentedControl",
             "ComponentLab/Timeline", "ComponentLab/Stepper", "ComponentLab/Pagination",
             "ComponentLab/PageHeader", "ComponentLab/TitleBar",
             "ComponentLab/AnchorNavigation", "ComponentLab/Filter",
             "ComponentLab/NumericKeypad", "ComponentLab/ColorPicker",
             "ComponentLab/Transfer", "ComponentLab/Upload",
             "ComponentLab/Menu", "ComponentLab/MenuBar", "ComponentLab/ToolBar",
             "ComponentLab/TabWidget", "ComponentLab/ToolBox",
             "ComponentLab/DockWidget", "ComponentLab/MdiSubWindow",
             "ComponentLab/MdiArea", "ComponentLab/StackedWidget",
             "ComponentLab/WizardPage", "ComponentLab/Wizard",
             "ComponentLab/Label", "ComponentLab/ProgressBar",
             "ComponentLab/Navbar", "ComponentLab/Sidebar",
             "ComponentLab/DesktopBottomNavigation"},
            {entry["canonicalId"] for entry in entries
             if entry["verificationStatus"]["runtimeScope"] ==
             "SDK_FACTORY_COMPONENTLAB_GENERATELAYOUT"},
        )
        for canonical_id in (
            "ComponentLab/Button", "ComponentLab/Checkbox", "ComponentLab/Input"
        ):
            entry = next(item for item in entries if item["canonicalId"] == canonical_id)
            self.assertEqual(
                "VERIFIED", entry["verificationStatus"]["downstreamConsumption"]
            )
            self.assertIn(
                "DesignToCode_GenerateLayout/Source/Validation/component/ComponentTests.cpp",
                entry["evidenceRefs"],
            )
        self.assertEqual(
            {"LAYOUT_CONVERTER_UNIT"},
            {entry["verificationStatus"]["runtimeScope"]
             for entry in entries if entry["kind"] == "LayoutHelper"},
        )

    def test_public_cpp_inventory_exposes_non_widget_library_entries(self):
        header = (ROOT / "runtime/component/DesignComponentCatalog.h").read_text(encoding="utf-8")
        source = (ROOT / "runtime/component/DesignComponentCatalog.cpp").read_text(encoding="utf-8")
        self.assertIn("struct DesignLibraryEntry", header)
        self.assertEqual(2, header.count("QString publicDesignNodeId;"))
        self.assertEqual(2, header.count("QString variantSourceComponentSetId;"))
        self.assertIn("QList<DesignLibraryEntry> designLibraryEntries();", header)
        self.assertIn("QList<DesignLibraryEntry> designLibraryEntries()", source)
        for canonical_id in (
            "ComponentLab/Spacer",
            "ComponentLab/LayoutContainer",
            "ComponentLab/IconAsset",
            "ComponentLab/ImageAsset",
            "ComponentLab/ModelAsset",
        ):
            self.assertIn(canonical_id, source)

    def test_core_interactive_components_have_explicit_optional_focus_contracts(self):
        by_id = {entry["canonicalId"]: entry for entry in self.catalog["entries"]}
        expected = {
            "ComponentLab/Button": "STRONG_FOCUS_OPTIONAL_INDICATOR",
            "ComponentLab/Checkbox": "STRONG_FOCUS_OPTIONAL_INDICATOR",
            "ComponentLab/Input": "CHILD_EDITOR_ONLY_OPTIONAL_INDICATOR",
        }
        for canonical_id, policy in expected.items():
            entry = by_id[canonical_id]
            self.assertEqual(policy, entry["focusPolicy"])
            self.assertIn(
                "focusIndicatorEnabled",
                entry["runtimeContract"]["controlledProperties"],
            )
            self.assertIn(
                "Source/ComponentKit/runtime/component/FocusIndicator.cpp",
                entry["evidenceRefs"],
            )

    def test_native_display_components_are_presentation_only_full_chain_contracts(self):
        by_id = {entry["canonicalId"]: entry for entry in self.catalog["entries"]}
        expected = {
            "ComponentLab/Label": "Source/ComponentKit/runtime/component/LabelAppearance.cpp",
            "ComponentLab/ProgressBar": "Source/ComponentKit/runtime/component/ProgressAppearance.cpp",
        }
        for canonical_id, evidence in expected.items():
            entry = by_id[canonical_id]
            self.assertEqual("NO_FOCUS_PRESENTATION", entry["focusPolicy"])
            self.assertEqual(
                "SDK_FACTORY_COMPONENTLAB_GENERATELAYOUT",
                entry["verificationStatus"]["runtimeScope"],
            )
            self.assertEqual(
                "VERIFIED", entry["verificationStatus"]["downstreamConsumption"]
            )
            self.assertIn(evidence, entry["evidenceRefs"])

    def test_traceability_actions_and_engineering_statuses_do_not_collapse(self):
        trace = self.traceability
        self.assertEqual(trace["counts"]["merge"], sum(
            item["governanceDecision"] == "MERGE" for item in trace["components"]
        ))
        self.assertEqual(trace["counts"]["built"], sum(
            item["governanceDecision"] == "BUILT" for item in trace["components"]
        ))
        by_id = {item["canonicalId"]: item for item in self.catalog["entries"]}
        drawer = by_id["ComponentLab/Drawer"]
        self.assertEqual("RENAME", drawer["identityMigration"])
        self.assertEqual("REGISTERED", drawer["engineeringStatus"])
        self.assertEqual("commonWidget::Z_Drawer", drawer["sdkClass"])
        self.assertEqual("LEGACY_COMPATIBILITY", drawer["compatibility"]["status"])
        self.assertEqual("commonWidget::Z_OverlayDrawer",
                         drawer["compatibility"]["implementationClass"])
        self.assertNotIn("ComponentLab/OverlayDrawer", by_id)

    def test_public_source_counts_use_current_inventory_without_new_widget_types(self):
        sets = {item["id"]: item for page in self.catalog["figmaPages"] for item in page["componentSets"]}
        for component in self.catalog["components"]:
            ids = [node_id for node_id in component["componentSetId"].split(",") if node_id]
            if component.get("publicDesignNodeId"):
                ids = component["variantSourceComponentSetId"].split(",")
                self.assertEqual(component["variantCount"],sum(len(sets[node_id]["variants"]) for node_id in ids))
            else:
                self.assertEqual(component["variantCount"],sum(len(sets[node_id]["variants"]) for node_id in ids))
        counts = {component["designComponent"]: component["variantCount"] for component in self.catalog["components"]}
        self.assertEqual(20,counts["ComponentLab/ProgressBar"])
        self.assertEqual(16,counts["ComponentLab/SpinBox"])
        self.assertEqual(16,counts["ComponentLab/DoubleSpinBox"])
        self.assertEqual(2,counts["ComponentLab/DesktopBottomNavigation"])
        self.assertNotIn("ComponentLab/MenuItem",counts)

    def test_progress_visual_evidence_matches_all_twenty_variants(self):
        doc = json.loads((ROOT / "component-definitions/figma-progress-visuals.json").read_text(encoding="utf-8"))
        page = next(page for page in self.catalog["figmaPages"] if page["id"]=="410:13")
        self.assertEqual({variant["name"] for variant in doc["variants"]},set(page["componentSets"][0]["variants"]))
        self.assertEqual(20,len(doc["variants"]))
        for variant in doc["variants"]:
            labels = [child for child in variant["children"] if child["type"]=="TEXT"]
            self.assertEqual(1,len(labels))
            self.assertEqual("State=Indeterminate" not in variant["name"],labels[0]["visible"])

    def test_button_captured_visuals_and_gallery_are_complete(self):
        document = json.loads((ROOT / "component-definitions/figma-button-visuals.json").read_text(encoding="utf-8"))
        self.assertEqual("2015:17140", document["galleryId"])
        self.assertEqual({"PRIMARY", "DEFAULT", "DASHED", "TEXT", "LINK", "DANGER", "ICONONLY"}, set(document["recipes"]))
        for recipe in document["recipes"].values():
            self.assertEqual({"default", "hover", "active", "focus-visible", "disabled", "loading"}, set(recipe))
        self.assertEqual([98, 53, 247, 1], document["recipes"]["PRIMARY"]["default"]["fill"])
        self.assertEqual([64, 150, 255, 1], document["recipes"]["DEFAULT"]["hover"]["stroke"])
        self.assertEqual([4, 3], document["recipes"]["DASHED"]["focus-visible"]["dash"])
        import xml.etree.ElementTree as ET
        assets = ROOT / "runtime/assets/icons/figma-button"
        self.assertEqual(8, len(list(assets.glob("*.svg"))))
        for asset in assets.glob("*.svg"):
            self.assertGreater(asset.stat().st_size, 100)
            svg = ET.parse(asset).getroot()
            self.assertEqual("16", svg.attrib["width"])
            self.assertEqual("16", svg.attrib["height"])

    def test_gallery_layout_evidence_preserves_gaps_and_node_geometry(self):
        document = json.loads((ROOT / "component-definitions/figma-gallery-layouts.json").read_text(encoding="utf-8"))
        self.assertEqual(self.catalog["figmaFileKey"], document["fileKey"])
        pages = {page["id"]: page for page in document["pages"]}
        current_page_ids = {page["id"] for page in self.catalog["figmaPages"]}
        self.assertTrue(set(pages) < current_page_ids)
        built_page_ids = {
            entry["pageId"] for entry in self.catalog["entries"]
            if entry["designOriginAction"] == "BUILD"
        }
        self.assertEqual(built_page_ids, current_page_ids - set(pages))
        missing = {key for key, page in pages.items() if page["status"] != "captured"}
        self.assertEqual(set(), missing)
        for key in missing:
            self.assertNotIn("galleries", pages[key])
            self.assertTrue(pages[key]["reason"])
        gallery = pages["234:2717"]["galleries"][0]
        self.assertEqual(["2015:20254", "Variant Gallery", "FRAME", 64, 1876, 1312, 991], gallery[:7])
        checkbox = gallery[7][1]
        self.assertEqual([206, 85, 900, 540], checkbox[3:7])
        self.assertEqual(30, len(checkbox[7]))
        self.assertEqual(4, len(gallery[7][2][7]))


if __name__ == "__main__":
    unittest.main()
