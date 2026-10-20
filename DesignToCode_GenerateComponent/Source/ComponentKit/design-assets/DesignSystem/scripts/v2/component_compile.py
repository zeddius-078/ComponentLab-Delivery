"""Component-layer recipe compiler (《Component 层 Token 构建规范》).

Pipeline: source/component-recipes/*.json + source/component-registry.json
    -> 3-phase Registry-Recipe validation (local/ci/release, error/warning,
       structured diagnostics)
    -> generated/component.json (shared tokens + combination tokens computed
       with the registered priority chain)

Recipe shape:
{
  "$registry": {"component": "button", "version": "2.0"},
  "$status": "active",
  "base": {
     "size": {"small": {"slot/container/min-height": "{semantic.interaction-size.embedded}"}},
     ...
  },
  "overrides": [
     {"when": {"intent": "primary", "appearance": "filled", "state": "default",
               "content": "text-only", "size": "medium"},
      "slot": "label", "property": "color", "value": "{semantic/...}"}
  ]
}

Base paths use component/{family}/base/{axis}/{value}/slot/{slot}/{property}.
Combination paths use
component/{family}/{axis}/{value}/.../slot/{slot}/{property}
where the axis order follows the registry priority list.
"""
from __future__ import annotations

import argparse
import itertools
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]  # design-assets
SOURCE_DIR = ROOT / "DesignSystem/source"
RECIPE_DIR = SOURCE_DIR / "component-recipes"
CONTRACT_DIR = SOURCE_DIR / "component-contracts"
FIGMA_BINDING_DIR = SOURCE_DIR / "figma-bindings"
REGISTRY_PATH = SOURCE_DIR / "component-registry.json"
GENERATED_COMPONENT = ROOT / "DesignSystem/generated/component.json"
GENERATED_COMBOS = ROOT / "DesignSystem/generated/component-combinations.json"
DIAGNOSTICS = ROOT / "DesignSystem/generated/reports/component-diagnostics.json"
SOURCE_MANIFEST = SOURCE_DIR / "v2-source-manifest.json"

AXIS_PREFIX = {
    "intent": "intent", "appearance": "appearance", "state": "state",
    "content": "content", "size": "size", "mode": "mode",
    "selection": "selection", "searchable": "searchable", "open": "open",
    "value": "value",
    "surface": "surface", "density": "density", "kind": "kind",
}

ALIAS = re.compile(r"^\{([a-zA-Z0-9._/-]+)\}$")


class Diagnostic:
    def __init__(self, severity, phase, source_file, source_path, component,
                 message, suggestion="", **extra):
        self.entry = {
            "severity": severity, "phase": phase, "sourceFile": source_file,
            "sourcePath": source_path, "component": component,
            "registryRef": "component-registry.json@v2", "registryVersion": "2.0",
            "message": message, "suggestion": suggestion,
        }
        self.entry.update(extra)

    def text(self):
        axes = self.entry.get("axes", {})
        combo = " ".join(f"{k}={v}" for k, v in sorted(axes.items()))
        lines = [f"{self.entry['severity'].upper()} {self.entry['phase']}",
                 f"source: {self.entry['sourceFile']}#{self.entry['sourcePath']}",
                 f"component: {self.entry['component']}",
                 f"registry: {self.entry['registryRef']}"]
        if combo:
            lines.append(f"combination: {combo}")
        if self.entry.get("slot"):
            lines.append(f"slot: {self.entry['slot']}")
        if self.entry.get("property"):
            lines.append(f"property: {self.entry['property']}")
        lines.append(f"message: {self.entry['message']}")
        if self.entry["suggestion"]:
            lines.append(f"suggestion: {self.entry['suggestion']}")
        return "\n".join(lines)


def load_json(path: Path) -> Any:
    def unique(pairs):
        out = {}
        for k, v in pairs:
            if k in out:
                raise ValueError(f"Duplicate JSON key in {path.name}: {k}")
            out[k] = v
        return out
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique)


def split_slot(path_parts: list[str], registry_slots: set[str]) -> tuple[list[str], str, str] | None:
    """Find the slot segment; returns (axes segments, slot, property) or None."""
    for index in range(len(path_parts) - 1, 0, -1):
        if path_parts[index] in registry_slots and index + 1 < len(path_parts):
            return path_parts[:index], path_parts[index], path_parts[index + 1]
    return None


def combo_key(when: dict) -> str:
    return json.dumps(when, sort_keys=True)


