"""Validate and compile the V2 Typography Composite.

This stage resolves Typography through the canonical TokenModel, materializes
Standard/Wide values for Vue and Qt, and emits a read-only Figma mapping
manifest. It never writes to Figma.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

from .model import TokenModel
from .resolver import Resolver
from .values import css_value, qt_value

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "DesignSystem/source"
OUTPUT = ROOT / "DesignSystem/generated/composites"
REPORT = ROOT / "DesignSystem/generated/reports/typography-diagnostics.json"
REGISTRY = SOURCE / "composite-registry.json"
PACKAGE = SOURCE / "typography.json"
ALIASES = SOURCE / "migrations/token-aliases.json"
REF = re.compile(r"^\{([a-z0-9]+(?:[.-][a-z0-9]+)*)\}$")
PROFILE_MODES = ("standard", "wide")
REQUIRED_MEMBERS = ("fontFamily", "fontSize", "fontWeight", "lineHeight", "letterSpacing")
EXPECTED_TYPES = {
    "fontFamily": "fontFamily", "fontSize": "fontSize", "fontWeight": "fontWeight",
    "lineHeight": "lineHeight", "letterSpacing": "letterSpacing",
}


def load(path: Path) -> Any:
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Duplicate JSON key in {path.name}: {key}")
            result[key] = value
        return result
    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique)


def reference(value: Any) -> str | None:
    if not isinstance(value, str):
        return None
    match = REF.fullmatch(value)
    return match.group(1) if match else None


class TypographyCompiler:
    def __init__(self, phase: str = "local"):
        self.phase = phase
        self.package = load(PACKAGE)
        self.registry = load(REGISTRY)["types"]["typography"]
        self.aliases = load(ALIASES)
        self.model = TokenModel(SOURCE)
        self.resolvers = {
            mode: Resolver(self.model, self.model.context(mode, "light"))
            for mode in PROFILE_MODES
        }
        self.diagnostics: list[dict[str, str]] = []
        self.entries: dict[str, tuple[str, str, dict[str, Any]]] = {}
        self._collect_entries()

    def add(self, severity: str, code: str, path: str, message: str) -> None:
        self.diagnostics.append({
            "severity": severity,
            "code": code,
            "sourceFile": "DesignSystem/source/typography.json",
            "sourcePath": path,
            "message": message,
        })

    def _collect_entries(self) -> None:
        tokens = self.package.get("tokens", {})
        if not isinstance(tokens, dict):
            self.add("error", "TYPOGRAPHY-TOKENS-INVALID", "/tokens",
                     "Typography tokens 必须是对象。")
            return
        for role, presets in tokens.items():
            if role.startswith("$"):
                continue
            if not isinstance(presets, dict):
                self.add("error", "TYPOGRAPHY-ROLE-INVALID", f"/tokens/{role}",
                         "Typography role 必须包含 preset 对象。")
                continue
            for preset, entry in presets.items():
                path = f"typography.{role}.{preset}"
                if not isinstance(entry, dict):
                    self.add("error", "TYPOGRAPHY-PRESET-INVALID", f"/tokens/{role}/{preset}",
                             "Typography preset 必须是对象。")
                    continue
                self.entries[path] = (role, preset, entry)

    def _resolve_member(self, alias: Any, member: str, source_path: str, mode: str):
        target = reference(alias)
        if not target:
            self.add("error", "TYPOGRAPHY-REFERENCE-INVALID", source_path,
                     f"{member} 必须引用 Primitive/Profile/Semantic Token。")
            return None
        try:
            resolved = self.resolvers[mode].resolve(target)
        except (KeyError, TypeError, ValueError) as exc:
            self.add("error", "TYPOGRAPHY-REFERENCE-MISSING", source_path,
                     f"无法解析 Typography 引用 {alias!r}：{exc}")
            return None
        expected = EXPECTED_TYPES[member]
        if resolved.type != expected:
            self.add("error", "TYPOGRAPHY-MEMBER-TYPE", source_path,
                     f"{member} 需要 {expected}，实际为 {resolved.type}。")
        return resolved

    def _target_entry(self, target: str) -> tuple[str, str, dict[str, Any]] | None:
        parts = target.split("/")
        if len(parts) != 4 or parts[:2] != ["DSM", "Typography"]:
            return None
        return self.entries.get(f"typography.{parts[2]}.{parts[3]}")

    def validate(self) -> dict[str, Any]:
        if self.package.get("kind") != "typography":
            self.add("error", "TYPOGRAPHY-KIND", "/kind", "Typography package kind 必须为 typography。")
        if self.registry.get("package") != "typography.json":
            self.add("error", "TYPOGRAPHY-REGISTRY-PACKAGE", "/types/typography/package",
                     "Typography Registry 与 package 不一致。")
        if tuple(self.registry.get("requiredMembers", [])) != REQUIRED_MEMBERS:
            self.add("error", "TYPOGRAPHY-REQUIRED-MEMBERS", "/types/typography/requiredMembers",
                     "Typography Registry requiredMembers 与编译器契约不一致。")
        mapping = self.registry.get("figmaMapping", {})
        expected_variable_patterns = {
            "fontFamily": "font-family/harmony-os-sans",
            "fontSize": "font-size/{role}/{preset}",
            "fontWeight": "font-weight/{role}/{preset}",
            "lineHeight": "line-height/{role}/{preset}",
            "letterSpacing": "letter-spacing/{role}/{preset}",
        }
        if mapping.get("variableNamePatterns") != expected_variable_patterns:
            self.add(
                "error",
                "TYPOGRAPHY-FIGMA-VARIABLE-NAMING",
                "/types/typography/figmaMapping/variableNamePatterns",
                "Figma Typography 原子变量命名模板与 V2 契约不一致。",
            )
        expected_bindings = {
            "fontFamily": {
                "type": "string", "figmaResolvedType": "STRING",
                "scope": "FONT_FAMILY", "bindingField": "fontFamily",
                "valuePolicy": "single-font-name",
                "source": "semantic.typography.font-family.harmony-os-sans",
            },
            "fontWeight": {
                "type": "number", "figmaResolvedType": "FLOAT",
                "scope": "TEXT_PROPERTY", "bindingField": "fontWeight",
                "valuePolicy": "semantic-font-weight",
            },
            "fontSize": {
                "type": "number", "figmaResolvedType": "FLOAT",
                "scope": "TEXT_PROPERTY", "bindingField": "fontSize",
                "valuePolicy": "semantic-or-profile-reference",
            },
            "lineHeight": {
                "type": "number", "figmaResolvedType": "FLOAT",
                "scope": "TEXT_PROPERTY", "bindingField": "lineHeight",
                "valuePolicy": "semantic-or-profile-reference",
            },
            "letterSpacing": {
                "type": "number", "figmaResolvedType": "FLOAT",
                "scope": "TEXT_PROPERTY", "bindingField": "letterSpacing",
                "valuePolicy": "semantic-or-profile-reference",
            },
        }
        if mapping.get("variableBindings") != expected_bindings:
            self.add(
                "error",
                "TYPOGRAPHY-FIGMA-BINDING-CONTRACT",
                "/types/typography/figmaMapping/variableBindings",
                "Figma Typography 变量类型、Scope 或 Text Style 绑定字段与 V2 契约不一致。",
            )
        expected_sync_policy = {
            "preflightFonts": ["HarmonyOS Sans", "HarmonyOS Sans SC"],
            "writeOrder": [
                "preflight-fonts",
                "create-variable-collection",
                "create-modes",
                "create-variables",
                "create-and-bind-text-styles",
                "validate-structure-and-provenance",
            ],
            "targetCollection": "DSM/Typography",
            "targetModes": ["standard", "wide"],
            "legacyAction": "leave-untouched",
            "requiredValidation": [
                "collection-name",
                "mode-names-and-default",
                "variable-name-and-type",
                "variable-scope-and-binding-field",
                "text-style-count-and-name",
                "text-style-variable-bindings",
                "sourceRef-sourceVersion-compositeType",
            ],
            "partialWritePolicy": "abort-before-write-on-preflight-or-compile-error",
            "rerunPolicy": {
                "missingTarget": "create",
                "matchingTarget": "idempotent-update-managed-resources",
                "mismatchedTarget": "abort-and-report",
                "managedScope": "DSM/Typography-created-resources-only",
                "unmanagedResources": "leave-untouched",
                "legacyResources": "leave-untouched",
            },
        }
        if mapping.get("figmaSyncPolicy") != expected_sync_policy:
            self.add(
                "error",
                "TYPOGRAPHY-FIGMA-SYNC-POLICY",
                "/types/typography/figmaMapping/figmaSyncPolicy",
                "Figma Typography 同步写入边界或校验顺序与 V2 契约不一致。",
            )
        expected_provenance_policy = {
            "figmaDescriptionFields": ["sourceRef", "sourceVersion", "compositeType"],
            "variableDescription": "compact-json",
            "textStyleDescription": "compact-json",
            "manifest": "generated/composites/typography-figma.json",
            "manifestRole": "complete-binding-migration-and-version-record",
            "sourceOfTruth": "DesignSystem/source/typography.json",
        }
        if mapping.get("provenancePolicy") != expected_provenance_policy:
            self.add(
                "error",
                "TYPOGRAPHY-FIGMA-PROVENANCE-POLICY",
                "/types/typography/figmaMapping/provenancePolicy",
                "Figma Typography 来源追溯位置或 Manifest 责任边界与 V2 契约不一致。",
            )
        expected_sync_diagnostics = {
            "report": "generated/reports/typography-figma-sync-diagnostics.json",
            "defaultSeverity": "error",
            "blocking": True,
            "requiredFields": [
                "severity", "code", "phase", "target", "sourceRef",
                "sourceVersion", "location", "message", "blocking",
            ],
            "categories": [
                {"code": "FIGMA-FONT-PREFLIGHT", "phase": "preflight"},
                {"code": "FIGMA-COMPILE-BLOCKED", "phase": "compile"},
                {"code": "FIGMA-TARGET-STRUCTURE-MISMATCH", "phase": "target-audit"},
                {"code": "FIGMA-VARIABLE-CONTRACT-MISMATCH", "phase": "target-audit"},
                {"code": "FIGMA-TEXT-STYLE-CONTRACT-MISMATCH", "phase": "target-audit"},
            ],
        }
        if mapping.get("syncDiagnostics") != expected_sync_diagnostics:
            self.add(
                "error",
                "TYPOGRAPHY-FIGMA-DIAGNOSTIC-CONTRACT",
                "/types/typography/figmaMapping/syncDiagnostics",
                "Figma Typography 同步错误分类或报文契约与 V2 规范不一致。",
            )
        mode_policy = mapping.get("modePolicy", {})
        if tuple(mode_policy.get("modes", [])) != PROFILE_MODES:
            self.add("error", "TYPOGRAPHY-FIGMA-MODES", "/types/typography/figmaMapping/modePolicy/modes",
                     "Figma Typography mode 必须为 standard、wide。")
        if mode_policy.get("defaultMode") != "standard":
            self.add("error", "TYPOGRAPHY-FIGMA-DEFAULT-MODE",
                     "/types/typography/figmaMapping/modePolicy/defaultMode",
                     "Figma Typography 默认 mode 必须为 standard。")
        if mapping.get("migrationAsset") != "source/migrations/token-aliases.json":
            self.add("error", "TYPOGRAPHY-MIGRATION-ASSET",
                     "/types/typography/figmaMapping/migrationAsset",
                     "Typography Figma 迁移资产路径未登记。")

        policy = self.package.get("policy", {})
        for key in ("fontFamily", "fontFallback"):
            target = reference(policy.get(key))
            if not target:
                self.add("error", "TYPOGRAPHY-POLICY-REFERENCE", f"/policy/{key}",
                         f"policy.{key} 必须是 Token 引用。")
            else:
                try:
                    self.resolvers["standard"].resolve(target)
                except (KeyError, TypeError, ValueError) as exc:
                    self.add("error", "TYPOGRAPHY-POLICY-REFERENCE-MISSING", f"/policy/{key}",
                             f"无法解析 policy.{key}：{exc}")
        if policy.get("fontPackage") != "harmony-os":
            self.add("error", "TYPOGRAPHY-FONT-PACKAGE", "/policy/fontPackage",
                     "当前 Typography 必须使用 harmony-os 字体包。")
        if policy.get("fallbackPolicy") != "harmony-os-sans-then-harmony-os-sans-sc":
            self.add("error", "TYPOGRAPHY-FALLBACK-POLICY", "/policy/fallbackPolicy",
                     "字体 fallback 顺序必须为 HarmonyOS Sans → HarmonyOS Sans SC。")

        for identity, (role, preset, entry) in sorted(self.entries.items()):
            base = f"/tokens/{role}/{preset}"
            for member in REQUIRED_MEMBERS:
                if member not in entry:
                    self.add("error", "TYPOGRAPHY-MEMBER-MISSING", f"{base}/{member}",
                             f"Typography preset 缺少必需成员 {member}。")
                else:
                    for mode in PROFILE_MODES:
                        self._resolve_member(entry[member], member, f"{base}/{member}", mode)
            for member in sorted(set(entry) - set(REQUIRED_MEMBERS)):
                self.add("warning", "TYPOGRAPHY-MEMBER-EXTRA", f"{base}/{member}",
                         "未登记的 Typography Composite 成员不会进入平台产物。")

        aliases = self.aliases.get("figma", {}).get("styleMappings", {})
        if not isinstance(aliases, dict) or not aliases:
            self.add("error", "TYPOGRAPHY-FIGMA-ALIASES", "/figma/styleMappings",
                     "Typography Figma 迁移映射不能为空。")
        else:
            for legacy, target in sorted(aliases.items()):
                if self._target_entry(target) is None:
                    self.add("error", "TYPOGRAPHY-FIGMA-TARGET-MISSING",
                             f"/figma/styleMappings/{legacy}",
                             f"迁移目标不存在：{target}。")

        errors = sum(item["severity"] == "error" for item in self.diagnostics)
        warnings = sum(item["severity"] == "warning" for item in self.diagnostics)
        return {
            "$description": "Typography Composite validation report. Generated by the V2 compiler.",
            "schemaVersion": "2.0",
            "phase": "typography-composite-validation",
            "packageStatus": self.package.get("$status"),
            "releaseBlocked": self.package.get("$status") != "active" or errors > 0,
            "compositeCount": len(self.entries),
            "profileModes": list(PROFILE_MODES),
            "summary": {"errors": errors, "warnings": warnings},
            "diagnostics": self.diagnostics,
        }

    def _materialize(self, role: str, preset: str, entry: dict[str, Any], mode: str,
                     *, css: bool) -> dict[str, Any]:
        result = {
            "sourceRef": f"typography.{role}.{preset}",
            "sourceVersion": self.package.get("$version"),
            "compositeType": "typography",
        }
        for member in REQUIRED_MEMBERS:
            target = reference(entry.get(member))
            if not target:
                continue
            resolved = self.resolvers[mode].resolve(target)
            result[member] = {
                "ref": target,
                "type": resolved.type,
                "value": css_value(resolved) if css else qt_value(resolved),
                "unit": resolved.unit,
            }
        return result

    def platform_payload(self, platform: str, *, css: bool) -> dict[str, Any]:
        typography: dict[str, dict[str, Any]] = {}
        for _, (role, preset, entry) in sorted(self.entries.items()):
            typography.setdefault(role, {})[preset] = {
                mode: self._materialize(role, preset, entry, mode, css=css)
                for mode in PROFILE_MODES
            }
        policy = self.package.get("policy", {})
        return {
            "$description": f"Generated {platform} Typography Composite values from reviewed V2 sources. Do not hand-edit.",
            "$meta": {
                "packageStatus": self.package.get("$status"),
                "releaseBlocked": self.package.get("$status") != "active",
                "sourceRef": "DesignSystem/source/typography.json",
                "sourceVersion": self.package.get("$version"),
                "compositeType": "typography",
                "fontFamily": qt_value(self.resolvers["standard"].resolve(reference(policy["fontFamily"]))),
                "fontFallback": qt_value(self.resolvers["standard"].resolve(reference(policy["fontFallback"]))),
            },
            "modes": list(PROFILE_MODES),
            "typography": typography,
        }

    def figma_manifest(self) -> dict[str, Any]:
        mapping = self.registry["figmaMapping"]
        policy = self.package.get("policy", {})
        styles = []
        for identity, (role, preset, _) in sorted(self.entries.items()):
            styles.append({
                "name": f"DSM/Typography/{role}/{preset}",
                "sourceRef": identity,
                "sourceVersion": self.package.get("$version"),
                "compositeType": "typography",
                "modes": list(PROFILE_MODES),
                "applicationEntry": "text-style",
                "variableBindings": mapping.get("variableBindings", {}),
                "fontFamily": reference(policy.get("fontFamily")),
                "fontFallback": reference(policy.get("fontFallback")),
            })
        return {
            "$description": "Generated Figma Typography mapping manifest. It describes the V2 target and never writes to Figma.",
            "$meta": {
                "sourceRef": "DesignSystem/source/typography.json",
                "sourceVersion": self.package.get("$version"),
                "compositeType": "typography",
                "packageStatus": self.package.get("$status"),
                "releaseBlocked": self.package.get("$status") != "active",
            },
            "variableCollection": mapping.get("variableCollection"),
            "textStylePattern": mapping.get("textStylePattern"),
            "variableNamePatterns": mapping.get("variableNamePatterns"),
            "figmaSyncPolicy": mapping.get("figmaSyncPolicy"),
            "provenancePolicy": mapping.get("provenancePolicy"),
            "syncDiagnostics": mapping.get("syncDiagnostics"),
            "modePolicy": mapping.get("modePolicy"),
            "fallbackHandling": mapping.get("fallbackHandling"),
            "fallbackVariableAllowed": mapping.get("fallbackVariableAllowed"),
            "styles": styles,
            "legacyMappings": self.aliases.get("figma", {}).get("styleMappings", {}),
            "writePolicy": "manifest-only-until-registry-version-check-passes",
        }


def main(argv: list[str] | None = None) -> int:
    from .output_guard import require_build_workspace
    require_build_workspace(ROOT)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("local", "ci", "release"), default="local")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    compiler = TypographyCompiler(args.phase)
    report = compiler.validate()
    report_bytes = (json.dumps(report, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    outputs = {
        OUTPUT / "typography-qt.json": (json.dumps(compiler.platform_payload("Qt", css=False), ensure_ascii=False, indent=2) + "\n").encode("utf-8"),
        OUTPUT / "typography-vue.json": (json.dumps(compiler.platform_payload("Vue", css=True), ensure_ascii=False, indent=2) + "\n").encode("utf-8"),
        OUTPUT / "typography-figma.json": (json.dumps(compiler.figma_manifest(), ensure_ascii=False, indent=2) + "\n").encode("utf-8"),
    }
    if args.check:
        if not REPORT.exists() or REPORT.read_bytes() != report_bytes:
            print("Typography diagnostic drift")
            return 1
        for path, content in outputs.items():
            if not path.exists() or path.read_bytes() != content:
                print(f"Typography output drift: {path}")
                return 1
    else:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.mkdir(parents=True, exist_ok=True)
        if not REPORT.exists() or REPORT.read_bytes() != report_bytes:
            REPORT.write_bytes(report_bytes)
        for path, content in outputs.items():
            if not path.exists() or path.read_bytes() != content:
                path.write_bytes(content)
    summary = report["summary"]
    print(f"Typography Composite {'check' if args.check else 'validation'} | presets={report['compositeCount']} | errors={summary['errors']} | warnings={summary['warnings']} | releaseBlocked={report['releaseBlocked']}")
    return 1 if summary["errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
