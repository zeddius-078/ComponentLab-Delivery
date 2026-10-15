"""Generate the public Figma component catalog consumed by both demo applications.

The current Figma ledger is the design evidence source. Native Qt construction remains
owned by native-components.json; this catalog must not turn a Figma-only contract into a
runtime implementation claim.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parents[1]
LEDGER = PROJECT / "docs/designRules/design-system/figma-core-rebuild-state.json"
NATIVE_DEFINITIONS = ROOT / "component-definitions/native-components.json"
OUTPUT_JSON = ROOT / "component-definitions/generated-design-component-catalog.json"
OUTPUT_HEADER = ROOT / "runtime/component/DesignComponentCatalog.h"
OUTPUT_SOURCE = ROOT / "runtime/component/DesignComponentCatalog.cpp"
FIGMA_INVENTORY = ROOT / "component-definitions/figma-component-inventory.json"
TRACEABILITY_MANIFEST = (
    PROJECT / "docs/designRules/design-system/component-traceability-manifest.json"
)

FIGMA_FILE_KEY = "w92BKW1EcutKmTXkwzNHbf"

CATEGORY_ORDER = [
    ("navigation", "导航"),
    ("display", "展示与编辑"),
    ("actions", "操作"),
    ("input", "输入"),
    ("selection", "选择"),
    ("container", "容器与布局"),
    ("data", "数据视图"),
    ("feedback", "反馈"),
    ("window", "窗口与系统"),
    ("assets", "资产"),
]

CATEGORY_ID_TO_KEY = {
    "20.1": "navigation",
    "20.2": "display",
    "20.3": "actions",
    "20.4": "input",
    "20.5": "selection",
    "20.6": "container",
    "20.7": "data",
    "20.8": "feedback",
    "20.9": "window",
    "20.10": "assets",
}

CATEGORY_COMPONENTS = {
    "navigation": {
        "Navbar", "Sidebar", "Menu", "MenuBar", "ToolBar", "TabWidget",
        "ToolBox", "DockWidget", "MdiArea", "MdiSubWindow", "StackedWidget",
        "WizardPage", "Wizard", "DesktopBottomNavigation",
    },
    "display": {
        "MetricCard", "Chart", "ImagePanel", "ProgressBar", "Label", "LCDNumber",
        "TextBrowser", "GraphicsView", "Calendar", "RichTextEdit", "PlainTextEdit",
        "OpenGLViewport", "Empty", "Statistic", "Descriptions",
    },
    "actions": {"Button", "ToolButton", "DialogButtonGroup", "CommandLinkButton"},
    "input": {
        "Input", "LineEdit", "SpinBox", "DoubleSpinBox", "DateTimePicker",
        "DatePicker", "TimePicker", "ShortcutInput", "Slider", "ScrollBar", "Dial",
        "Form",
    },
    "selection": {
        "Checkbox", "RadioButton", "Switch", "Chip", "Select", "ComboBox",
        "FontComboBox",
    },
    "container": {
        "Card", "IntegrationCard", "Splitter", "GroupBox", "ScrollArea", "Widget",
        "Frame", "Divider", "Collapse", "OverlayDrawer",
    },
    "data": {
        "DataTable", "TableWidget", "ListView", "ListWidget", "TreeView",
        "TreeWidget", "TableView", "ColumnView", "UndoView",
    },
    "feedback": {"MessageDialog", "Alert", "Message", "Notification", "Spin", "Result", "Skeleton"},
    "window": {
        "Dialog", "StatusBar", "FileDialog", "ColorDialog", "FontDialog",
        "InputDialog", "ProgressDialog",
    },
}

SINGLE_NAME_OVERRIDES = {
    "progress": "ProgressBar",
    "radio": "RadioButton",
    "tabs": "TabWidget",
    "toolbox": "ToolBox",
}

GROUP_VARIANT_OVERRIDES = {
    "MdiArea": 4,
    "MdiSubWindow": 6,
    "StackedWidget": 3,
    "WizardPage": 6,
    "Wizard": 16,
    "Widget": 6,
    "Frame": 24,
    "Dialog": 12,
    "MessageDialog": 10,
}

# Project-owned runtime classes for Figma components that do not have a
# one-to-one native Qt widget. Their public class name follows the same stable
# ComponentLab/<Name> -> commonWidget::Z_<Name> rule as the design contract.
DESIGN_RUNTIME_BASES = {
    "ComponentLab/Navbar": "QWidget composition",
    "ComponentLab/Sidebar": "QWidget composition",
    "ComponentLab/DesktopBottomNavigation": "QWidget composition",
    "ComponentLab/MetricCard": "QWidget",
    "ComponentLab/Chart": "QWidget",
    "ComponentLab/ImagePanel": "QWidget",
    "ComponentLab/Empty": "QWidget",
    "ComponentLab/Statistic": "QWidget",
    "ComponentLab/Descriptions": "QGridLayout composition",
    "ComponentLab/Form": "QFormLayout composition",
    "ComponentLab/Switch": "QAbstractButton",
    "ComponentLab/Chip": "QAbstractButton",
    "ComponentLab/Card": "QWidget",
    "ComponentLab/IntegrationCard": "QWidget",
    "ComponentLab/Divider": "QWidget",
    "ComponentLab/Collapse": "QWidget",
    "ComponentLab/OverlayDrawer": "QWidget",
    "ComponentLab/DataTable": "QTableWidget",
    "ComponentLab/Alert": "QWidget",
    "ComponentLab/Message": "QWidget",
    "ComponentLab/Notification": "QWidget",
    "ComponentLab/Spin": "QWidget",
    "ComponentLab/Result": "QWidget",
    "ComponentLab/Skeleton": "QWidget",
}


def lower_first(value: str) -> str:
    return value[:1].lower() + value[1:]


def category_for(name: str) -> str:
    matches = [category for category, names in CATEGORY_COMPONENTS.items() if name in names]
    assert len(matches) == 1, (name, matches)
    return matches[0]


def native_design_for_key(key: str, native_by_design: dict[str, dict]) -> str | None:
    override = SINGLE_NAME_OVERRIDES.get(key)
    if override and f"ComponentLab/{override}" in native_by_design:
        return f"ComponentLab/{override}"
    for design_id in native_by_design:
        if design_id.rsplit("/", 1)[-1].lower() == key.lower():
            return design_id
    return None


def record_design_id(key: str, record: dict, native_by_design: dict[str, dict]) -> str:
    for field in ("designComponent", "publicComponentSetName", "publicDesignComponent"):
        value = record.get(field)
        if isinstance(value, str) and value.startswith("ComponentLab/"):
            return value
    native = native_design_for_key(key, native_by_design)
    if record.get("mappingType") == "native" and native:
        return native
    name = SINGLE_NAME_OVERRIDES.get(key, key[:1].upper() + key[1:])
    return f"ComponentLab/{name}"


def group_variant_count(record: dict, name: str) -> int:
    if name in GROUP_VARIANT_OVERRIDES:
        return GROUP_VARIANT_OVERRIDES[name]
    field = lower_first(name) + "VariantCount"
    if field in record:
        return int(record[field])
    for value in record.get("componentSets", {}).values():
        if isinstance(value, dict) and value.get("componentSetId"):
            # Match the component-set id in the classification rather than a display label.
            return -1
    raise AssertionError((name, "missing grouped variant count"))


def runtime_identity(design_id: str, mapping_kind: str, native_by_design: dict[str, dict]):
    name = design_id.rsplit("/", 1)[-1]
    if mapping_kind == "native":
        native = native_by_design[design_id]
        return native["qtClass"], f'commonWidget::{native["sdkClass"]}', True
    if design_id == "ComponentLab/Input":
        return "QWidget composition", "commonWidget::Z_InputField", True
    if design_id == "ComponentLab/Select":
        return "QWidget composition", "commonWidget::Z_Select", True
    if design_id in DESIGN_RUNTIME_BASES:
        return DESIGN_RUNTIME_BASES[design_id], f'commonWidget::Z_{name}', True
    return "", f"commonWidget::Z_{name}", False


def grouped_components(record: dict, native_by_design: dict[str, dict]) -> list[dict]:
    output = []
    component_sets = record.get("componentSets", {})
    for asset in record.get("assetClassifications", []):
        if asset["assetClass"] == "engineering-contract":
            continue
        design_id = asset["designComponentId"]
        name = design_id.rsplit("/", 1)[-1]
        count = group_variant_count(record, name)
        if count < 0:
            lookup = lower_first(name)
            entry = component_sets.get(lookup)
            assert entry, (name, component_sets.keys())
            count = int(entry["variantCount"])
        mapping_kind = asset["assetClass"].replace("-component", "")
        qt_class, sdk_class, instantiable = runtime_identity(
            design_id, mapping_kind, native_by_design
        )
        output.append({
            "designComponent": design_id,
            "sdkClass": sdk_class,
            "qtClass": qt_class or asset.get("qtCompositionBasis", ""),
            "mappingKind": mapping_kind,
            "category": category_for(name),
            "pageId": record["pageId"],
            "componentSetId": asset["componentSetId"],
            "variantCount": count,
            "instantiable": instantiable,
            "figmaStatus": "verified",
            "approvalStatus": "not-approved",
            "runtimeStatus": "implemented" if instantiable else "contract-only",
        })
    return output


def load_traceability_manifest() -> dict:
    manifest = json.loads(TRACEABILITY_MANIFEST.read_text(encoding="utf-8"))
    components = manifest["components"]
    counts = manifest["counts"]
    assert manifest["fileKey"] == FIGMA_FILE_KEY
    assert counts["total"] == len(components)
    assert counts["merge"] == sum(
        item["governanceDecision"] == "MERGE" for item in components
    )
    assert counts["built"] == sum(
        item["governanceDecision"] == "BUILT" for item in components
    )
    assert counts["engineeringVerified"] == sum(
        item["engineeringStatus"] == "VERIFIED" for item in components
    )
    assert counts["engineeringNameSyncRequired"] == sum(
        item["engineeringStatus"] == "NAME_SYNC_REQUIRED" for item in components
    )
    assert counts["engineeringGap"] == sum(
        item["engineeringStatus"] == "ENGINEERING_GAP" for item in components
    )
    assert counts["engineeringNotApplicable"] == sum(
        item["engineeringStatus"] == "NOT_APPLICABLE" for item in components
    )
    assert counts["total"] == sum(
        counts[key] for key in (
            "engineeringVerified",
            "engineeringNameSyncRequired",
            "engineeringGap",
            "engineeringNotApplicable",
        )
    )
    assert len({item["designComponentId"] for item in components}) == len(components)
    assert len({item["canonicalName"] for item in components}) == len(components)
    return manifest


def inventory_indexes(inventory: dict) -> tuple[dict[str, dict], dict[str, dict]]:
    pages_by_set = {}
    sets = {}
    for page in inventory["pages"]:
        for component_set in page["componentSets"]:
            assert component_set["id"] not in sets, component_set["id"]
            sets[component_set["id"]] = component_set
            pages_by_set[component_set["id"]] = page
    return pages_by_set, sets


def component_set_governance(
    inventory: dict,
    entries: list[dict],
    manifest: dict,
) -> dict:
    """Account for every Figma Component Set without inflating public identities."""
    _, inventory_sets = inventory_indexes(inventory)
    canonical_owners: dict[str, list[str]] = {}
    for entry in entries:
        for component_set_id in entry.get("componentSetId", "").split(","):
            if component_set_id:
                canonical_owners.setdefault(component_set_id, []).append(
                    entry["canonicalId"]
                )

    duplicate_canonical_owners = {
        component_set_id: owners
        for component_set_id, owners in canonical_owners.items()
        if len(owners) != 1
    }
    assert not duplicate_canonical_owners, duplicate_canonical_owners
    assert set(canonical_owners) <= set(inventory_sets)

    policy = manifest["componentSetPolicy"]
    assert policy["source"].endswith("figma-component-inventory.json")
    allowed_roles = set(policy["allowedRoles"])
    assert allowed_roles == {"INTERNAL", "PROPOSAL", "RESIDUE"}
    canonical_ids = {entry["canonicalId"] for entry in entries}
    public_identity_nodes = policy.get("publicIdentityNodes", {})
    assert set(public_identity_nodes) <= canonical_ids
    assert len(set(public_identity_nodes.values())) == len(public_identity_nodes)
    assert not (set(public_identity_nodes.values()) & set(inventory_sets))
    for entry in entries:
        public_node_id = public_identity_nodes.get(entry["canonicalId"])
        if public_node_id:
            assert entry["componentSetId"] == "", entry
            assert entry["figmaNodeIds"] == [public_node_id], entry
    records = policy["unreferencedSets"]
    classified_ids = [record["componentSetId"] for record in records]
    assert len(classified_ids) == len(set(classified_ids))
    assert not (set(classified_ids) & set(canonical_owners))
    assert set(inventory_sets) == set(canonical_owners) | set(classified_ids), {
        "unaccounted": sorted(
            set(inventory_sets) - set(canonical_owners) - set(classified_ids)
        ),
        "unknownClassifications": sorted(set(classified_ids) - set(inventory_sets)),
    }

    records_by_id = {record["componentSetId"]: record for record in records}
    for entry in entries:
        if not entry.get("publicDesignNodeId"):
            continue
        variant_source_ids = [
            value
            for value in entry.get("variantSourceComponentSetId", "").split(",")
            if value
        ]
        assert variant_source_ids, entry
        for component_set_id in variant_source_ids:
            record = records_by_id[component_set_id]
            assert record["role"] == "INTERNAL", (entry, record)
            assert record.get("ownerCanonicalId") == entry["canonicalId"], (entry, record)

    for record in records:
        component_set_id = record["componentSetId"]
        assert record["role"] in allowed_roles, record
        assert record["name"] == inventory_sets[component_set_id]["name"], record
        owner = record.get("ownerCanonicalId")
        if owner is not None:
            assert owner in canonical_ids, record
        if record["role"] == "PROPOSAL":
            assert owner is not None, record
        if record["role"] == "RESIDUE":
            duplicate_of = record.get("duplicateOfComponentSetId")
            assert duplicate_of in inventory_sets, record
            assert duplicate_of != component_set_id, record
            assert inventory_sets[duplicate_of]["name"] == record["name"], record

    role_counts = {
        role: sum(record["role"] == role for record in records)
        for role in sorted(allowed_roles)
    }
    return {
        "rule": policy["rule"],
        "auditDate": policy["auditDate"],
        "figmaComponentSetCount": len(inventory_sets),
        "canonicalReferencedComponentSetCount": len(canonical_owners),
        "classifiedUnreferencedComponentSetCount": len(records),
        "publicIdentityNodes": public_identity_nodes,
        "roleCounts": role_counts,
        "unreferencedSets": records,
    }


def current_kind(design_id: str, item: dict | None, kind_policy: dict) -> str:
    if design_id in kind_policy["layoutHelpers"]:
        return "LayoutHelper"
    if design_id in kind_policy["assets"]:
        return "Asset"
    if design_id in kind_policy["newWidgets"]:
        return "Widget"
    if design_id in kind_policy["newComposites"]:
        return "Composite"
    assert item is not None, (design_id, "missing kind policy")
    return "Composite" if item["mappingKind"] == "composite" else "Widget"


def governance_fields(
    item: dict,
    trace: dict | None,
    kind: str,
    inventory: dict,
    downstream_evidence: dict | None = None,
) -> dict:
    design_id = item["designComponent"]
    engineering_status = "REGISTERED" if item["instantiable"] else "UNMAPPED"
    design_origin = "REUSE"
    identity_migration = "NONE"
    token_status = "UNRESOLVED"
    evidence = [
        "Source/ComponentKit/component-definitions/figma-component-inventory.json",
        "Source/ComponentKit/runtime/component/ComponentFactory.cpp",
    ]
    if trace:
        design_origin = "BUILD" if trace["governanceDecision"] == "BUILT" else "REUSE"
        if trace["engineeringStatus"] == "NAME_SYNC_REQUIRED":
            engineering_status = "IMPLEMENTED"
            identity_migration = "RENAME"
        elif trace["engineeringStatus"] == "ENGINEERING_GAP":
            engineering_status = trace.get("consumerEngineeringStatus", "UNMAPPED")
        elif trace["engineeringStatus"] == "NOT_APPLICABLE":
            assert kind in {"LayoutHelper", "Asset"}, (design_id, kind)
            engineering_status = trace.get("consumerEngineeringStatus", "NOT_APPLICABLE")
        else:
            engineering_status = "REGISTERED"
            if design_id == "ComponentLab/Drawer":
                identity_migration = "RENAME"
            elif trace["sourceName"] != trace["canonicalName"]:
                identity_migration = "MERGE"
        token_status = trace["tokenStatus"]
        evidence.append(
            "docs/designRules/design-system/component-traceability-manifest.json"
        )
    if downstream_evidence:
        evidence.append(
            "docs/designRules/design-system/component-traceability-manifest.json"
        )
        token_status = downstream_evidence.get("tokenStatus", token_status)
    consumer_implemented = (
        (trace and trace.get("consumerEngineeringStatus") == "IMPLEMENTED")
        or (
            downstream_evidence
            and downstream_evidence.get("consumerEngineeringStatus") == "IMPLEMENTED"
        )
    )
    runtime_scope = (
        downstream_evidence.get("runtimeScope")
        if downstream_evidence and downstream_evidence.get("runtimeScope")
        else trace.get("runtimeScope")
        if trace and trace.get("runtimeScope")
        else "FACTORY_INSTANTIATION_ONLY"
        if engineering_status == "REGISTERED"
        else "LAYOUT_CONVERTER_UNIT"
        if kind == "LayoutHelper" and engineering_status == "IMPLEMENTED"
        else "NONE"
    )
    result = {
        "canonicalId": design_id,
        "name": design_id.rsplit("/", 1)[-1],
        "kind": kind,
        "figmaFileKey": FIGMA_FILE_KEY,
        "figmaNodeIds": (
            [item["publicDesignNodeId"]]
            if item.get("publicDesignNodeId")
            else [
                value for value in item.get("componentSetId", "").split(",") if value
            ]
        ),
        "figmaAuditDate": inventory["source"]["capturedAt"],
        "lifecycleStatus": "CURRENT",
        "owner": "UNRESOLVED",
        "version": "UNRESOLVED",
        "evidenceRefs": evidence,
        "designOriginAction": design_origin,
        "identityMigration": identity_migration,
        "engineeringStatus": engineering_status,
        "runtimeOwner": trace.get("runtimeOwner", "GenerateComponent") if trace else "GenerateComponent",
        "runtimeStatus": (
            "implemented" if engineering_status in {"REGISTERED", "IMPLEMENTED"}
            else "unmapped" if engineering_status == "UNMAPPED"
            else "not-applicable"
        ),
        "verificationStatus": {
            "schema": "VERIFIED",
            "build": "VERIFIED" if engineering_status in {"REGISTERED", "IMPLEMENTED"} else "NOT_VERIFIED",
            "runtime": "VERIFIED" if engineering_status in {"REGISTERED", "IMPLEMENTED"} else "NOT_VERIFIED",
            "runtimeScope": runtime_scope,
            "factoryRegistration": (
                "VERIFIED" if engineering_status == "REGISTERED"
                else "NOT_APPLICABLE" if kind in {"LayoutHelper", "Asset"}
                else "NOT_VERIFIED"
            ),
            "visual": "NOT_VERIFIED",
            "downstreamConsumption": (
                "VERIFIED" if consumer_implemented
                else "NOT_APPLICABLE" if kind in {"LayoutHelper", "Asset"}
                else "NOT_VERIFIED"
            ),
            "tokenBinding": token_status,
        },
        "governanceApprovalStatus": "NOT_REVIEWED",
        "focusPolicy": (
            "NOT_APPLICABLE" if kind in {"LayoutHelper", "Asset"}
            else downstream_evidence.get("focusPolicy")
            if downstream_evidence and downstream_evidence.get("focusPolicy")
            else trace.get("focusPolicy", "UNRESOLVED") if trace
            else "UNRESOLVED"
        ),
    }
    if trace and trace.get("runtimeContract"):
        result["runtimeContract"] = trace["runtimeContract"]
    elif downstream_evidence and downstream_evidence.get("runtimeContract"):
        result["runtimeContract"] = downstream_evidence["runtimeContract"]
    if trace:
        result["evidenceRefs"].extend(trace.get("runtimeEvidence", []))
        result["evidenceRefs"].extend(trace.get("consumerEvidence", []))
    if downstream_evidence:
        result["evidenceRefs"].extend(
            downstream_evidence.get("runtimeEvidence", [])
        )
        result["evidenceRefs"].extend(
            downstream_evidence.get("consumerEvidence", [])
        )
    if design_id == "ComponentLab/Drawer":
        result["compatibility"] = {
            "status": "LEGACY_COMPATIBILITY",
            "implementationClass": "commonWidget::Z_OverlayDrawer",
            "factoryKey": "commonWidget::Z_OverlayDrawer",
            "canonicalFactoryKey": "commonWidget::Z_Drawer",
        }
    return result


def merge_current_library(legacy: list[dict], inventory: dict) -> list[dict]:
    manifest = load_traceability_manifest()
    kind_policy = manifest["kindPolicy"]
    classified = set().union(
        kind_policy["newWidgets"],
        kind_policy["newComposites"],
        kind_policy["layoutHelpers"],
        kind_policy["assets"],
    )
    assert sum(len(kind_policy[key]) for key in (
        "newWidgets", "newComposites", "layoutHelpers", "assets"
    )) == len(classified)
    pages_by_set, sets = inventory_indexes(inventory)
    by_design = {item["designComponent"]: item for item in legacy}
    trace_by_design = {
        item["designComponentId"]: item for item in manifest["components"]
    }
    downstream_by_design = manifest.get("downstreamEvidenceOverrides", {})

    # Figma renamed the public identity in place. The factory publishes the
    # canonical Z_Drawer key while the original implementation type remains a
    # compatibility alias; neither creates a second catalog identity.
    drawer = by_design.pop("ComponentLab/OverlayDrawer")
    drawer["designComponent"] = "ComponentLab/Drawer"
    drawer["sdkClass"] = "commonWidget::Z_Drawer"
    drawer["componentSetId"] = "569:632"

    for design_id, trace in trace_by_design.items():
        if design_id in by_design:
            continue
        if design_id == "ComponentLab/Drawer":
            by_design[design_id] = drawer
            continue
        assert trace["engineeringStatus"] in {
            "ENGINEERING_GAP", "NOT_APPLICABLE", "VERIFIED"
        }, trace
        node_ids = trace["figmaNodeIds"]
        assert len(node_ids) == 1 and node_ids[0] in sets, trace
        component_set = sets[node_ids[0]]
        page = pages_by_set[node_ids[0]]
        kind = current_kind(design_id, None, kind_policy)
        by_design[design_id] = {
            "designComponent": design_id,
            "sdkClass": trace.get("developmentComponentId") or "",
            "qtClass": trace.get("runtimeContract", {}).get("qtBaseClass", ""),
            "mappingKind": {
                "Widget": "widget",
                "Composite": "composite",
                "LayoutHelper": "layout-helper",
                "Asset": "asset",
            }[kind],
            "category": CATEGORY_ID_TO_KEY[page["categoryId"]],
            "pageId": page["id"],
            "componentSetId": component_set["id"],
            "variantCount": len(component_set["variants"]),
            "instantiable": trace["engineeringStatus"] == "VERIFIED",
            "figmaStatus": "verified",
            "approvalStatus": "not-approved",
            "runtimeStatus": (
                "implemented" if trace["engineeringStatus"] == "VERIFIED"
                else "not-applicable" if kind in {"LayoutHelper", "Asset"} else "unmapped"
            ),
        }

    assert classified == {
        design_id for design_id, item in by_design.items()
        if design_id not in {legacy_item["designComponent"] for legacy_item in legacy}
        or design_id == "ComponentLab/Drawer"
    } - {"ComponentLab/Drawer"}, (classified, set(by_design))

    entries = list(by_design.values())
    assert set(downstream_by_design) <= set(by_design), {
        "unknownDownstreamEvidence": sorted(set(downstream_by_design) - set(by_design))
    }
    for item in entries:
        design_id = item["designComponent"]
        trace = trace_by_design.get(design_id)
        kind = current_kind(design_id, item, kind_policy)
        item.update(governance_fields(
            item,
            trace,
            kind,
            inventory,
            downstream_by_design.get(design_id),
        ))
    return entries


def build_catalog() -> list[dict]:
    ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
    traceability = load_traceability_manifest()
    public_identity_nodes = traceability["componentSetPolicy"].get(
        "publicIdentityNodes", {}
    )
    native_contract = json.loads(NATIVE_DEFINITIONS.read_text(encoding="utf-8"))
    native_by_design = {
        f'ComponentLab/{item["designComponent"]}': item
        for item in native_contract["components"]
    }
    result = []
    for key, record in ledger["entities"]["componentPages"].items():
        if record.get("recordKind") == "component-specimen":
            continue
        if record.get("assetClassifications"):
            result.extend(grouped_components(record, native_by_design))
            continue
        design_id = record_design_id(key, record, native_by_design)
        mapping_kind = record.get("mappingType")
        assert mapping_kind in {"native", "design-system", "composite"}, (key, mapping_kind)
        qt_class, sdk_class, instantiable = runtime_identity(
            design_id, mapping_kind, native_by_design
        )
        component_set_ids = record.get("componentSetIds", [])
        component_set_id = record.get("componentSetId", "")
        if not component_set_id and component_set_ids:
            component_set_id = ",".join(component_set_ids)
        status_text = record.get("status", "")
        item = {
            "designComponent": design_id,
            "sdkClass": sdk_class,
            "qtClass": qt_class,
            "mappingKind": mapping_kind,
            "category": category_for(design_id.rsplit("/", 1)[-1]),
            "pageId": record["pageId"],
            "componentSetId": component_set_id,
            "variantCount": int(record.get("variantCount", 0)),
            "instantiable": instantiable,
            "figmaStatus": "verified",
            "approvalStatus": "not-approved",
            "runtimeStatus": (
                "implemented" if instantiable else
                "implementation-blocked" if "BLOCKED" in status_text else "contract-only"
            ),
        }
        public_design_node_id = public_identity_nodes.get(design_id)
        if public_design_node_id:
            item["publicDesignNodeId"] = public_design_node_id
        result.append(item)

    # Public Qt identity and approval come from the governance ledger. Current
    # source set IDs/counts come from the live Figma inventory, never an older
    # prose/ledger count (ProgressBar and both SpinBoxes have grown since then).
    inventory = load_figma_inventory()
    pages = {page["id"]: page for page in inventory["pages"]}
    sets = {item["id"]: item for page in inventory["pages"] for item in page["componentSets"]}
    for item in result:
        if item.get("publicDesignNodeId"):
            assert item["publicDesignNodeId"] not in sets, item
            ids = [node_id for node_id in item["componentSetId"].split(",") if node_id]
            if not ids:
                owners = [other for other in result if other["pageId"] == item["pageId"]]
                assert len(owners) == 1, (item["designComponent"], "ambiguous variant source")
                ids = [source["id"] for source in pages[item["pageId"]]["componentSets"]]
            assert ids and all(node_id in sets for node_id in ids), item
            item["variantSourceComponentSetId"] = ",".join(ids)
            item["componentSetId"] = ""
            item["variantCount"] = sum(len(sets[node_id]["variants"]) for node_id in ids)
            continue
        ids = [node_id for node_id in item["componentSetId"].split(",") if node_id]
        if not ids:
            owners = [other for other in result if other["pageId"] == item["pageId"]]
            assert len(owners) == 1, (item["designComponent"], "ambiguous source ownership")
            # Menu/MenuBar/TabWidget/ToolBox expose item anatomy sets, not new
            # public widget classes. Select owns its shell/anatomy/popup sets.
            ids = [source["id"] for source in pages[item["pageId"]]["componentSets"]]
            item["componentSetId"] = ",".join(ids)
        assert ids and all(node_id in sets for node_id in ids), item
        item["variantCount"] = sum(len(sets[node_id]["variants"]) for node_id in ids)

    legacy_count = len(result)
    expected_design_ids = {
        "ComponentLab/Drawer" if item["designComponent"] == "ComponentLab/OverlayDrawer"
        else item["designComponent"]
        for item in result
    } | {
        item["designComponentId"]
        for item in load_traceability_manifest()["components"]
    }
    result = merge_current_library(result, inventory)
    order = {category: index for index, (category, _) in enumerate(CATEGORY_ORDER)}
    result.sort(key=lambda item: (order[item["category"]], item["pageId"], item["designComponent"]))
    trace = traceability
    assert {item["canonicalId"] for item in result} == expected_design_ids
    assert len({item["canonicalId"] for item in result}) == len(result)
    sdk_classes = [item["sdkClass"] for item in result if item["sdkClass"]]
    assert len(set(sdk_classes)) == len(sdk_classes)
    assert sum(item["mappingKind"] == "native" for item in result) == 59
    assert sum(item["instantiable"] for item in result) == sum(
        item["engineeringStatus"] == "REGISTERED" for item in result
    )
    assert {item["category"] for item in result} == {item[0] for item in CATEGORY_ORDER}
    return result


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def load_figma_inventory() -> dict:
    inventory = json.loads(FIGMA_INVENTORY.read_text(encoding="utf-8"))
    pages = inventory["pages"]
    component_sets = [item for page in pages for item in page["componentSets"]]
    variants = [variant for item in component_sets for variant in item["variants"]]
    assert len(pages) == inventory["source"]["pageCount"]
    assert len(component_sets) == inventory["source"]["componentSetCount"]
    assert len(variants) == inventory["source"]["variantCount"]
    assert [item["id"] for item in inventory["categories"]] == list(CATEGORY_ID_TO_KEY)
    assert len({page["id"] for page in pages}) == len(pages)
    assert len({item["id"] for item in component_sets}) == len(component_sets)
    assert all(page["categoryId"] in CATEGORY_ID_TO_KEY for page in pages)
    return inventory


def cpp_qstring(value: str) -> str:
    return f'QString::fromUtf8("{cpp_string(value)}")'


def cpp_qstring_list(values: list[str]) -> str:
    return "{" + ", ".join(cpp_qstring(value) for value in values) + "}"


def header_text() -> str:
    return '''#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace commonWidget {

// Public design inventory generated from the current Figma component ledger.
// instantiable=false means the design contract exists but no runtime class is claimed.
struct DesignComponentDescriptor {
    QString designComponent;
    QString sdkClass;
    QString qtClass;
    QString mappingKind;
    QString category;
    QString pageId;
    QString componentSetId;
    QString publicDesignNodeId;
    QString variantSourceComponentSetId;
    int variantCount;
    bool instantiable;
    QString figmaStatus;
    QString approvalStatus;
    QString runtimeStatus;
};

// Full canonical library inventory. Widget/Composite entries may also appear in
// DesignComponentDescriptor; LayoutHelper and Asset entries deliberately do not.
struct DesignLibraryEntry {
    QString canonicalId;
    QString name;
    QString kind;
    QString category;
    QString pageId;
    QString componentSetId;
    QString publicDesignNodeId;
    QString variantSourceComponentSetId;
    QString engineeringStatus;
    QString approvalStatus;
    bool instantiable;
};

struct DesignComponentCategory {
    QString id;
    QString label;
};

struct DesignVariantAxis {
    QString name;
    QStringList values;
};

struct DesignComponentSet {
    QString id;
    QString name;
    QList<DesignVariantAxis> variantAxes;
    QStringList variants;
};

struct DesignComponentPage {
    QString id;
    QString name;
    QString categoryId;
    QList<DesignComponentSet> componentSets;
};

QList<DesignComponentCategory> designComponentCategories();
QList<DesignComponentDescriptor> designComponentDescriptors();
QList<DesignLibraryEntry> designLibraryEntries();
QList<DesignComponentCategory> figmaComponentCategories();
QList<DesignComponentPage> figmaComponentPages();

} // namespace commonWidget
'''


def source_text(entries: list[dict], components: list[dict], inventory: dict) -> str:
    categories = "\n".join(
        f'        {{QStringLiteral("{cpp_string(key)}"), QString::fromUtf8("{cpp_string(label)}")}},'
        for key, label in CATEGORY_ORDER
    )
    rows = "\n".join(
        "        {"
        + ", ".join([
            f'QStringLiteral("{cpp_string(item["designComponent"])}")',
            f'QStringLiteral("{cpp_string(item["sdkClass"])}")',
            f'QStringLiteral("{cpp_string(item["qtClass"])}")',
            f'QStringLiteral("{cpp_string(item["mappingKind"])}")',
            f'QStringLiteral("{cpp_string(item["category"])}")',
            f'QStringLiteral("{cpp_string(item["pageId"])}")',
            f'QStringLiteral("{cpp_string(item["componentSetId"])}")',
            f'QStringLiteral("{cpp_string(item.get("publicDesignNodeId", ""))}")',
            f'QStringLiteral("{cpp_string(item.get("variantSourceComponentSetId", ""))}")',
            str(item["variantCount"]),
            "true" if item["instantiable"] else "false",
            f'QStringLiteral("{cpp_string(item["figmaStatus"])}")',
            f'QStringLiteral("{cpp_string(item["approvalStatus"])}")',
            f'QStringLiteral("{cpp_string(item["runtimeStatus"])}")',
        ])
        + "},"
        for item in components
    )
    library_rows = "\n".join(
        "        {"
        + ", ".join([
            f'QStringLiteral("{cpp_string(item["canonicalId"])}")',
            f'QStringLiteral("{cpp_string(item["name"])}")',
            f'QStringLiteral("{cpp_string(item["kind"])}")',
            f'QStringLiteral("{cpp_string(item["category"])}")',
            f'QStringLiteral("{cpp_string(item["pageId"])}")',
            f'QStringLiteral("{cpp_string(item["componentSetId"])}")',
            f'QStringLiteral("{cpp_string(item.get("publicDesignNodeId", ""))}")',
            f'QStringLiteral("{cpp_string(item.get("variantSourceComponentSetId", ""))}")',
            f'QStringLiteral("{cpp_string(item["engineeringStatus"])}")',
            f'QStringLiteral("{cpp_string(item["governanceApprovalStatus"])}")',
            "true" if item["instantiable"] else "false",
        ])
        + "},"
        for item in entries
    )
    figma_categories = "\n".join(
        f"        {{{cpp_qstring(item['id'])}, {cpp_qstring(item['name'])}}},"
        for item in inventory["categories"]
    )
    figma_pages = []
    for page in inventory["pages"]:
        component_sets = []
        for component_set in page["componentSets"]:
            axes = ", ".join(
                "{" + cpp_qstring(name) + ", " + cpp_qstring_list(values) + "}"
                for name, values in component_set["variantAxes"].items()
            )
            component_sets.append(
                "{" + ", ".join([
                    cpp_qstring(component_set["id"]),
                    cpp_qstring(component_set["name"]),
                    "{" + axes + "}",
                    cpp_qstring_list(component_set["variants"]),
                ]) + "}"
            )
        figma_pages.append(
            "        {" + ", ".join([
                cpp_qstring(page["id"]),
                cpp_qstring(page["name"]),
                cpp_qstring(page["categoryId"]),
                "{" + ", ".join(component_sets) + "}",
            ]) + "},"
        )
    figma_page_rows = "\n".join(figma_pages)
    return f'''#include "DesignComponentCatalog.h"

namespace commonWidget {{

QList<DesignComponentCategory> designComponentCategories() {{
    return {{
{categories}
    }};
}}

QList<DesignComponentDescriptor> designComponentDescriptors() {{
    return {{
{rows}
    }};
}}

QList<DesignLibraryEntry> designLibraryEntries() {{
    return {{
{library_rows}
    }};
}}

QList<DesignComponentCategory> figmaComponentCategories() {{
    return {{
{figma_categories}
    }};
}}

QList<DesignComponentPage> figmaComponentPages() {{
    return {{
{figma_page_rows}
    }};
}}

}} // namespace commonWidget
'''


def expected_outputs() -> dict[Path, str]:
    entries = build_catalog()
    components = [item for item in entries if item["kind"] in {"Widget", "Composite"}]
    inventory = load_figma_inventory()
    component_set_policy = component_set_governance(
        inventory, entries, load_traceability_manifest()
    )
    statistics = {
        "canonicalLibraryEntryCount": len(entries),
        "componentIdentityCount": len(components),
        "widgetCount": sum(item["kind"] == "Widget" for item in entries),
        "compositeCount": sum(item["kind"] == "Composite" for item in entries),
        "runtimeComponentCandidateCount": len(components),
        "registeredRuntimeComponentCount": sum(
            item["engineeringStatus"] == "REGISTERED" for item in entries
        ),
        "layoutHelperCount": sum(item["kind"] == "LayoutHelper" for item in entries),
        "implementedLayoutHelperCount": sum(
            item["kind"] == "LayoutHelper" and item["engineeringStatus"] == "IMPLEMENTED"
            for item in entries
        ),
        "assetEntryCount": sum(item["kind"] == "Asset" for item in entries),
        "unmappedAssetCount": sum(
            item["kind"] == "Asset" and item["engineeringStatus"] == "UNMAPPED"
            for item in entries
        ),
        "componentEngineeringGapCount": sum(
            item["kind"] in {"Widget", "Composite"}
            and item["engineeringStatus"] == "UNMAPPED"
            for item in entries
        ),
        "engineeringGapCount": sum(
            item["engineeringStatus"] == "UNMAPPED" for item in entries
        ),
        "legacyImplementationCount": sum(
            item.get("compatibility", {}).get("status") == "LEGACY_COMPATIBILITY"
            for item in entries
        ),
        "approvedComponentCount": sum(
            item["kind"] in {"Widget", "Composite"}
            and item["governanceApprovalStatus"] == "APPROVED"
            for item in entries
        ),
        "figmaComponentSetCount": component_set_policy["figmaComponentSetCount"],
        "canonicalReferencedComponentSetCount": component_set_policy[
            "canonicalReferencedComponentSetCount"
        ],
        "internalComponentSetCount": component_set_policy["roleCounts"]["INTERNAL"],
        "proposalComponentSetCount": component_set_policy["roleCounts"]["PROPOSAL"],
        "residueComponentSetCount": component_set_policy["roleCounts"]["RESIDUE"],
    }
    payload = {
        "schemaVersion": 2,
        "owner": "DesignToCode_GenerateComponent",
        "figmaFileKey": FIGMA_FILE_KEY,
        "scope": "Current Figma canonical library entries reconciled with real C++ runtime evidence",
        "namingRules": {
            "native": "Q<Class> -> commonWidget::Z_<Class>",
            "designSystemAndComposite": "ComponentLab/<Name> -> commonWidget::Z_<Name>",
            "compatibility": {
                "ComponentLab/Input": "commonWidget::Z_InputField",
                "commonWidget::Z_OverlayDrawer": "commonWidget::Z_Drawer",
            },
        },
        "statistics": statistics,
        "canonicalLibraryEntryCount": statistics["canonicalLibraryEntryCount"],
        "componentCount": len(components),
        "instantiableCount": sum(item["instantiable"] for item in components),
        "contractOnlyCount": sum(not item["instantiable"] for item in components),
        "categories": [{"id": key, "label": label} for key, label in CATEGORY_ORDER],
        "componentSetGovernance": component_set_policy,
        "entries": entries,
        "components": components,
        "figmaSource": inventory["source"],
        "figmaCategories": inventory["categories"],
        "figmaPages": inventory["pages"],
    }
    return {
        OUTPUT_JSON: json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        OUTPUT_HEADER: header_text(),
        OUTPUT_SOURCE: source_text(entries, components, inventory),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    drift = []
    outputs = expected_outputs()
    for path, content in outputs.items():
        if args.check:
            if not path.is_file() or path.read_text(encoding="utf-8") != content:
                drift.append(str(path.relative_to(ROOT)))
            continue
        if path.is_file() and path.read_text(encoding="utf-8") == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
    if drift:
        raise SystemExit("Generated Figma component catalog drift:\n" + "\n".join(drift))
    catalog = json.loads(outputs[OUTPUT_JSON])
    print(json.dumps({
        "canonicalEntries": catalog["canonicalLibraryEntryCount"],
        "components": catalog["componentCount"],
        "figmaPages": len(load_figma_inventory()["pages"]),
        "outputs": len(outputs),
        "mode": "check" if args.check else "write",
    }))


if __name__ == "__main__":
    main()