class ComponentCompiler:
    def __init__(self, phase: str = "local", design_budget_report: Path | None = None):
        self.design_budget_report = design_budget_report
        self.phase = phase
        self.registry = load_json(REGISTRY_PATH)
        self.source_manifest = load_json(SOURCE_MANIFEST)
        self.typography = load_json(SOURCE_DIR / "typography.json").get("tokens", {})
        self.source_types = self.load_source_types()
        self.diagnostics: list[Diagnostic] = []
        self.materialization_metrics: dict[str, Any] = {
            "designSourceTokenCount": None,
            "designVariableCount": None,
            "observedVariableCounts": {"Figma": None, "Pixso": None},
            "runtimeExpandedCount": None,
            "designVariableBudget": None,
            "designBudgetStatus": "capacity-unverified",
        }

    def load_source_types(self) -> dict[str, tuple[str, str | None]]:
        result: dict[str, tuple[str, str | None]] = {}

        def visit(node: Any, prefix: str) -> None:
            if isinstance(node, dict) and "$type" in node:
                result[prefix] = (node["$type"], node.get("unit"))
                return
            if not isinstance(node, dict):
                return
            for key, child in node.items():
                if key.startswith("$"):
                    continue
                visit(child, f"{prefix}.{key}" if prefix else key)

        for layer in ("primitive", "profile", "semantic"):
            doc = load_json(SOURCE_DIR / f"{layer}.json")
            visit(doc["root"][layer], layer)
        return result

    def alias_leaf(self, value: str, source_path: str) -> dict:
        if not isinstance(value, str):
            raise ValueError(
                f"Recipe value at {source_path} must be a token reference string; "
                f"move behavioral metadata outside base (got {type(value).__name__})"
            )
        match = ALIAS.fullmatch(value)
        if not match:
            raise ValueError(f"Recipe value at {source_path} must be a token reference")
        target = match.group(1).replace("/", ".")
        if target not in self.source_types:
            raise ValueError(f"Unknown Recipe reference at {source_path}: {target}")
        type_, unit = self.source_types[target]
        leaf = {
            "$type": type_,
            "$value": "{" + target + "}",
            "$description": f"Recipe base value from {source_path}",
        }
        if unit:
            leaf["unit"] = unit
        return leaf

    def typography_values(self, value: str, source_path: str) -> dict[str, dict]:
        match = ALIAS.fullmatch(value)
        if not match:
            raise ValueError(f"Typography Recipe value at {source_path} must be a reference")
        reference = match.group(1).replace(".", "/")
        family, _, role = reference.removeprefix("typography/").partition("/")
        role_aliases = {("label", "default"): "label-1",
                        ("body", "default"): "body-1"}
        role = role_aliases.get((family, role), role)
        values = self.typography.get(family, {}).get(role)
        if not isinstance(values, dict):
            raise ValueError(f"Unknown typography composite at {source_path}: {reference}")
        property_names = {
            "fontFamily": "font-family", "fontSize": "font-size",
            "fontWeight": "font-weight", "lineHeight": "line-height",
            "letterSpacing": "letter-spacing",
        }
        return {
            property_names[key]: self.alias_leaf(alias, f"{source_path}/{key}")
            for key, alias in values.items() if key in property_names
        }

    def base_tokens(self, recipe: dict) -> dict[str, dict]:
        tokens: dict[str, dict] = {}

        def visit(node: Any, parts: list[str]) -> None:
            if isinstance(node, dict) and "$type" in node:
                tokens["base/" + "/".join(parts)] = node
                return
            if isinstance(node, dict):
                for key, child in node.items():
                    visit(child, parts + [part for part in key.split("/") if part])
                return
            source_path = "/base/" + "/".join(parts)
            if parts and parts[-1] == "typography":
                for prop, leaf in self.typography_values(node, source_path).items():
                    tokens["base/" + "/".join(parts[:-1] + [prop])] = leaf
                return
            tokens["base/" + "/".join(parts)] = self.alias_leaf(node, source_path)

        visit(recipe.get("base", {}), [])
        return tokens

    def named_slot_tokens(self, mapping: Any, source_path: str) -> dict[tuple[str, str], dict]:
        """Resolve sparse slot maps kept outside base/axis token paths."""
        if not isinstance(mapping, dict):
            raise ValueError(f"Token map at {source_path} must be an object")
        result: dict[tuple[str, str], dict] = {}
        for token_path, value in mapping.items():
            parts = [part for part in str(token_path).split("/") if part]
            if len(parts) < 3 or parts[0] != "slot":
                raise ValueError(
                    f"Token map at {source_path}/{token_path} must use slot/{{slot}}/{{property}}"
                )
            slot = parts[1]
            prop = "/".join(parts[2:])
            slot, prop = self.normalize(slot, prop)
            result[(slot, prop)] = self.alias_leaf(
                value, f"{source_path}/{token_path}"
            )
        return result

    def validate_named_slot_map(self, source: str, component: str, mapping: Any,
                                source_path: str, normalized_slots: dict[str, set]) -> None:
        try:
            tokens = self.named_slot_tokens(mapping, source_path)
        except ValueError as exc:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, source_path, component,
                str(exc), suggestion="使用可解析的 slot/{slot}/{property} Token 引用。"))
            return
        for (slot, prop), leaf in tokens.items():
            if slot not in normalized_slots:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, source_path, component,
                    f"Slot 未登记: {slot}。", slot=slot, property=prop))
            elif prop not in normalized_slots[slot]:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, source_path, component,
                    f"属性 {prop} 不属于 Slot {slot}。", slot=slot, property=prop))
            if not leaf.get("$value"):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, source_path, component,
                    "稀疏 Slot Token 缺少 $value。", slot=slot, property=prop))

    def normalized_slot_props(self, component_entry: dict) -> dict[str, set[str]]:
        """Registry slot property lists with the alias map applied, so recipes
        written with historical property names validate and normalize cleanly."""
        normalized: dict[str, set[str]] = {}
        for slot, props in component_entry.get("slots", {}).items():
            normalized[slot] = set()
            for prop in props:
                if prop == "typography":
                    normalized[slot].update({
                        "font-family", "font-size", "font-weight",
                        "line-height", "letter-spacing",
                    })
                    continue
                normalized[slot].add(self.normalize(slot, prop)[1])
        return normalized

    def normalize(self, slot: str, prop: str) -> tuple[str, str]:
        """Apply the registered property aliases (slot-scoped)."""
        aliases = self.registry.get("propertyAliases", {})
        if f"{slot}+{prop}" in aliases:
            new_slot, _, new_prop = aliases[f"{slot}+{prop}"].partition("+")
            return new_slot, new_prop
        if prop == "radius":
            return slot, "border-radius"
        if prop == "width" and slot in ("border", "focus-ring"):
            return slot, "border-width"
        return slot, prop

    # ------------------------------------------------------------------ validation

    def validate(self) -> None:
        self.validate_token_architecture()
        self.validate_content_catalog()
        self.validate_composite_packages()
        self.validate_contracts()
        self.validate_figma_bindings()
        recipes = self.load_recipes()
        if not recipes:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, str(RECIPE_DIR), "", "",
                "No component recipes found; generation cannot start."))
            return
        recipe_docs = {}
        for path in sorted(RECIPE_DIR.glob("*.json")):
            recipe = load_json(path)
            recipe_docs[path.stem] = recipe
            self.validate_recipe(path, recipe)
        self.validate_materialization_policy(recipe_docs)

    def validate_content_catalog(self) -> None:
        """Validate Content as terminology/localized copy, never as tokens."""
        path = SOURCE_DIR / "content/catalog.json"
        source = "source/content/catalog.json"
        if not path.exists():
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "", "",
                "缺少 Content Catalog 源文件。",
                suggestion="建立只保存术语和多语言实例文本的 Content Catalog。"))
            return
        catalog = load_json(path)
        if (catalog.get("layer") != "content" or
                catalog.get("kind") != "content-catalog" or
                catalog.get("$status") != "active"):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "", "",
                "Content Catalog 必须声明 layer=content、kind=content-catalog 且 status=active。"))
            return
        locales = catalog.get("supportedLocales")
        default_locale = catalog.get("defaultLocale")
        fallback_locale = catalog.get("fallbackLocale")
        entries = catalog.get("entries")
        if (not isinstance(locales, list) or not locales or
                default_locale not in locales or fallback_locale not in locales or
                default_locale != "en-US" or fallback_locale != "en-US"):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/supportedLocales", "",
                "Content Catalog 必须以英文 en-US 作为默认语言和回退语言，且二者必须登记在 supportedLocales。"))
        if not isinstance(entries, dict):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/entries", "",
                "Content Catalog entries 必须是对象。"))
            return
        for key, entry in entries.items():
            if not isinstance(entry, dict) or not isinstance(entry.get("values"), dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/entries/{key}", "",
                    "Content 条目必须只包含 values、fallbackKey 和 description。"))
                continue
            unknown = set(entry["values"]) - set(locales)
            if unknown:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/entries/{key}/values", "",
                    f"Content 条目包含未登记 Locale: {sorted(unknown)}。"))

    def validate_composite_packages(self) -> None:
        """Ensure every Composite registry entry has a typed source package."""
        source = "source/composite-registry.json"
        registry = load_json(SOURCE_DIR / "composite-registry.json")
        types = registry.get("types", {})
        if not isinstance(types, dict):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/types", "",
                "Composite Registry 的 types 必须是对象。"))
            return
        allowed = {"draft", "candidate", "active", "reserved"}
        for name, spec in types.items():
            if not isinstance(spec, dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/types/{name}", "",
                    "Composite 类型登记必须是对象。"))
                continue
            package_name = spec.get("package")
            package_path = SOURCE_DIR / str(package_name) if package_name else None
            if not package_name or package_path is None or not package_path.exists():
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/types/{name}/package", "",
                    f"Composite {name} 缺少可读取的源包。",
                    suggestion="每个 Composite 类型必须对应 source/*.json 源包。"))
                continue
            package = load_json(package_path)
            if package.get("layer") != "composite":
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, str(package_path), "/layer", "",
                    f"Composite 源包 {package_name} 必须声明 layer=composite。"))
            if package.get("kind") != name:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, str(package_path), "/kind", "",
                    f"Composite Registry 类型 {name} 与源包 kind 不一致。"))
            if package.get("$status") not in allowed:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, str(package_path), "/$status", "",
                    f"Composite {name} 的状态不在允许集合内。"))
            if not isinstance(package.get("tokens"), dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, str(package_path), "/tokens", "",
                    f"Composite {name} 必须提供 tokens 对象。"))

    def validate_token_architecture(self) -> None:
        """Validate the logical seven-layer architecture declaration.

        Physical migration remains staged: the loader still consumes the
        compatibility projection, while this manifest is the authoritative
        declaration of the target ownership and dependency order.
        """
        source = "source/v2-source-manifest.json"
        authority = self.source_manifest.get("authority", {})
        architecture = authority.get("tokenArchitecture")
        if not isinstance(architecture, dict):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/authority/tokenArchitecture", "",
                "缺少七层 Token 架构声明。",
                suggestion="声明 Primitive、Profile、Theme、Semantic、Composite、Component、Content 的依赖顺序。"))
            return
        expected = ["primitive", "profile", "theme", "semantic",
                    "composite", "component", "content"]
        if architecture.get("dependencyOrder") != expected:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source,
                "/authority/tokenArchitecture/dependencyOrder", "",
                "Token 层依赖顺序不符合 V2 七层架构。",
                suggestion="必须保持 Primitive → Profile → Theme → Semantic → Composite → Component → Content。"))
        layers = architecture.get("layers")
        if not isinstance(layers, list) or [item.get("key") for item in layers
                                             if isinstance(item, dict)] != expected:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/authority/tokenArchitecture/layers", "",
                "Token 层登记必须完整覆盖七个逻辑层，并按依赖顺序排列。",
                suggestion="补齐每层的 id、key、path、role 和 status。"))
        compatibility = architecture.get("compatibility")
        if not isinstance(compatibility, dict) or compatibility.get("generatedOutputsAreNotSourceLayers") is not True:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source,
                "/authority/tokenArchitecture/compatibility", "",
                "必须明确生成产物与设计源物理分离。",
                suggestion="将 generated outputs 标记为非 source layer，并通过兼容投影逐步迁移。"))
        bindings = self.source_manifest.get("componentLayerBindings")
        required_components = {
            "button", "input", "select", "table", "checkbox", "radio",
            "menu", "tool-button",
        }
        if not isinstance(bindings, dict) or bindings.get("resolvedComponentLayer") != "06.component":
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source,
                "/componentLayerBindings", "",
                "已确认组件缺少七层参与关系登记。",
                suggestion="登记每个组件的上游层、Theme/Semantic 路径、Composite、Content 和 Runtime 边界。"))
            return
        component_bindings = bindings.get("components", {})
        if not isinstance(component_bindings, dict):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source,
                "/componentLayerBindings/components", "",
                "componentLayerBindings.components 必须是对象。"))
            return
        expected_upstream = {"primitive", "profile", "theme", "semantic", "composite", "content"}
        for component in sorted(required_components):
            record = component_bindings.get(component)
            if not isinstance(record, dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/componentLayerBindings/components/{component}", component,
                    "已确认组件未登记七层适配记录。"))
                continue
            if record.get("status") != "adapted":
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/componentLayerBindings/components/{component}/status", component,
                    "组件七层适配状态必须为 adapted。"))
            if set(record.get("upstream", [])) != expected_upstream:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/componentLayerBindings/components/{component}/upstream", component,
                    "组件上游层必须覆盖 Primitive、Profile、Theme、Semantic、Composite、Content。"))
            for key in ("themePath", "profilePath", "compositePath", "contentPath", "runtimeBoundary"):
                if not record.get(key):
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source,
                        f"/componentLayerBindings/components/{component}/{key}", component,
                        f"组件七层适配记录缺少 {key}。"))

    def validate_materialization_policy(self, recipes: dict[str, dict]) -> None:
        """Validate the design/runtime materialization boundary and budget.

        The source-side count is a preflight count of sparse authored paths.
        It intentionally excludes generated Cartesian combinations.  An actual
        Figma/Pixso variable count is optional because it must come from a
        platform audit/sync adapter, not be guessed from runtime output.
        """
        # The landing manifest historically nested this policy under
        # ``authority`` while the V2 schema documents it at the root. Accept
        # both during the migration, but keep one source object and emit the
        # same diagnostics path for consumers.
        policy = self.source_manifest.get("materializationPolicy")
        if policy is None:
            policy = self.source_manifest.get("authority", {}).get("materializationPolicy")
        if policy is None:
            # Keep compatibility with the current landing manifest, where
            # source authority metadata is grouped under `authority`.
            policy = self.source_manifest.get("authority", {}).get(
                "materializationPolicy")
        source = "source/v2-source-manifest.json"
        if not isinstance(policy, dict):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/materializationPolicy", "",
                "缺少设计侧/运行时 Token 物化策略。",
                suggestion="声明 design=sparse 与 runtime=expanded，并登记设计侧预算。"))
            return
        design = policy.get("design")
        runtime = policy.get("runtime")
        if not isinstance(design, dict) or design.get("mode") != "sparse":
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/materializationPolicy/design", "",
                "设计侧物化策略必须是 sparse。",
                suggestion="禁止把运行时合法组合直接物化为 Figma/Pixso 变量。"))
        if not isinstance(runtime, dict) or runtime.get("mode") != "expanded":
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/materializationPolicy/runtime", "",
                "运行时物化策略必须是 expanded。",
                suggestion="由 Registry、Contract、Recipe 生成运行时合法组合。"))
        budget = design.get("figmaCollectionBudget", {}) if isinstance(design, dict) else {}
        platform_hard_max = budget.get("platformHardMaxVariablesPerCollection")
        if not isinstance(platform_hard_max, int) or platform_hard_max <= 0:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source,
                "/materializationPolicy/design/figmaCollectionBudget", "",
                "Figma 设计变量策略必须声明正整数平台硬上限；项目不设全局变量数量目标线。",
                suggestion="由 Registry/Contract/Recipe 控制稀疏建模，平台硬上限只用于阻断超容量同步。"))
            return

        sparse_count = 0
        for component, recipe in recipes.items():
            entry = self.registry.get("components", {}).get(component)
            if not entry:
                continue
            shared, _, overlays = self.materialize(component, entry, recipe)
            sparse_paths = set(shared) | set(overlays)
            for override in recipe.get("overrides", []):
                slot, prop = self.normalize(override["slot"], override["property"])
                sparse_paths.add(self.combo_path(
                    component, override.get("when", {}), slot, prop))
            sparse_count += len(sparse_paths)

        self.materialization_metrics.update({
            "designSourceTokenCount": sparse_count,
            "designVariableBudget": {
                "collection": "DSM/Component",
                "platformHardMax": platform_hard_max,
                "basis": "sparse-authored-source-paths; model-governed; no-global-project-target",
            },
        })
        if sparse_count > platform_hard_max:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source,
                "/materializationPolicy/design", "",
                f"设计侧稀疏 Token 候选数 {sparse_count} 超过 Figma Collection 平台硬上限 {platform_hard_max}。",
                suggestion="阻止 Figma/Pixso 同步，先减少设计侧物化变量；不得把生成产物机械拆分后导入。",
                metric="designSourceTokenCount", count=sparse_count,
                platformHardMax=platform_hard_max))
        self.validate_design_budget_report(sparse_count, platform_hard_max)

    def validate_design_budget_report(self, sparse_count: int,
                                      platform_hard_max: int) -> None:
        """Read the adapter-produced platform capacity report.

        This report is deliberately outside the source tree's authority.  A
        missing or unmeasured platform count does not block local/CI builds,
        but release output records that capacity is not yet verified.
        """
        source = "generated/reports/design-materialization-budget.json"
        if self.design_budget_report is None:
            self.materialization_metrics["designBudgetStatus"] = "capacity-unverified"
            if self.phase == "release":
                self.diagnostics.append(Diagnostic(
                    "warning", self.phase, source, "", "",
                    "缺少设计变量容量审计报告。",
                    suggestion="先由 Figma/Pixso 只读适配器生成设计侧变量计数后再发布。"))
            return
        try:
            report = load_json(self.design_budget_report)
        except (OSError, json.JSONDecodeError, ValueError) as exc:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "", "",
                f"设计变量容量审计报告无法读取: {exc}。",
                suggestion="重新生成合法的 design-materialization-budget.json。"))
            return

        report_count = report.get("designSourceTokenCount")
        if report_count != sparse_count:
            self.diagnostics.append(Diagnostic(
                "warning", self.phase, source, "/designSourceTokenCount", "",
                f"容量报告中的设计源计数 {report_count} 已落后于当前编译计数 {sparse_count}。",
                suggestion="重新运行设计侧预算适配器，更新只读审计报告。",
                expected=sparse_count, reported=report_count))

        collections = report.get("collections")
        if not isinstance(collections, list):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/collections", "",
                "设计变量容量报告缺少 collections 数组。",
                suggestion="按 Collection 和平台记录 observedVariableCount。"))
            return
        target = next((item for item in collections
                       if isinstance(item, dict) and item.get("name") == "DSM/Component"), None)
        if not isinstance(target, dict) or not isinstance(target.get("platforms"), dict):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/collections", "",
                "容量报告缺少 DSM/Component 的平台测量记录。",
                suggestion="同时登记 Figma 与 Pixso 的 observedVariableCount。"))
            return

        statuses = []
        for platform in ("Figma", "Pixso"):
            record = target["platforms"].get(platform, {})
            if not isinstance(record, dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/collections/DSM~1Component/platforms/{platform}", "",
                    f"{platform} 容量记录必须是对象。"))
                continue
            observed = record.get("observedVariableCount")
            self.materialization_metrics["observedVariableCounts"][platform] = observed
            if observed is None:
                statuses.append("capacity-unverified")
                if self.phase == "release":
                    self.diagnostics.append(Diagnostic(
                        "warning", self.phase, source,
                        f"/collections/DSM~1Component/platforms/{platform}", "",
                        f"{platform} 的实际变量数尚未测量。",
                        suggestion="发布前执行目标工作区的只读变量计数。"))
                continue
            if not isinstance(observed, int) or observed < 0:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/collections/DSM~1Component/platforms/{platform}/observedVariableCount", "",
                    f"{platform} observedVariableCount 必须是非负整数或 null。"))
                continue
            measured_hard_max = record.get("platformHardMax", platform_hard_max)
            if isinstance(measured_hard_max, int) and observed > measured_hard_max:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/collections/DSM~1Component/platforms/{platform}", "",
                    f"{platform} 设计变量数 {observed} 超过平台硬上限 {measured_hard_max}。",
                    suggestion="阻止同步，先减少设计侧物化变量。",
                    metric="observedVariableCount", count=observed,
                    platformHardMax=measured_hard_max))
                statuses.append("over-budget")
            else:
                statuses.append("verified")
        if "over-budget" in statuses:
            self.materialization_metrics["designBudgetStatus"] = "over-budget"
        elif statuses and all(status == "verified" for status in statuses):
            self.materialization_metrics["designBudgetStatus"] = "verified"
        else:
            self.materialization_metrics["designBudgetStatus"] = "capacity-unverified"

    def validate_contracts(self) -> None:
        """Validate non-visual component contracts separately from recipes.

        Contracts describe Table/Cell capabilities and runtime semantics. They
        are intentionally not expanded into component-layer Token paths.
        """
        if not CONTRACT_DIR.is_dir():
            return
        for path in sorted(CONTRACT_DIR.glob("*.json")):
            source = str(path.relative_to(ROOT)).replace("\\", "/")
            try:
                contract = load_json(path)
            except (OSError, json.JSONDecodeError, ValueError) as exc:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "", path.stem,
                    f"Component Contract 无法读取: {exc}",
                    suggestion="修复 JSON 结构后重新运行 V2 校验。"))
                continue
            component = contract.get("component", path.stem)
            entry = self.registry.get("components", {}).get(component)
            if not entry:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/component", component,
                    "Component Contract 引用了未登记的组件。",
                    suggestion="先在 component-registry.json 登记该组件。"))
                continue
            # Every public Component Contract has one common shape.  The
            # component-specific runtime contract may be shallow or deep, but
            # its source boundary is always separate from Registry and Recipe.
            required = ("contractKind", "registryBindings", "structure",
                        "stateLayers", "compositionRules", "tokenBoundary")
            missing = [key for key in required if key not in contract]
            if missing:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "", component,
                    f"Component Contract 缺少必需段落: {missing}。",
                    suggestion="补齐 Contract 类型、Registry 引用、结构、状态、组合规则和 Token 边界。"))
            contract_ref = entry.get("configuration", {}).get("contract")
            expected_ref = f"source/component-contracts/{path.name}"
            if not contract_ref:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/component", component,
                    "Registry 未登记该 Component Contract。",
                    suggestion=f"在 Registry 中登记 configuration.contract={expected_ref}。"))
            elif contract_ref != expected_ref:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/component", component,
                    f"Registry Contract 引用不一致: {contract_ref}。",
                    suggestion=f"统一为 {expected_ref}。"))

            bindings = contract.get("registryBindings")
            if not isinstance(bindings, dict):
                bindings = {}
            registry_axes = list(entry.get("axes", {}).keys())
            registry_slots = list(entry.get("slots", {}).keys())
            if bindings.get("source") != f"source/component-registry.json#/components/{component}":
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/registryBindings/source", component,
                    "Contract 的 Registry source 引用不一致。",
                    suggestion="引用当前组件的 component-registry.json#/components/{component}。"))
            for key, expected in (("axes", registry_axes), ("slots", registry_slots)):
                actual = bindings.get(key)
                if actual != expected:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, f"/registryBindings/{key}", component,
                        f"Contract 的 Registry {key} 引用与 Registry 不一致。",
                        suggestion=f"Contract 必须按 Registry 顺序引用: {expected}。"))

            for key in ("structure", "compositionRules"):
                if key in contract and not isinstance(contract[key], list):
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, f"/{key}", component,
                        f"{key} 必须是数组。"))
            if "stateLayers" in contract and not isinstance(contract["stateLayers"], dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/stateLayers", component,
                    "stateLayers 必须是对象。"))
            if "tokenBoundary" in contract and not isinstance(contract["tokenBoundary"], dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/tokenBoundary", component,
                    "tokenBoundary 必须是对象。"))

            # Table keeps its richer data contract checks in addition to the
            # common Contract shape.  Other components must not inherit Table's
            # column/editor vocabulary merely to satisfy the validator.
            if contract.get("contractKind") == "data-table":
                table_required = ("columnRoles", "contentDisplayTypes",
                                  "editorTypes")
                table_missing = [key for key in table_required if key not in contract]
                if table_missing:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, "", component,
                        f"Table Contract 缺少必需段落: {table_missing}。",
                        suggestion="补齐列职责、内容展示和编辑器类型定义。"))
                for key in table_required:
                    if key in contract and not isinstance(contract[key], list):
                        self.diagnostics.append(Diagnostic(
                            "error", self.phase, source, f"/{key}", component,
                            f"{key} 必须是数组。"))

    def validate_figma_bindings(self) -> None:
        """Validate draft-v2 Figma manifests without touching Figma.

        Older manifests may describe legacy evidence and can use a different
        shape.  The specification-only V2 manifests are required to mirror the
        Registry axes exactly; Figma remains a generated consumer, not a source.
        """
        if not FIGMA_BINDING_DIR.is_dir():
            return
        for path in sorted(FIGMA_BINDING_DIR.glob("*.json")):
            source = str(path.relative_to(ROOT)).replace("\\", "/")
            try:
                manifest = load_json(path)
            except (OSError, json.JSONDecodeError, ValueError) as exc:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "", path.stem,
                    f"Figma Binding Manifest 无法读取: {exc}",
                    suggestion="修复 JSON 结构后重新运行 V2 校验。"))
                continue
            if manifest.get("$status") != "draft-v2" or \
                    manifest.get("writePolicy") != "specification-only":
                continue
            component = manifest.get("component", path.stem)
            entry = self.registry.get("components", {}).get(component)
            if not entry:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/component", component,
                    "Figma Binding Manifest 引用了未登记的组件。",
                    suggestion="先在 component-registry.json 登记该组件。"))
                continue
            if component != path.stem:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/component", component,
                    "Figma Binding Manifest 文件名必须与 component 一致。",
                    suggestion=f"将文件命名为 {component}.json。"))
            expected_axes = entry.get("axes", {})
            actual_axes = manifest.get("variantAxes")
            if actual_axes != expected_axes:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/variantAxes", component,
                    "Figma Variant 轴和值必须与 Registry 完全一致。",
                    suggestion="不要在 Figma Manifest 中新增、删除或改名 Registry 轴值。"))
            namespace = manifest.get("variableNamespace", {})
            expected_namespace = {
                "collection": "DSM/Component",
                "prefix": f"{component}/",
                "canonicalPrefix": f"component.{component}.",
            }
            for key, expected in expected_namespace.items():
                if namespace.get(key) != expected:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source,
                        f"/variableNamespace/{key}", component,
                        f"Figma 变量命名空间 {key} 不符合 V2 约束。",
                        suggestion=f"使用 {expected}。"))
            if not isinstance(manifest.get("nestedStructure"), dict):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/nestedStructure", component,
                    "Figma Binding Manifest 必须声明嵌套结构。"))
            if not isinstance(manifest.get("bindingRules"), list) or \
                    not manifest.get("bindingRules"):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, "/bindingRules", component,
                    "Figma Binding Manifest 必须至少声明一条绑定规则。"))

    def load_recipes(self) -> list[Path]:
        if not RECIPE_DIR.is_dir():
            return []
        return list(RECIPE_DIR.glob("*.json"))

    def validate_recipe(self, path: Path, recipe: dict) -> None:
        ref = recipe.get("$registry", {})
        component = ref.get("component", "")
        source = str(path.relative_to(ROOT)).replace("\\", "/")
        component_entry = self.registry.get("components", {}).get(component)
        if not component_entry:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/$registry/component", component,
                "Registry 中不存在该组件。", suggestion="在 component-registry.json 登记该组件。"))
            return
        if str(ref.get("version", "")) != str(self.registry.get("$version", "")):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/$registry/version", component,
                "Registry 主版本不兼容。", suggestion="更新 Recipe 的 $registry.version。"))
        axes = component_entry.get("axes", {})
        slots = component_entry.get("slots", {})
        normalized_slots = self.normalized_slot_props(component_entry)
        slot_names = set(normalized_slots)
        axis_names = set(axes)
        axis_values = {name: set(values) for name, values in axes.items()}
        self.validate_combination_contract(source, component, component_entry)
        overrides = recipe.get("overrides", [])
        for index, override in enumerate(overrides):
            pointer = f"/overrides/{index}"
            when = override.get("when")
            if not isinstance(when, dict) or not when:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    "覆盖记录缺少 when 条件。", suggestion="提供结构化 when 条件。",
                    slot=override.get("slot"), property=override.get("property")))
                continue
            bad_axes = set(when) - axis_names
            if bad_axes:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    f"when 包含未登记轴: {sorted(bad_axes)}。",
                    suggestion="删除未登记轴或先在 Registry 登记。", axes=when))
            for axis, value in when.items():
                if axis in axis_values and value not in axis_values[axis]:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, pointer, component,
                        f"非法轴值: {axis}={value}。",
                        suggestion=f"合法值: {sorted(axis_values[axis])}。", axes=when))
            slot = override.get("slot")
            prop = override.get("property")
            if slot is not None and prop is not None:
                slot, prop = self.normalize(slot, prop)
            if slot not in slot_names:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    f"Slot 未登记: {slot}。", suggestion=f"合法 Slot: {sorted(slot_names)}。",
                    slot=slot, axes=when))
            prop = override.get("property")
            if slot is not None and prop is not None:
                slot, prop = self.normalize(slot, prop)
            if slot in normalized_slots and prop not in normalized_slots[slot]:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    f"属性 {prop} 不属于 Slot {slot}。",
                    suggestion=f"合法属性: {sorted(normalized_slots[slot])}。", slot=slot, property=prop, axes=when))
            value = override.get("value")
            # component-private direct values are registered in the Recipe and
            # allowed (spec: component-private material); aliases must be
            # resolvable {path} references.
            if isinstance(value, str) and not value.startswith("{"):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    "覆盖值必须是可解析引用或登记的结构化直接值。",
                    suggestion="value 使用 {path} 引用或结构化颜色/数值对象。",
                    slot=slot, property=prop, axes=when))
        base = self.base_tokens(recipe)
        for path_key, leaf in base.items():
            parts = path_key.split("/")
            found = split_slot(parts, slot_names)
            if not found:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/{path_key}", component,
                    f"Base Token 缺少已登记 Slot 定位: {path_key}。",
                    suggestion="使用 base/{axis}/.../slot/{slot}/{property} 路径。"))
                continue
            axis_segments, slot, prop = found
            slot, prop = self.normalize(slot, prop)
            if isinstance(leaf, dict):
                if slot in normalized_slots and prop not in normalized_slots[slot]:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, f"/{path_key}", component,
                        f"属性 {prop} 不属于 Slot {slot}。",
                        suggestion=f"合法属性: {sorted(normalized_slots[slot])}。",
                        slot=slot, property=prop))
                if not leaf.get("$value"):
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, f"/{path_key}", component,
                        "Base Token 缺少 $value。"))
        palette_groups = recipe.get("paletteGroups", {})
        if isinstance(palette_groups, dict):
            for group, mapping in palette_groups.items():
                self.validate_named_slot_map(
                    source, component, mapping,
                    f"/paletteGroups/{group}", normalized_slots)
        elif palette_groups:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/paletteGroups", component,
                "paletteGroups 必须是对象。"))
        value_structure = recipe.get("valueStructure", {})
        declared_groups = set(palette_groups) if isinstance(palette_groups, dict) else set()
        for value, rule in value_structure.items() if isinstance(value_structure, dict) else []:
            if value not in axis_values.get("value", set()):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/valueStructure/{value}", component,
                    f"valueStructure 引用了未登记的 Value: {value}。"))
            group = rule.get("palette-group") if isinstance(rule, dict) else None
            if group not in declared_groups:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, f"/valueStructure/{value}/palette-group", component,
                    f"Value 的调色板分组不存在: {group}。"))
        state_overrides = recipe.get("stateOverrides", {})
        if isinstance(state_overrides, dict):
            for state, rule in state_overrides.items():
                if state not in axis_values.get("state", set()):
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, f"/stateOverrides/{state}", component,
                        f"stateOverrides 引用了未登记的 State: {state}。"))
                    continue
                if not isinstance(rule, dict):
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, f"/stateOverrides/{state}", component,
                        "State 覆盖必须是对象。"))
                    continue
                for group, mapping in rule.get("paletteGroups", {}).items():
                    if group not in declared_groups:
                        self.diagnostics.append(Diagnostic(
                            "error", self.phase, source,
                            f"/stateOverrides/{state}/paletteGroups/{group}", component,
                            f"State 覆盖引用了不存在的调色板分组: {group}。"))
                    self.validate_named_slot_map(
                        source, component, mapping,
                        f"/stateOverrides/{state}/paletteGroups/{group}", normalized_slots)
                self.validate_named_slot_map(
                    source, component, rule.get("tokens", {}),
                    f"/stateOverrides/{state}/tokens", normalized_slots)
        elif state_overrides:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/stateOverrides", component,
                "stateOverrides 必须是对象。"))
        self.validate_overlays(source, component, recipe, normalized_slots,
                               component_entry.get("overlayDimensions", {}))

    def validate_overlays(self, source: str, component: str, recipe: dict,
                          normalized_slots: dict[str, set[str]], dimensions: dict) -> None:
        overlays = recipe.get("overlays", [])
        if not overlays:
            return
        if not isinstance(overlays, list):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/overlays", component,
                "overlays 必须是数组。"))
            return
        dimension_values = {name: set(values) for name, values in dimensions.items()}
        for index, overlay in enumerate(overlays):
            pointer = f"/overlays/{index}"
            when = overlay.get("when") if isinstance(overlay, dict) else None
            if not isinstance(when, dict) or not when:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    "Overlay 缺少 when 条件。",
                    suggestion="使用已登记的 overlay dimension/value。"))
                continue
            for dimension, value in when.items():
                if dimension not in dimension_values:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, pointer, component,
                        f"Overlay 使用了未登记的维度: {dimension}。"))
                elif value not in dimension_values[dimension]:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, pointer, component,
                        f"Overlay 使用了非法值: {dimension}={value}。"))
            slot = overlay.get("slot") if isinstance(overlay, dict) else None
            raw_prop = overlay.get("property") if isinstance(overlay, dict) else None
            prop = raw_prop
            if slot is not None and prop is not None and prop != "typography":
                slot, prop = self.normalize(slot, prop)
            if slot not in normalized_slots:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    f"Overlay Slot 未登记: {slot}。", slot=slot))
            elif raw_prop != "typography" and prop not in normalized_slots[slot]:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    f"Overlay 属性 {prop} 不属于 Slot {slot}。",
                    slot=slot, property=prop))
            value = overlay.get("value") if isinstance(overlay, dict) else None
            if not isinstance(value, str) or not ALIAS.fullmatch(value):
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source, pointer, component,
                    "Overlay value 必须是可解析 Token 引用。",
                    suggestion="使用 {semantic/...}、{primitive/...} 或完整 Composite 引用。",
                    slot=slot, property=prop))
            else:
                try:
                    if raw_prop == "typography":
                        self.typography_values(value, f"{source}{pointer}/value")
                    else:
                        self.alias_leaf(value, f"{source}{pointer}/value")
                except ValueError as exc:
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source, pointer, component,
                        str(exc), slot=slot, property=prop))

    @staticmethod
    def condition_matches(combo: dict, condition: dict) -> bool:
        for axis, expected in condition.items():
            if axis not in combo:
                return False
            allowed = expected if isinstance(expected, list) else [expected]
            if combo[axis] not in allowed:
                return False
        return True

    def is_legal_combo(self, entry: dict, combo: dict) -> bool:
        contract = entry.get("legalCombinations", {})
        if not isinstance(contract, dict):
            return True
        for constraint in contract.get("constraints", []):
            if constraint.get("type") == "forbid" and \
               self.condition_matches(combo, constraint.get("when", {})):
                return False
        return True

    def validate_combination_contract(self, source: str, component: str,
                                      entry: dict) -> None:
        contract = entry.get("legalCombinations", {})
        if not isinstance(contract, dict):
            return
        axes = entry.get("axes", {})
        matrix = entry.get("matrix", {})
        matrix_axes = matrix.get("axes", [])
        if matrix_axes != list(axes):
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/matrix/axes", component,
                "Matrix 轴顺序必须与组件 axes 完全一致。",
                suggestion="按 axes 声明顺序列出 matrix.axes。"))
            return
        constraint_error = False
        for index, constraint in enumerate(contract.get("constraints", [])):
            when = constraint.get("when", {})
            if constraint.get("type") != "forbid" or not isinstance(when, dict) or not when:
                constraint_error = True
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, source,
                    f"/legalCombinations/constraints/{index}", component,
                    "组合约束必须是带 when 条件的 forbid 记录。"))
                continue
            for axis, expected in when.items():
                allowed = expected if isinstance(expected, list) else [expected]
                if axis not in axes or any(value not in axes.get(axis, []) for value in allowed):
                    constraint_error = True
                    self.diagnostics.append(Diagnostic(
                        "error", self.phase, source,
                        f"/legalCombinations/constraints/{index}/when/{axis}", component,
                        f"组合约束引用了未登记的轴或轴值: {axis}={allowed}。"))
        if constraint_error:
            return
        names = list(axes)
        legal_count = sum(
            1 for values in itertools.product(*(axes[name] for name in names))
            if self.is_legal_combo(entry, dict(zip(names, values))))
        declared = matrix.get("logicalCombinationCount")
        if declared != legal_count:
            self.diagnostics.append(Diagnostic(
                "error", self.phase, source, "/matrix/logicalCombinationCount", component,
                f"声明的逻辑组合数 {declared} 与约束计算结果 {legal_count} 不一致。",
                suggestion="更新计数或修正 legalCombinations 约束。"))

    # ------------------------------------------------------------------ generation

    def resolve_combo(self, component: str, entry: dict, recipe: dict,
                      combo: dict) -> dict:
        """Priority chain: base/size -> intent -> appearance -> state -> content -> overrides."""
        priority = [axis for axis in self.registry["priority"]
                    if axis in entry.get("axes", {})]
        sources: dict[tuple[str, str], tuple[str, Any]] = {}
        base = self.base_tokens(recipe)
        for axis in priority:
            value = combo.get(axis, "")
            prefix = f"base/{axis}/{value}/"
            for path_key, leaf in base.items():
                if path_key.startswith(prefix):
                    parts = path_key.split("/")
                    found = split_slot(parts, set(entry.get("slots", {})))
                    if found:
                        slot, prop = self.normalize(found[1], found[2])
                        sources[(slot, prop)] = (path_key, leaf)
        value_rule = recipe.get("valueStructure", {}).get(combo.get("value"), {})
        palette_group = value_rule.get("palette-group") if isinstance(value_rule, dict) else None
        palette_map = recipe.get("paletteGroups", {}).get(palette_group, {})
        for (slot, prop), leaf in self.named_slot_tokens(
                palette_map, f"/paletteGroups/{palette_group}").items():
            sources[(slot, prop)] = (
                f"paletteGroups[{palette_group}]/{slot}/{prop}", leaf)
        state_rule = recipe.get("stateOverrides", {}).get(combo.get("state"), {})
        if isinstance(state_rule, dict):
            state_palette = state_rule.get("paletteGroups", {}).get(palette_group, {})
            for (slot, prop), leaf in self.named_slot_tokens(
                    state_palette,
                    f"/stateOverrides/{combo.get('state')}/paletteGroups/{palette_group}").items():
                sources[(slot, prop)] = (
                    f"stateOverrides[{combo.get('state')}]/paletteGroups[{palette_group}]/{slot}/{prop}",
                    leaf)
            for (slot, prop), leaf in self.named_slot_tokens(
                    state_rule.get("tokens", {}),
                    f"/stateOverrides/{combo.get('state')}/tokens").items():
                sources[(slot, prop)] = (
                    f"stateOverrides[{combo.get('state')}]/tokens/{slot}/{prop}", leaf)
        for override in recipe.get("overrides", []):
            when = override.get("when", {})
            if all(combo.get(axis) == value for axis, value in when.items()):
                slot, prop = self.normalize(override["slot"], override["property"])
                sources[(slot, prop)] = (
                    f"overrides[{combo_key(when)}]",
                    {"$value": override["value"],
                     "$type": override.get("$type", "string"),
                     "unit": override.get("unit")},
                )
        return sources

    def combo_path(self, component: str, combo: dict, slot: str, prop: str) -> str:
        parts = ["component", component]
        for axis in self.registry["priority"]:
            if axis in combo:
                parts += [AXIS_PREFIX.get(axis, axis), combo[axis]]
        for axis in AXIS_PREFIX:
            if axis in combo and axis not in self.registry["priority"]:
                parts += [AXIS_PREFIX.get(axis, axis), combo[axis]]
        parts += ["slot", slot, prop]
        return ".".join(parts)

    def overlay_path(self, component: str, entry: dict, when: dict,
                     slot: str, prop: str) -> str:
        parts = ["component", component, "overlay"]
        ordered = list(entry.get("overlayDimensions", {}))
        for dimension in ordered:
            if dimension in when:
                parts += [dimension, str(when[dimension])]
        for dimension in sorted(set(when) - set(ordered)):
            parts += [dimension, str(when[dimension])]
        parts += ["slot", slot, prop]
        return ".".join(parts)

    def overlay_tokens(self, component: str, entry: dict,
                       recipe: dict) -> dict[str, dict]:
        tokens: dict[str, dict] = {}
        for index, overlay in enumerate(recipe.get("overlays", [])):
            slot = overlay["slot"]
            prop = overlay["property"]
            if prop == "typography":
                leaves = self.typography_values(
                    overlay["value"], f"/overlays/{index}/value")
                for typography_prop, leaf in leaves.items():
                    tokens[self.overlay_path(
                        component, entry, overlay["when"], slot,
                        typography_prop)] = leaf
                continue
            slot, prop = self.normalize(slot, prop)
            leaf = self.alias_leaf(
                overlay["value"], f"/overlays/{index}/value")
            tokens[self.overlay_path(component, entry, overlay["when"], slot, prop)] = leaf
        return tokens

    def materialize(self, component: str, entry: dict, recipe: dict) -> tuple[dict, dict, dict]:
        axes = entry.get("axes", {})
        appearance_status = entry.get("appearanceStatus", {})
        shared_tokens: dict[str, Any] = {}
        combination_tokens: dict[str, Any] = {}
        base = self.base_tokens(recipe)
        for path_key, leaf in base.items():
            parts = path_key.split("/")
            found = split_slot(parts, set(entry.get("slots", {})))
            if found:
                axis_segments, slot, prop = found
                slot, prop = self.normalize(slot, prop)
                path_key = "/".join(axis_segments + [slot, prop])
            shared_tokens[f"component.{component}.{path_key.replace('/', '.')}"] = leaf
        covered: dict[str, set[str]] = {}
        for path_key in base:
            for axis in axes:
                marker = f"base/{axis}/"
                if path_key.startswith(marker):
                    rest = path_key[len(marker):]
                    value = rest.split("/", 1)[0]
                    covered.setdefault(axis, set()).add(value)
        for override in recipe.get("overrides", []):
            for axis, value in override.get("when", {}).items():
                covered.setdefault(axis, set()).add(value)
        if not covered:
            return shared_tokens, combination_tokens
        value_structure = recipe.get("valueStructure", {})
        state_overrides = recipe.get("stateOverrides", {})
        if isinstance(value_structure, dict) and value_structure:
            covered.setdefault("value", set()).update(value_structure)
        if isinstance(state_overrides, dict) and state_overrides:
            covered.setdefault("state", set()).update(state_overrides)
        names, values = [], []
        forced_axes = set(entry.get("matrix", {}).get("materializeAxes", []))
        for axis in axes:
            if axis == "size":
                # spec: Size/Padding/shared Spacing come from shared paths and
                # are not expanded per combination
                continue
            axis_values = list(axes[axis]) if axis in forced_axes \
                else sorted(covered.get(axis, set()))
            if not axis_values:
                continue
            materialized = [v for v in axis_values
                            if appearance_status.get(v, "complete") != "structural-placeholder"]
            if materialized:
                axis_values = materialized
            if not axis_values:
                continue
            names.append(axis)
            values.append(axis_values)
        for combo_values in itertools.product(*values):
            combo = dict(zip(names, combo_values))
            if not self.is_legal_combo(entry, combo):
                continue
            sources = self.resolve_combo(component, entry, recipe, combo)
            for (slot, prop), (source_path, leaf) in sorted(sources.items()):
                target = self.combo_path(component, combo, slot, prop)
                value = leaf.get("$value")
                if isinstance(value, str) and ALIAS.fullmatch(value):
                    combination_tokens[target] = {
                        "$type": leaf.get("$type", "string"),
                        "$value": value,
                        "$description": f"由 {source_path} 解析"}
                    if leaf.get("unit"):
                        combination_tokens[target]["unit"] = leaf["unit"]
                else:
                    combination_tokens[target] = leaf
        return shared_tokens, combination_tokens, self.overlay_tokens(component, entry, recipe)

    def generate(self) -> dict:
        recipes = {path.stem: load_json(path)
                   for path in sorted(self.load_recipes())}
        layer: dict[str, Any] = {}
        combinations_report: dict[str, Any] = {}

        def nest(path: str, leaf: dict) -> None:
            node = layer
            parts = path.split(".")[1:]  # strip the component root segment
            for part in parts[:-1]:
                node = node.setdefault(part, {})
            if parts[-1] in node:
                self.diagnostics.append(Diagnostic(
                    "error", self.phase, "component-recipes", "", component,
                    f"重复 Token 路径: {path}"))
            else:
                node[parts[-1]] = leaf

        for component, entry in self.registry.get("components", {}).items():
            recipe = recipes.get(component)
            if not recipe:
                continue
            shared, combos, overlays = self.materialize(component, entry, recipe)
            for path, leaf in shared.items():
                nest(path, leaf)
            for path, leaf in combos.items():
                nest(path, leaf)
            for path, leaf in overlays.items():
                nest(path, leaf)
            if entry.get("legalCombinations") == "cartesian" and \
               entry.get("matrixNote"):
                combinations_report[component] = {
                    "legalCombinationNote": entry["matrixNote"],
                    "axes": entry.get("axes", {}),
                }
        return layer


