#!/usr/bin/env python3
"""将分层设计令牌编译为 Qt 样式、运行时数值、CSV 账本和中文设计系统文档。

输入均来自 DesignSystem/source：primitives.json 定义 Token；profiles.json 覆盖
已有 Token；widgets.json 将选择器和 QSS 属性映射到 Token。组件目录、布局绑定和
Qt 能力表分别约束组件实现、XML 布局参数及 Qt5/Qt6 共用的样式能力。

值的组织关系是 base -> semantic -> component.basic / component.composite。
Theme 与 Density 只选择覆盖值，不增加 Token 层，也不允许在 Profile 中新增路径。
Qt 不解析 {token.path} 引用，必须在生成阶段把它们替换成最终值。

阅读顺序：main -> build_profiles/load_tokens -> _resolve_token -> generate_*。
--check 只校验；--write 生成平台产物；--sync-ui-layouts 单独更新 Designer 预览值。
生成物不作为修改入口，设计调整应回到 Source JSON。
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import itertools
import json
import re
from pathlib import Path
from typing import Any

from ui_layout_contract import (
    bootstrap_layout_contract,
    contract_ui_files,
    sync_ui_layouts,
    validate_layout_contract,
)


ROOT = Path(__file__).resolve().parents[2]
DESIGN_SYSTEM_DIR = ROOT / "DesignSystem"
SOURCE_DIR = DESIGN_SYSTEM_DIR / "source"
GENERATED_DIR = DESIGN_SYSTEM_DIR / "generated"
PRIMITIVES_PATH = SOURCE_DIR / "primitives.json"
PROFILES_PATH = SOURCE_DIR / "profiles.json"
WIDGETS_PATH = SOURCE_DIR / "widgets.json"
COMPONENT_LIBRARY_PATH = SOURCE_DIR / "component_library.json"
UI_LAYOUT_BINDINGS_PATH = SOURCE_DIR / "ui_layout_bindings.json"
QSS_CAPABILITIES_PATH = SOURCE_DIR / "qt_qss_capabilities.json"
STYLES_DIR = DESIGN_SYSTEM_DIR / "styles"
PROFILE_QSS_DIR = STYLES_DIR / "profiles"
BASE_QSS = STYLES_DIR / "base.qss"
COMPONENTS_QSS = STYLES_DIR / "components.qss"
MD_PATH = GENERATED_DIR / "docs" / "DesignSystem.md"
QT_TOKENS_PATH = GENERATED_DIR / "runtime" / "tokens_qt.json"
TOKEN_CATALOG_PATH = GENERATED_DIR / "reports" / "token_catalog.csv"
TOKEN_CONTRACT_PATH = GENERATED_DIR / "reports" / "token_contract.json"

REFERENCE_RE = re.compile(r"\{([a-zA-Z0-9_.-]+)\}")


def source_revision() -> str:
    """按固定顺序散列六份公共 Source，供 PythonGUI 与 CGY 核对设计决策版本。"""
    digest = hashlib.sha256()
    for path in (
        PRIMITIVES_PATH,
        PROFILES_PATH,
        WIDGETS_PATH,
        COMPONENT_LIBRARY_PATH,
        UI_LAYOUT_BINDINGS_PATH,
        QSS_CAPABILITIES_PATH,
    ):
        digest.update(path.name.encode("utf-8"))
        digest.update(path.read_bytes())
    return digest.hexdigest()[:12]


def platform_source_revision() -> str:
    """记录 Python 编译器版本；散列源码字节，因此仅修改注释也会改变此版本号。"""
    digest = hashlib.sha256()
    for path in (Path(__file__), Path(__file__).with_name("ui_layout_contract.py")):
        digest.update(path.name.encode("utf-8"))
        digest.update(path.read_bytes())
    return digest.hexdigest()[:12]


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate QSS, runtime JSON, Token CSV and DesignSystem.md from design tokens.")
    parser.add_argument("--qss-only", action="store_true")
    parser.add_argument("--md-only", action="store_true")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="只读验证全部 Token、QSS、组件和 .ui 合同")
    mode.add_argument("--write", action="store_true", help="生成全部平台产物（默认行为）")
    mode.add_argument("--sync-ui-layouts", action="store_true", help="将默认 Profile 布局值同步到源 .ui")
    mode.add_argument(
        "--bootstrap-ui-layout-bindings",
        action="store_true",
        help="为新增 .ui 建立布局绑定迁移草稿，并补齐 Base/Semantic Alias",
    )
    args = parser.parse_args()

    ui_files = contract_ui_files(ROOT)
    if args.bootstrap_ui_layout_bindings:
        stats = bootstrap_layout_contract(PRIMITIVES_PATH, UI_LAYOUT_BINDINGS_PATH, ui_files)
        print(f"Qt .ui 布局绑定已初始化: {stats['bindings']}/{stats['slots']}")
        return 0

    # 先检查输入合同再写产物，避免把错误 Alias、缺失状态或布局遗漏发布到运行端。
    raw = load_json(PRIMITIVES_PATH)
    profile_config = load_json(PROFILES_PATH)
    widgets = load_json(WIDGETS_PATH)
    component_library = load_json(COMPONENT_LIBRARY_PATH)
    layout_bindings = load_json(UI_LAYOUT_BINDINGS_PATH)
    qss_capabilities = load_json(QSS_CAPABILITIES_PATH)
    token_types = collect_token_types(raw)
    validate_layer_dependencies(raw)
    validate_alias_types(raw)
    profiles, profile_meta = build_profiles(raw, profile_config)
    # 源 .ui 保存默认 Profile 的预览值；其他 Profile 由加载器在内存中编译。
    tokens = profiles[profile_meta["defaultProfile"]]
    validate_component_library(component_library, tokens, widgets)
    qss_report = validate_qss_contract(widgets, token_types, qss_capabilities)
    layout_report = validate_layout_contract(layout_bindings, ui_files, tokens)

    if args.sync_ui_layouts:
        changed = sync_ui_layouts(layout_bindings, ui_files, tokens)
        print(f"Qt .ui 默认 Profile 已同步: changed={changed}")
        return 0

    if args.check:
        print("Commerly Token 合同检查通过")
        print(f"  tokens: {len(tokens)}")
        print(f"  qss declarations: {qss_report['declarations']}")
        print(f"  ui layout: {layout_report['boundSlots']}/{layout_report['meaningfulPositiveSlots']}")
        return 0

    # 所有写入均在 --check 返回之后，保证只读检查不会重写 Source 或生成物。
    for output_path in (MD_PATH, QT_TOKENS_PATH, TOKEN_CATALOG_PATH, TOKEN_CONTRACT_PATH):
        output_path.parent.mkdir(parents=True, exist_ok=True)

    print("Commerly Token Generator")
    print(f"  source: {PRIMITIVES_PATH.relative_to(ROOT)}")
    print(f"  tokens: {len(tokens)}")
    print(f"  profiles: {', '.join(profiles)}")

    if not args.md_only:
        generate_qss(tokens, profiles)
        generate_qt_tokens(tokens, profiles, token_types, profile_meta)
        generate_token_catalog(raw, profiles, profile_meta)
    if not args.qss_only:
        generate_md(raw, tokens, profile_config, profile_meta, component_library)
    generate_token_contract(
        tokens=tokens,
        profiles=profiles,
        qss_report=qss_report,
        layout_report=layout_report,
        component_library=component_library,
    )
    return 0


def validate_component_library(
    library: dict[str, Any],
    tokens: dict[str, Any],
    widgets: dict[str, Any],
) -> None:
    """核对组件状态、实现路径及 Token/QSS 引用；Figma 证据可选，不阻断自有组件。"""
    components = library.get("components")
    if not isinstance(components, dict) or not components:
        raise ValueError("component_library.json must define at least one component")

    valid_levels = {"basic", "composite"}
    valid_implementations = {"styled-native", "custom-widget", "ui-composite"}
    valid_evidence_levels = {"component", "reference", "project-owned"}
    node_pattern = re.compile(r"^\d+[:\-]\d+$")
    # 必需状态独立于组件自身的 states，防止删掉状态后校验也随之失去依据。
    required_states = library.get("$requiredStates", {})
    if not isinstance(required_states, dict):
        raise TypeError("component_library.json $requiredStates must be an object")
    for component_id, states in required_states.items():
        contract = components.get(component_id)
        if not isinstance(contract, dict):
            raise KeyError(f"Required component contract is missing: {component_id}")
        declared_states = contract.get("states", [])
        if not isinstance(declared_states, list):
            raise TypeError(f"Component states must be an array: {component_id}")
        missing_states = [state for state in states if state not in declared_states]
        if missing_states:
            raise KeyError(
                f"Required component states are missing: {component_id} / {', '.join(missing_states)}"
            )
    for component_id, contract in components.items():
        if not isinstance(contract, dict):
            raise TypeError(f"Component contract must be an object: {component_id}")
        if contract.get("level") not in valid_levels:
            raise ValueError(f"Invalid component level: {component_id}")
        if contract.get("implementation") not in valid_implementations:
            raise ValueError(f"Invalid component implementation: {component_id}")

        # 公共目录共享语义，本生成器只核验 PySide2 实现；旧 qt 字段保留兼容读取。
        platforms = contract.get("platforms", {})
        qt = platforms.get("pyside2", contract.get("qt")) if isinstance(platforms, dict) else contract.get("qt")
        if not isinstance(qt, dict) or not qt.get("class"):
            raise ValueError(f"Component requires platforms.pyside2.class: {component_id}")
        module = qt.get("module")
        if module:
            module_path = ROOT / (str(module).replace(".", "/") + ".py")
            if not module_path.exists():
                raise FileNotFoundError(f"Component module is missing: {component_id} / {module}")
        ui_path = qt.get("ui")
        if ui_path and not (ROOT / str(ui_path)).exists():
            raise FileNotFoundError(f"Component UI is missing: {component_id} / {ui_path}")

        for prefix in contract.get("tokenPrefixes", []):
            if not any(path == prefix or path.startswith(f"{prefix}.") for path in tokens):
                raise KeyError(f"Unknown component Token prefix: {component_id} / {prefix}")
        for qss_contract in contract.get("qssContracts", []):
            if qss_contract not in widgets:
                raise KeyError(f"Unknown QSS contract: {component_id} / {qss_contract}")

        for evidence in contract.get("designEvidence", []):
            if not isinstance(evidence, dict) or evidence.get("level") not in valid_evidence_levels:
                raise ValueError(f"Invalid design evidence: {component_id}")
            if evidence.get("level") == "project-owned":
                continue
            if not evidence.get("provider") or not evidence.get("fileKey"):
                raise ValueError(f"Design evidence requires provider and fileKey: {component_id}")
            if not node_pattern.fullmatch(str(evidence.get("nodeId", ""))):
                raise ValueError(f"Invalid design node ID: {component_id}")


def validate_qss_contract(
    widgets: dict[str, Any],
    token_types: dict[str, str],
    capabilities: dict[str, Any],
) -> dict[str, Any]:
    """依据公共能力表拒绝不支持的 QSS 属性、缺失状态、直接视觉值和类型错配。"""
    supported = capabilities.get("supportedProperties", {})
    allowed_syntax = capabilities.get("allowedSyntaxValues", {})
    forbidden_properties = set(capabilities.get("forbiddenProperties", []))
    forbidden_patterns = [str(value).lower() for value in capabilities.get("forbiddenValuePatterns", [])]
    if not isinstance(supported, dict) or not supported:
        raise ValueError("qt_qss_capabilities.json must define supportedProperties")

    # 只验证现有状态块会漏掉“整个 disabled 块被删除”，因此先检查必需块清单。
    required_blocks = widgets.get("$requiredBlocks", {})
    if not isinstance(required_blocks, dict):
        raise TypeError("widgets.json $requiredBlocks must be an object")
    for contract_name, selectors in required_blocks.items():
        contract = widgets.get(contract_name)
        if not isinstance(contract, dict):
            raise KeyError(f"Required QSS contract is missing: {contract_name}")
        for selector in selectors:
            if selector not in contract or not isinstance(contract[selector], dict):
                raise KeyError(f"Required QSS state block is missing: {contract_name} / {selector}")

    declaration_count = 0
    referenced_tokens: set[str] = set()
    errors: list[str] = []
    direct_value_pattern = re.compile(r"(?<![\w.-])([1-9]\d*(?:\.\d+)?)\s*(?:px|pt|em|%)\b", re.I)
    for contract_name, contract in widgets.items():
        if contract_name.startswith("$"):
            continue
        if not isinstance(contract, dict):
            raise TypeError(f"QSS contract must be an object: {contract_name}")
        if contract_name.startswith("@"):
            selector = contract.get("$selector")
            if not isinstance(selector, str) or not selector.strip():
                raise ValueError(f"Reusable QSS contract requires a valid $selector: {contract_name}")
        for block_name, declarations in contract.items():
            if block_name.startswith("$"):
                continue
            if not isinstance(declarations, dict):
                raise TypeError(f"QSS block must be an object: {contract_name} / {block_name}")
            for prop, raw_value in declarations.items():
                declaration_count += 1
                if prop in forbidden_properties or prop not in supported:
                    errors.append(f"Qt5/Qt6 公共 QSS 不支持属性: {contract_name} / {block_name} / {prop}")
                    continue
                value = str(raw_value).strip()
                lowered = value.lower()
                for pattern in forbidden_patterns:
                    if pattern in lowered:
                        errors.append(f"QSS 值包含禁用语法 {pattern}: {contract_name} / {prop}")
                # none、transparent 等白名单值是语法，不是设计决策；尺寸和色值应引用 Token。
                refs = REFERENCE_RE.findall(value)
                syntax_values = {str(item).lower() for item in allowed_syntax.get(prop, [])}
                if not refs and lowered not in syntax_values:
                    errors.append(f"QSS 视觉值必须来自 Token: {contract_name} / {block_name} / {prop}={value}")
                # 即使声明包含 Token，也要检查剩余文本，防止混入硬编码的非零尺寸。
                residual = REFERENCE_RE.sub("", value)
                if direct_value_pattern.search(residual):
                    errors.append(f"QSS 含未 Token 化的非零尺寸: {contract_name} / {block_name} / {prop}={value}")
                allowed_types = set(supported[prop])
                for token_path in refs:
                    referenced_tokens.add(token_path)
                    token_type = token_types.get(token_path)
                    if token_type is None:
                        errors.append(f"QSS 引用了不存在的 Token: {contract_name} / {token_path}")
                        continue
                    if token_type not in allowed_types:
                        errors.append(
                            f"QSS Token 类型错配: {contract_name} / {prop} <- {token_path} ({token_type})"
                        )
    if errors:
        raise ValueError("QSS 合同失败:\n" + "\n".join(errors[:100]))
    return {
        "target": capabilities.get("target", "qt5-qt6-common"),
        "declarations": declaration_count,
        "referencedTokens": len(referenced_tokens),
        "requiredStateContracts": len(required_blocks),
        "unsupportedProperties": 0,
        "typeMismatches": 0,
        "directVisualValues": 0,
    }


def generate_token_contract(
    *,
    tokens: dict[str, Any],
    profiles: dict[str, dict[str, Any]],
    qss_report: dict[str, Any],
    layout_report: dict[str, Any],
    component_library: dict[str, Any],
) -> None:
    """写入本轮静态检查结果和双版本号；passed 不代表截图或交互已经验收。"""
    payload = {
        "$schemaVersion": 1,
        "$description": "Token、QSS、Qt Designer XML、组件与 Profile 的机器可读检查合同。",
        "commonSourceRevision": source_revision(),
        "platformSourceRevision": platform_source_revision(),
        "platform": "pyside2-qt5",
        "tokenCount": len(tokens),
        "profiles": {"count": len(profiles), "names": list(profiles)},
        "qss": qss_report,
        "uiLayout": layout_report,
        "components": {
            "count": len(component_library.get("components", {})),
            "platform": "pyside2",
            "danglingContracts": 0,
        },
        "status": "passed",
    }
    TOKEN_CONTRACT_PATH.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"  wrote: {TOKEN_CONTRACT_PATH.relative_to(ROOT)}")


def load_tokens(raw: dict[str, Any], overrides: dict[str, Any] | None = None) -> dict[str, Any]:
    """将嵌套 Source 展平，应用 Profile 覆盖，再解析为“完整路径 -> 最终值”。"""
    tokens: dict[str, Any] = {}
    _collect_tokens(raw, "", tokens)

    # 覆盖必须早于引用求值，否则 Component 会保留覆盖前的 Semantic 值。
    for path, override in (overrides or {}).items():
        if path not in tokens:
            raise KeyError(f"Profile override targets unknown token: {path}")
        value = override.get("$value") if isinstance(override, dict) and "$value" in override else override
        tokens[path] = _normalize_value(value)

    resolved: dict[str, Any] = {}
    for key in tokens:
        resolved[key] = _resolve_token(key, tokens, resolved, set())
    return resolved


def load_json(path: Path) -> dict[str, Any]:
    """拒绝 JSON 重复键，避免默认解析器静默保留最后一个值、掩盖设计冲突。"""
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Duplicate JSON key in {path.relative_to(ROOT)}: {key}")
            result[key] = value
        return result

    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=reject_duplicates)


def collect_token_types(raw: dict[str, Any]) -> dict[str, str]:
    """读取叶子或父组继承的 $type，供 Alias 类型校验和 Qt 输出分组使用。"""
    token_types: dict[str, str] = {}

    def walk(node: Any, prefix: str, inherited_type: str | None) -> None:
        if not isinstance(node, dict):
            return
        current_type = node.get("$type", inherited_type)
        if "$value" in node:
            token_types[prefix] = str(current_type or "other")
            return
        for key, child in node.items():
            if not key.startswith("$"):
                walk(child, f"{prefix}.{key}" if prefix else key, current_type)

    walk(raw, "", None)
    return token_types


def collect_token_records(raw: dict[str, Any]) -> dict[str, dict[str, Any]]:
    """保留未解析的 $value 和说明，供账本对照“设计引用”与“运行最终值”。"""
    records: dict[str, dict[str, Any]] = {}

    def walk(
        node: Any,
        prefix: str,
        inherited_type: str | None,
        inherited_description: str,
    ) -> None:
        if not isinstance(node, dict):
            return
        current_type = node.get("$type", inherited_type)
        direct_description = str(node.get("$description", ""))
        if "$value" in node:
            records[prefix] = {
                "token_type": str(current_type or "other"),
                "raw_value": node["$value"],
                "description": direct_description,
                "group_description": inherited_description,
            }
            return
        child_description = direct_description or inherited_description
        for key, child in node.items():
            if not key.startswith("$"):
                walk(
                    child,
                    f"{prefix}.{key}" if prefix else key,
                    current_type,
                    child_description,
                )

    walk(raw, "", None, "")
    return records


def validate_layer_dependencies(raw: dict[str, Any], overrides: dict[str, Any] | None = None) -> None:
    """禁止低层依赖高层，如 semantic -> component、basic -> composite。

    这里只检查方向；同层循环与缺失引用由 _resolve_token 阻断。
    允许向更低层引用，不等同于强制每条引用都经过相邻层。
    """
    source_values: dict[str, Any] = {}
    _collect_tokens(raw, "", source_values)
    for path, override in (overrides or {}).items():
        if path not in source_values:
            raise KeyError(f"Profile override targets unknown token: {path}")
        source_values[path] = override.get("$value") if isinstance(override, dict) and "$value" in override else override

    def rank(path: str) -> int:
        if path.startswith("base."):
            return 0
        if path.startswith("semantic."):
            return 1
        if path.startswith("component.basic."):
            return 2
        if path.startswith("component.composite."):
            return 3
        raise ValueError(f"Token is outside the supported layers: {path}")

    violations: list[str] = []
    for source, value in source_values.items():
        if not isinstance(value, str):
            continue
        for target in REFERENCE_RE.findall(value):
            if target not in source_values:
                continue
            if rank(target) > rank(source):
                violations.append(f"{source} -> {target}")
    if violations:
        raise ValueError("Upward token references are not allowed:\n" + "\n".join(violations))


def validate_alias_types(raw: dict[str, Any], overrides: dict[str, Any] | None = None) -> None:
    """整值别名（Alias，如 {semantic.color.text.primary}）必须保持相同 $type。

    混合字符串可能组合多个类型，不套用整值规则；QSS 声明另由属性能力表校验。
    """
    records = collect_token_records(raw)
    for path, override in (overrides or {}).items():
        if path not in records:
            raise KeyError(f"Profile override targets unknown token: {path}")
        records[path]["raw_value"] = (
            override.get("$value")
            if isinstance(override, dict) and "$value" in override
            else override
        )

    errors: list[str] = []
    for source, record in records.items():
        value = record["raw_value"]
        if not isinstance(value, str):
            continue
        match = REFERENCE_RE.fullmatch(value)
        if not match or match.group(1) not in records:
            continue
        target = match.group(1)
        source_type = record["token_type"]
        target_type = records[target]["token_type"]
        if source_type != target_type:
            errors.append(f"{source} ({source_type}) <- {target} ({target_type})")
    if errors:
        raise TypeError("Token Alias type mismatch:\n" + "\n".join(errors[:100]))


def build_profiles(
    raw: dict[str, Any],
    profile_config: dict[str, Any],
) -> tuple[dict[str, dict[str, Any]], dict[str, Any]]:
    """组合所有环境轴，返回各 Profile 的完整值表及默认项、轴顺序等元信息。"""
    axes = profile_config.get("axes")
    if not isinstance(axes, dict) or not axes:
        raise ValueError("profiles.json must define at least one axis")

    axis_names = list(axes)
    axis_values: list[list[str]] = []
    axis_meta: dict[str, Any] = {}
    for axis_name, axis in axes.items():
        if not isinstance(axis, dict):
            raise TypeError(f"Profile axis must be an object: {axis_name}")
        values = [key for key in axis if not key.startswith("$")]
        default = axis.get("$default")
        if not values or default not in values:
            raise ValueError(f"Profile axis {axis_name!r} has invalid default {default!r}")
        axis_values.append(values)
        axis_meta[axis_name] = {
            "default": default,
            "values": values,
            "description": axis.get("$description", ""),
        }

    # 笛卡尔积使 Theme 与 Density 独立组合，不要求维护四份完整 Token 源文件。
    profiles: dict[str, dict[str, Any]] = {}
    for combination in itertools.product(*axis_values):
        overrides: dict[str, Any] = {}
        for axis_name, value_name in zip(axis_names, combination):
            profile = axes[axis_name][value_name]
            profile_overrides = profile.get("$overrides", {})
            if not isinstance(profile_overrides, dict):
                raise TypeError(f"Profile overrides must be an object: {axis_name}.{value_name}")
            # 同一路径若被多个轴覆盖，当前规则是后轴优先；Source 应避免跨轴争用。
            overrides.update(profile_overrides)
        profile_name = "-".join(combination)
        validate_layer_dependencies(raw, overrides)
        validate_alias_types(raw, overrides)
        profiles[profile_name] = load_tokens(raw, overrides)

    default_profile = "-".join(str(axes[name]["$default"]) for name in axis_names)
    metadata = {
        "source": profile_config.get("$source", {}),
        "defaultProfile": default_profile,
        "axisOrder": axis_names,
        "axes": axis_meta,
    }
    return profiles, metadata


def _collect_tokens(node: Any, prefix: str, out: dict[str, Any]) -> None:
    """只把带 $value 的节点视为 Token；$description 等元数据不进入运行值表。"""
    if isinstance(node, dict):
        if "$value" in node:
            out[prefix] = _normalize_value(node["$value"])
            return
        for key, child in node.items():
            if key.startswith("$"):
                continue
            _collect_tokens(child, f"{prefix}.{key}" if prefix else key, out)


def _resolve_token(
    key: str,
    tokens: dict[str, Any],
    resolved: dict[str, Any],
    stack: set[str],
) -> Any:
    """递归沿引用链求值；resolved 复用已完成结果，stack 检测当前递归链的循环。"""
    if key in resolved:
        return resolved[key]
    if key in stack:
        raise ValueError(f"Circular token reference: {' -> '.join([*stack, key])}")
    if key not in tokens:
        raise KeyError(f"Unknown token: {key}")

    stack.add(key)
    value = tokens[key]
    if isinstance(value, str):
        value = _resolve_references(value, tokens, resolved, stack)
    stack.remove(key)
    resolved[key] = value
    return value


def _resolve_references(
    value: str,
    tokens: dict[str, Any],
    resolved: dict[str, Any],
    stack: set[str],
) -> Any:
    # 整值引用保留数值、布尔等原类型；只有嵌入字符串的引用才转换为文本。
    whole_reference = REFERENCE_RE.fullmatch(value)
    if whole_reference:
        return _resolve_token(whole_reference.group(1), tokens, resolved, stack)

    def replace(match: re.Match[str]) -> str:
        ref = match.group(1)
        return str(_resolve_token(ref, tokens, resolved, stack))

    return REFERENCE_RE.sub(replace, value)


def _normalize_value(value: Any) -> Any:
    """将当前 Source 使用的颜色/阴影对象转成可导出的值，不实现通用色彩空间转换。"""
    if isinstance(value, dict) and "colorSpace" in value and "components" in value:
        return _components_to_css(value["components"])
    # 阴影描述能被导出，不代表 QSS 支持 box-shadow；绘制能力仍由消费端实现。
    if isinstance(value, dict) and {"offsetX", "offsetY", "blur", "spread", "color"} <= set(value):
        color = _components_to_css(value["color"]["components"])
        return f"{value['offsetX']}px {value['offsetY']}px {value['blur']}px {value['spread']}px {color}"
    return value


def _components_to_css(components: list[float]) -> str:
    red = _channel(components[0])
    green = _channel(components[1])
    blue = _channel(components[2])
    if len(components) > 3 and components[3] < 1:
        return f"rgba({red}, {green}, {blue}, {components[3]:.2f})"
    return f"#{red:02X}{green:02X}{blue:02X}"


def _channel(value: float) -> int:
    return max(0, min(255, int(round(float(value) * 255))))


def resolve_value(value: str, tokens: dict[str, Any]) -> str:
    """将 widgets.json 的 Token 占位符替换为已解析值，保留模板中的 px 等单位。"""
    def replace(match: re.Match[str]) -> str:
        path = match.group(1)
        if path not in tokens:
            raise KeyError(f"Missing token reference in widgets.json: {path}")
        return str(tokens[path])

    return REFERENCE_RE.sub(replace, value)


def generate_qss(tokens: dict[str, Any], profiles: dict[str, dict[str, Any]]) -> None:
    """生成默认拆分样式及每个 Profile 的完整 QSS，运行时整份选择而非叠加主题。"""
    widgets = load_json(WIDGETS_PATH)
    STYLES_DIR.mkdir(exist_ok=True)
    PROFILE_QSS_DIR.mkdir(exist_ok=True)

    revision = source_revision()
    base_text, component_text = _render_qss_parts(tokens, widgets, revision)
    BASE_QSS.write_text(base_text, encoding="utf-8")
    COMPONENTS_QSS.write_text(component_text, encoding="utf-8")
    print(f"  wrote: {BASE_QSS.relative_to(ROOT)}")
    print(f"  wrote: {COMPONENTS_QSS.relative_to(ROOT)}")

    for profile_name, profile_tokens in profiles.items():
        profile_base, profile_components = _render_qss_parts(profile_tokens, widgets, revision)
        profile_path = PROFILE_QSS_DIR / f"{profile_name}.qss"
        profile_header = (
            "/* Full generated QSS profile. Do not hand-edit. */\n"
            f"/* profile: {profile_name}; source-revision: {revision} */\n\n"
        )
        profile_path.write_text(
            profile_header + profile_base + "\n" + profile_components,
            encoding="utf-8",
        )
        print(f"  wrote: {profile_path.relative_to(ROOT)}")


def _render_qss_parts(
    tokens: dict[str, Any],
    widgets: dict[str, Any],
    revision: str,
) -> tuple[str, str]:
    base_lines = [
        "/* base.qss - generated from layered design tokens. Do not hand-edit. */",
        f"/* source-revision: {revision} */",
        "/* layers: base -> semantic -> component.basic / component.composite */",
        "",
        "* {",
        f"  font-family: {tokens.get('base.fontFamily.app', tokens['base.fontFamily.primary'])};",
        f"  font-size: {tokens['base.fontSize.body-sm']}px;",
        "}",
        "",
    ]
    component_lines = [
        "/* components.qss - generated from widgets.json. Do not hand-edit. */",
        f"/* source-revision: {revision} */",
        "",
    ]

    for object_name in sorted(widgets):
        style_blocks = widgets[object_name]
        if object_name.startswith("$"):
            continue
        block = _qss_block(object_name, style_blocks, tokens)
        if object_name in {"SignInWindow", "SignUpWindow", "cardArea", "workflowWindow", "dashboardScreen", "ordersScreen"}:
            base_lines.append(block)
        else:
            component_lines.append(block)

    return (
        "\n".join(base_lines).strip() + "\n",
        "\n".join(component_lines).strip() + "\n",
    )


def generate_qt_tokens(
    tokens: dict[str, Any],
    profiles: dict[str, dict[str, Any]],
    token_types: dict[str, str],
    profile_meta: dict[str, Any],
) -> None:
    """输出运行时 Token JSON，与 QSS 使用相同的 Profile 求值结果。

    QSS 负责原生控件样式；布局、资产与 QPainter 需要从此文件取数。
    顶层默认分组保留兼容读取，profiles 保存每个组合的完整分组。
    """
    default_groups = _group_qt_tokens(tokens, token_types)
    qt_tokens: dict[str, Any] = {
        "$description": "Generated PySide2 runtime token adapter. Do not hand-edit.",
        "$meta": {
            "source": profile_meta.get("source", {}),
            "commonSourceRevision": source_revision(),
            "platformSourceRevision": platform_source_revision(),
            "defaultProfile": profile_meta["defaultProfile"],
            "axisOrder": profile_meta["axisOrder"],
            "axes": profile_meta["axes"],
        },
    }
    qt_tokens.update(default_groups)
    qt_tokens["profiles"] = {
        profile_name: _group_qt_tokens(profile_tokens, token_types)
        for profile_name, profile_tokens in profiles.items()
    }
    QT_TOKENS_PATH.write_text(json.dumps(qt_tokens, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"  wrote: {QT_TOKENS_PATH.relative_to(ROOT)}")


def generate_token_catalog(
    raw: dict[str, Any],
    profiles: dict[str, dict[str, Any]],
    profile_meta: dict[str, Any],
) -> None:
    """导出含原始值、引用路径及各 Profile 最终值的全量账本，便于设计与开发核对。"""
    records = collect_token_records(raw)
    default_profile = profile_meta["defaultProfile"]
    profile_names = list(profiles)
    profile_columns = [f"profile_{name.replace('-', '_')}" for name in profile_names]
    header = [
        "record_id",
        "section",
        "layer",
        "category",
        "group_path",
        "token_path",
        "token_type",
        "value_kind",
        "raw_value",
        "reference_paths",
        "default_resolved_value",
        *profile_columns,
        "profile_sensitive",
        "description",
        "group_description",
        "source_file",
    ]

    section_order = {
        "Color": 0,
        "Spacing & Shape": 1,
        "Typography": 2,
        "Effects & Motion": 3,
        "Assets & Terms": 4,
        "Component Basic": 5,
        "Component Composite": 6,
        "Other": 7,
    }
    ordered_paths = sorted(
        records,
        key=lambda path: (
            section_order[_catalog_section(path, records[path]["token_type"])],
            path,
        ),
    )

    TOKEN_CATALOG_PATH.parent.mkdir(parents=True, exist_ok=True)
    # UTF-8 BOM 兼容 Excel 中文读取；稳定排序和换行保证重复生成可比较。
    with TOKEN_CATALOG_PATH.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=header, lineterminator="\n")
        writer.writeheader()
        for index, path in enumerate(ordered_paths, start=1):
            record = records[path]
            raw_text = _catalog_value(record["raw_value"])
            references = sorted(set(REFERENCE_RE.findall(raw_text)))
            profile_values = {
                name: _catalog_value(profiles[name][path])
                for name in profile_names
            }
            parts = path.split(".")
            row = {
                "record_id": f"T{index:04d}",
                "section": _catalog_section(path, record["token_type"]),
                "layer": _catalog_layer(path),
                "category": parts[2] if path.startswith("component.") and len(parts) > 2 else parts[1],
                "group_path": ".".join(parts[:-1]),
                "token_path": path,
                "token_type": record["token_type"],
                "value_kind": "alias" if references else "direct",
                "raw_value": raw_text,
                "reference_paths": " | ".join(references),
                "default_resolved_value": profile_values[default_profile],
                "profile_sensitive": "true" if len(set(profile_values.values())) > 1 else "false",
                "description": record["description"],
                "group_description": record["group_description"],
                "source_file": "DesignSystem/source/primitives.json",
            }
            for profile_name, column in zip(profile_names, profile_columns):
                row[column] = profile_values[profile_name]
            writer.writerow(row)

    print(f"  wrote: {TOKEN_CATALOG_PATH.relative_to(ROOT)}")


def _catalog_layer(path: str) -> str:
    if path.startswith("component.basic."):
        return "component.basic"
    if path.startswith("component.composite."):
        return "component.composite"
    return path.split(".", 1)[0]


def _catalog_section(path: str, token_type: str) -> str:
    if path.startswith("component.basic."):
        return "Component Basic"
    if path.startswith("component.composite."):
        return "Component Composite"
    if token_type == "color":
        return "Color"
    if token_type in {"spacing", "sizing", "dimension", "borderRadius", "borderWidth", "border"}:
        return "Spacing & Shape"
    if token_type in {
        "fontFamily",
        "fontFamilies",
        "fontSize",
        "fontSizes",
        "fontWeight",
        "fontWeights",
        "lineHeight",
        "lineHeights",
        "letterSpacing",
        "typography",
    }:
        return "Typography"
    if token_type in {"boxShadow", "gradient", "duration", "cubicBezier", "transition", "opacity", "number", "boolean"}:
        return "Effects & Motion"
    if token_type in {"asset", "text", "string"} or ".asset." in path or ".term." in path:
        return "Assets & Terms"
    return "Other"


def _catalog_value(value: Any) -> str:
    if isinstance(value, str):
        return value
    if value is None:
        return ""
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (dict, list)):
        return json.dumps(value, ensure_ascii=False, separators=(",", ":"))
    return str(value)


def _group_qt_tokens(tokens: dict[str, Any], token_types: dict[str, str]) -> dict[str, dict[str, Any]]:
    """按 Qt 消费用途分组，键仍保留完整 Token 路径；这不是新增 Token 语义层。"""
    groups = {
        "color": {},
        "font": {},
        "layout": {},
        "motion": {},
        "shape": {},
        "gradient": {},
        "shadow": {},
        "asset": {},
        "term": {},
        "string": {},
        "number": {},
        "boolean": {},
        "other": {},
    }
    font_types = {"fontFamily", "fontFamilies", "fontSize", "fontSizes", "fontWeight", "fontWeights", "lineHeight", "lineHeights", "letterSpacing", "typography"}
    layout_types = {"spacing", "sizing", "dimension", "borderRadius", "borderWidth", "border"}
    motion_types = {"duration", "cubicBezier", "transition"}

    for path, value in sorted(tokens.items()):
        token_type = token_types.get(path, "other")
        lowered = path.lower()
        if path.startswith("component.basic.button.innerShadow.") and (
            ".geometry." in path or path.endswith(".bgMidStop")
        ):
            group = "shape"
        elif ".motion." in lowered or token_type in motion_types:
            group = "motion"
        elif ".asset." in lowered or token_type == "asset":
            group = "asset"
        elif ".term." in lowered:
            group = "term"
        elif token_type == "color":
            group = "color"
        elif token_type in font_types or ".type." in lowered:
            group = "font"
        elif token_type in layout_types:
            group = "layout"
        elif token_type == "gradient" or "gradient" in lowered:
            group = "gradient"
        elif token_type == "boxShadow":
            group = "shadow"
        elif token_type == "boolean":
            group = "boolean"
        elif token_type in {"number", "opacity"}:
            group = "number"
        elif token_type in {"text", "string"}:
            group = "string"
        else:
            group = "other"
        groups[group][path] = value
    return groups


def _qss_block(object_name: str, style_blocks: dict[str, Any], tokens: dict[str, Any]) -> str:
    """优先生成可复用 ds* 选择器；没有 $selector 时使用 objectName 定位单个对象。"""
    reusable_selector = style_blocks.get("$selector")
    blocks: list[str] = []
    for selector in sorted(style_blocks):
        declarations = style_blocks[selector]
        if selector.startswith("$"):
            continue
        if not isinstance(declarations, dict):
            raise TypeError(f"QSS declarations must be an object: {object_name}.{selector}")
        if reusable_selector:
            qss_selector = _selector_for_reusable(str(reusable_selector), selector)
        else:
            qss_selector = _selector_for_object(selector, object_name)
        lines = [f"{qss_selector} {{"]
        for prop in sorted(declarations):
            value = declarations[prop]
            lines.append(f"  {prop}: {resolve_value(str(value), tokens)};")
        lines.append("}")
        blocks.append("\n".join(lines))
    return "\n\n".join(blocks) + "\n"


def _selector_for_reusable(base_selector: str, selector: str) -> str:
    """把合同中的 & 替换为 Qt 类/ds* 属性选择器，保留 :hover 等状态后缀。"""
    if (
        not base_selector
        or not re.match(r"^[A-Za-z_]", base_selector)
        or any(char in base_selector for char in "{};\r\n")
    ):
        raise ValueError(f"Unsupported reusable Qt selector in widgets.json: {base_selector}")
    if selector == "&":
        return base_selector
    if selector.startswith("&"):
        return base_selector + selector[1:]
    raise ValueError(
        f"Reusable selector blocks must use '&' or '&:state', got: {selector}"
    )


def _selector_for_object(selector: str, object_name: str) -> str:
    match = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)(.*)$", selector)
    if not match:
        raise ValueError(f"Unsupported Qt selector in widgets.json: {selector}")
    widget, suffix = match.groups()
    return f"{widget}#{object_name}{suffix}"


def generate_md(
    raw: dict[str, Any],
    tokens: dict[str, Any],
    profile_config: dict[str, Any],
    profile_meta: dict[str, Any],
    component_library: dict[str, Any],
) -> None:
    """从同一份 Source 生成中文工程参考；规则说明改模板，设计值改 Source。"""
    revision = source_revision()
    token_records = collect_token_records(raw)
    token_types = collect_token_types(raw)
    layer_counts = {
        "base": sum(path.startswith("base.") for path in tokens),
        "semantic": sum(path.startswith("semantic.") for path in tokens),
        "component.basic": sum(path.startswith("component.basic.") for path in tokens),
        "component.composite": sum(path.startswith("component.composite.") for path in tokens),
    }
    type_counts: dict[str, int] = {}
    for token_type in token_types.values():
        type_counts[token_type] = type_counts.get(token_type, 0) + 1

    lines = [
        "# Commerly Design System 工程参考",
        "",
        f"> 由 `DesignSystem/source/*.json` 自动生成；Source revision：`{revision}`。",
        "> 本文件不可手改。完整逐项数据见 `generated/reports/token_catalog.csv`。",
        "",
        "## 1. 当前模型",
        "",
        "```text",
        "base -> semantic -> component.basic / component.composite",
        "           + Theme(light/dark) x Density(comfortable/compact)",
        "             -> QSS + tokens_qt.json -> Qt",
        "```",
        "",
        "| 层 | 职责 | 主要消费者 |",
        "|---|---|---|",
        "| `base` | 原始颜色、字号、间距、圆角、资产等事实 | Token 作者 |",
        "| `semantic` | 产品背景、文字、操作、状态、空间等含义 | Component Token |",
        "| `component.basic` | Button、Input、Card、Switch 等基础控件合同 | QSS / 自绘 Widget |",
        "| `component.composite` | Navbar、Sidebar、Table、Orders 等复合合同 | 页面与 Controller |",
        "",
        "Basic 与 Composite 是组件层内部的复杂度分区。Theme 与 Density 是环境选择轴，不是新的 Token 层。",
        "",
        "## 2. 四个 Profile",
        "",
        f"默认 Profile：`{profile_meta['defaultProfile']}`。Theme 只改变颜色语义，Density 只改变尺寸、间距和排版密度。",
        "",
        "| Profile | Theme | Density |",
        "|---|---|---|",
    ]
    axis_order = profile_meta["axisOrder"]
    axis_values = [profile_meta["axes"][name]["values"] for name in axis_order]
    for combination in itertools.product(*axis_values):
        name = "-".join(combination)
        values = dict(zip(axis_order, combination))
        lines.append(f"| `{name}` | `{values['theme']}` | `{values['density']}` |")

    lines.extend(
        [
            "",
            "## 3. Qt 消费方式",
            "",
            "| 决策 | 工程通路 |",
            "|---|---|",
            "| 颜色、字体、边框、状态 | `widgets.json -> styles/profiles/*.qss` |",
            "| 全量布局槽 | `ui_layout_bindings.json -> 活动 Profile 内存 XML -> QUiLoader` |",
            "| 资产、行高和兼容属性 | `.ui ds*Token -> LayoutTokenService / AssetBindingService` |",
            "| 内阴影、Switch 等复杂绘制 | `tokens_qt.json -> app/components -> QPainter` |",
            "",
            "QSS 不支持 CSS Variable，也不能设置 `QLayout`。生成器把 Alias 解析为最终 QSS 值；619 个有意义正值布局槽通过外部绑定在 QUiLoader 前编译，零值、最大尺寸哨兵和 Expanding spacer 是结构例外。",
            "",
            "## 4. 主按钮示例",
            "",
            "```text",
            "base.color.brand.deep-forest.500",
            "  -> semantic.color.action.primary",
            "  -> component.basic.button.primary.background",
            "  -> widgets.json / QPushButton[dsVariant=\"primary\"]",
            "  -> profile QSS -> Qt `.ui`",
            "```",
            "",
            "## 5. 内阴影例外",
            "",
            "`SolidInnerShadowButton` 仍使用 `component.basic.button.innerShadow.*`，但通过 `tokens_qt.json` 和 QPainter 消费。状态优先级为 `disabled > pressed > hover > default`；Profile 切换时清理缓存并重绘。算法见 `docs/内阴影算法.md`。",
            "",
            "## 6. Qt 组件目录",
            "",
            "组件目录描述共同逻辑合同，并以 `platforms.pyside2` / `platforms.qt-cpp` 分别登记实现路径。它不是新的 Token 层；Figma 证据允许为正式组件、视觉参考或项目自有实现。",
            "",
            "| 组件 ID | 级别 | 实现 | Qt 类 | 设计证据 |",
            "|---|---|---|---|---|",
            *[
                "| `{}` | `{}` | `{}` | `{}` | {} |".format(
                    component_id,
                    contract.get("level", ""),
                    contract.get("implementation", ""),
                    contract.get("platforms", {}).get("pyside2", contract.get("qt", {})).get("class", ""),
                    ", ".join(
                        f"{item.get('level')}:{item.get('nodeId', 'project')}"
                        for item in contract.get("designEvidence", [])
                    ),
                )
                for component_id, contract in component_library.get("components", {}).items()
            ],
            "",
            "## 7. 数量快照",
            "",
            "| Token 层 | 数量 |",
            "|---|---:|",
            *[f"| `{layer}` | {count} |" for layer, count in layer_counts.items()],
            f"| **合计** | **{len(tokens)}** |",
            "",
            "| Token 类型 | 数量 |",
            "|---|---:|",
            *[f"| `{token_type}` | {count} |" for token_type, count in sorted(type_counts.items())],
            "",
            f"CSV 全量账本共 {len(token_records)} 行。数量表示设计系统表达能力，不等于所有 Token 已在页面中使用。",
            "",
            "## 8. 修改规则",
            "",
            "1. Token 修改进入 `primitives.json` / `profiles.json`；QSS 映射进入 `widgets.json`；组件映射进入 `component_library.json`；布局进入 `ui_layout_bindings.json`。",
            "2. 运行 `python DesignSystem/scripts/generate.py --check`，再运行 `--write`。",
            "3. 审查 QSS、运行时 JSON、CSV、`token_contract.json` 和本文件差异。",
            "4. 运行 `python tools/check_project.py`。",
            "5. 不直接修改 `generated/` 和 `styles/`。",
            "",
            "详细工程链路见 `docs/Token到Qt工程链路.md`。",
            "",
        ]
    )
    MD_PATH.write_text("\n".join(lines), encoding="utf-8")
    print(f"  wrote: {MD_PATH.relative_to(ROOT)}")



if __name__ == "__main__":
    raise SystemExit(main())