def main(argv: list[str] | None = None) -> int:
    from .output_guard import require_build_workspace
    require_build_workspace(ROOT)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=["local", "ci", "release"], default="local")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--design-budget-report", type=Path,
                        help="Optional current platform measurement; never inferred from history.")
    args = parser.parse_args(argv)

    compiler = ComponentCompiler(args.phase, args.design_budget_report)
    compiler.validate()
    has_errors = any(d.entry["severity"] == "error" for d in compiler.diagnostics)
    for diagnostic in compiler.diagnostics:
        print(diagnostic.text())
        print()
    if has_errors:
        errors = sum(1 for d in compiler.diagnostics if d.entry["severity"] == "error")
        warnings = sum(1 for d in compiler.diagnostics if d.entry["severity"] == "warning")
        print(f"Component Registry-Recipe 校验失败（{errors} errors / {warnings} warnings）")
        DIAGNOSTICS.parent.mkdir(parents=True, exist_ok=True)
        DIAGNOSTICS.write_text(
            json.dumps(diagnostics_payload(compiler),
                       ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return 1

    layer = compiler.generate()
    compiler.materialization_metrics["runtimeExpandedCount"] = _count(layer)
    payload = {
        "$schema": "../schema/v2-token-schema.json",
        "$description": "Generated component-layer product of the V2 recipe compiler. "
                        "Do not hand-edit; edit component-recipes/ and component-registry.json.",
        "layer": "component",
        "root": {"component": layer},
    }
    combos_payload = {
        "$description": "Legal combination index computed from the V2 registry.",
        "registry": {"components": compiler.registry.get("components", {})},
    }
    data = (json.dumps(payload, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    combos_data = (json.dumps(combos_payload, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    DIAGNOSTICS.parent.mkdir(parents=True, exist_ok=True)
    if args.check:
        drift = []
        if not GENERATED_COMPONENT.exists() or GENERATED_COMPONENT.read_bytes() != data:
            drift.append(str(GENERATED_COMPONENT))
        if not GENERATED_COMBOS.exists() or GENERATED_COMBOS.read_bytes() != combos_data:
            drift.append(str(GENERATED_COMBOS))
        DIAGNOSTICS.write_text(
            json.dumps(diagnostics_payload(compiler),
                       ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        if drift:
            print("Component generated drift: " + ", ".join(drift))
            return 1
        print(f"Component V2 check OK | phase={args.phase} | "
              f"tokens={_count(layer)} | diagnostics={len(compiler.diagnostics)}")
        return 0
    GENERATED_COMPONENT.parent.mkdir(parents=True, exist_ok=True)
    GENERATED_COMPONENT.write_bytes(data)
    GENERATED_COMBOS.write_bytes(combos_data)
    DIAGNOSTICS.write_text(
        json.dumps(diagnostics_payload(compiler),
                   ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Component V2 generated | phase={args.phase} | "
          f"design-source={compiler.materialization_metrics['designSourceTokenCount']} | "
          f"runtime-expanded={compiler.materialization_metrics['runtimeExpandedCount']} | "
          f"recipes={recipes_count()} | diagnostics={len(compiler.diagnostics)}")
    return 0


def recipes_count():
    return len(list(RECIPE_DIR.glob("*.json"))) if RECIPE_DIR.is_dir() else 0


def _count(layer: dict) -> int:
    count = 0
    for key, value in layer.items():
        if isinstance(value, dict) and "$type" in value:
            count += 1
        elif isinstance(value, dict):
            count += _count(value)
    return count


def diagnostics_payload(compiler: ComponentCompiler) -> dict[str, Any]:
    return {
        "phase": compiler.phase,
        "metrics": compiler.materialization_metrics,
        "diagnostics": [d.entry for d in compiler.diagnostics],
    }


if __name__ == "__main__":
    sys.exit(main())
