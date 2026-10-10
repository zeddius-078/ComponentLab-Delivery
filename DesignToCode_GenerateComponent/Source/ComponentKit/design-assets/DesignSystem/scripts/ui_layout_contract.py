"""把 Qt Designer XML 中的视觉布局参数与 Token 建立可检查、可编译的绑定。

输入：.ui、ui_layout_bindings.json 和已解析的单个 Profile 值表。
输出：检查报告、供 QUiLoader 使用的内存 XML，或同步后的默认 Profile 源 .ui。
这里只替换已存在的数值槽，不修改控件层级、sizePolicy 或 stretch 等结构规则。
零值、Qt 最大尺寸哨兵和伸展占位符的 sizeHint 不计入正值视觉槽。
"""
from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import xml.etree.ElementTree as ET
from typing import Any, Iterable


QWIDGETSIZE_MAX = 16777215
LAYOUT_PROPERTIES = {
    "leftMargin",
    "topMargin",
    "rightMargin",
    "bottomMargin",
    "spacing",
    "horizontalSpacing",
    "verticalSpacing",
}
SIZE_PROPERTIES = {"minimumSize", "maximumSize"}
NUMBER_SIZE_PROPERTIES = {
    "minimumWidth",
    "minimumHeight",
    "maximumWidth",
    "maximumHeight",
    "fixedWidth",
    "fixedHeight",
}


@dataclass(frozen=True)
class LayoutSlot:
    """单个可绑定数值槽；category 区分间距与尺寸，value 保存源 XML 的整数值。"""
    binding_id: str
    category: str
    value: int
    ui_path: Path


def contract_ui_files(root: Path) -> list[Path]:
    """限定正式页面、组件模板和 Showcase 的检查范围，不递归扫描历史产物。"""
    paths: list[Path] = []
    for directory in (
        root / "app" / "views" / "qt_designer_xml" / "screens_1440x1024",
        root / "app" / "components" / "ui",
    ):
        if directory.exists():
            paths.extend(sorted(directory.glob("*.ui")))
    showcase = root / "app" / "views" / "qt_designer_xml" / "design_system_showcase.ui"
    if showcase.exists():
        paths.append(showcase)
    return paths


def _binding_id(file_name: str, kind: str, owner: str, prop: str, field: str = "") -> str:
    """用文件名、对象名和属性定位槽位；改名后应同步绑定，不依赖易变的 XML 行号。"""
    parts = [file_name, kind, owner, prop]
    if field:
        parts.append(field)
    return "|".join(parts)


def _direct_properties(owner: ET.Element) -> Iterable[ET.Element]:
    return owner.findall("./property")


def _positive_int(element: ET.Element | None) -> int | None:
    # 0 表示无间距，负值可能表示 Qt 默认行为，最大尺寸值表示无上限，均不是视觉尺度。
    if element is None or element.text is None:
        return None
    try:
        value = int(element.text.strip())
    except ValueError:
        return None
    if value <= 0 or value == QWIDGETSIZE_MAX:
        return None
    return value


def _is_expanding_spacer(spacer: ET.Element) -> bool:
    # 伸展占位由剩余空间决定；其 sizeHint 不是固定留白，不应机械迁移为尺寸 Token。
    for prop in _direct_properties(spacer):
        if prop.get("name") != "sizeType":
            continue
        value = prop.findtext("enum", default="")
        return value.endswith("Expanding") or value.endswith("MinimumExpanding")
    return False


def _slot_elements(ui_path: Path) -> dict[str, tuple[str, ET.Element]]:
    """扫描明确的视觉属性；只读对象自身 property，避免把子控件属性归给父对象。"""
    root = ET.parse(ui_path).getroot()
    result: dict[str, tuple[str, ET.Element]] = {}
    file_name = ui_path.name

    for widget in root.iter("widget"):
        owner = widget.get("name", "")
        for prop in _direct_properties(widget):
            name = prop.get("name", "")
            if name == "geometry":
                rect = prop.find("rect")
                if rect is None:
                    continue
                for field in ("x", "y", "width", "height"):
                    element = rect.find(field)
                    if _positive_int(element) is not None:
                        result[_binding_id(file_name, "widget", owner, name, field)] = (
                            "dimension",
                            element,
                        )
            elif name in SIZE_PROPERTIES:
                size = prop.find("size")
                if size is None:
                    continue
                for field in ("width", "height"):
                    element = size.find(field)
                    if _positive_int(element) is not None:
                        result[_binding_id(file_name, "widget", owner, name, field)] = (
                            "dimension",
                            element,
                        )
            elif name in NUMBER_SIZE_PROPERTIES:
                element = prop.find("number")
                if _positive_int(element) is not None:
                    result[_binding_id(file_name, "widget", owner, name)] = ("dimension", element)

    for layout in root.iter("layout"):
        owner = layout.get("name", "")
        for prop in _direct_properties(layout):
            name = prop.get("name", "")
            if name not in LAYOUT_PROPERTIES:
                continue
            element = prop.find("number")
            if _positive_int(element) is not None:
                result[_binding_id(file_name, "layout", owner, name)] = ("space", element)

    for spacer in root.iter("spacer"):
        if _is_expanding_spacer(spacer):
            continue
        owner = spacer.get("name", "")
        for prop in _direct_properties(spacer):
            if prop.get("name") != "sizeHint":
                continue
            size = prop.find("size")
            if size is None:
                continue
            for field in ("width", "height"):
                element = size.find(field)
                if _positive_int(element) is not None:
                    result[_binding_id(file_name, "spacer", owner, "sizeHint", field)] = (
                        "dimension",
                        element,
                    )
    return result


def collect_layout_slots(ui_paths: Iterable[Path]) -> dict[str, LayoutSlot]:
    """汇总槽位并拒绝跨文件重名绑定；仅文件名参与 ID，所以 .ui 文件名必须唯一。"""
    slots: dict[str, LayoutSlot] = {}
    for ui_path in ui_paths:
        for binding_id, (category, element) in _slot_elements(ui_path).items():
            if binding_id in slots:
                raise ValueError(f"Duplicate .ui layout binding ID: {binding_id}")
            slots[binding_id] = LayoutSlot(
                binding_id=binding_id,
                category=category,
                value=int(element.text or "0"),
                ui_path=ui_path,
            )
    return slots


def bootstrap_layout_contract(
    primitives_path: Path,
    bindings_path: Path,
    ui_paths: Iterable[Path],
) -> dict[str, int]:
    """从 XML 建立迁移草稿并写回两个 Source JSON，不是日常自动生成步骤。

    已有语义/组件绑定优先保留。按数值创建的 semantic.uiLayout 只是迁移落点，
    不能据此认定设计语义已成熟；复用区域仍需人工归并成具名组件 Token。
    """
    primitives = json.loads(primitives_path.read_text(encoding="utf-8"))
    previous = {}
    if bindings_path.exists():
        previous = json.loads(bindings_path.read_text(encoding="utf-8")).get("bindings", {})

    slots = collect_layout_slots(ui_paths)
    base = primitives.setdefault("base", {}).setdefault("uiLayout", {})
    base["$description"] = (
        "Qt Designer 原始布局数值层；页面和组件不得直接消费，应通过 semantic 或 component Alias。"
    )
    semantic = primitives.setdefault("semantic", {}).setdefault("uiLayout", {})
    semantic["$description"] = (
        "Qt 布局适配语义层。迁移值用于全量约束 XML；稳定复用区域应继续提升为 component Token。"
    )

    bindings: dict[str, str] = {}
    for binding_id, slot in sorted(slots.items()):
        category = slot.category
        value_key = str(slot.value)
        base_group = base.setdefault(category, {"$type": "spacing" if category == "space" else "dimension"})
        base_group.setdefault("$type", "spacing" if category == "space" else "dimension")
        base_group.setdefault(value_key, {"$value": slot.value})
        semantic_group = semantic.setdefault(
            category,
            {"$type": "spacing" if category == "space" else "dimension"},
        )
        semantic_group.setdefault("$type", "spacing" if category == "space" else "dimension")
        semantic_group.setdefault(
            value_key,
            {
                "$value": f"{{base.uiLayout.{category}.{value_key}}}",
                "$description": "Qt XML 迁移语义值；复用成熟后应提升为具名组件 Token。",
            },
        )

        old_path = str(previous.get(binding_id, ""))
        if old_path.startswith("semantic.") or old_path.startswith("component."):
            bindings[binding_id] = old_path
        else:
            bindings[binding_id] = f"semantic.uiLayout.{category}.{value_key}"

    payload = {
        "$schemaVersion": 2,
        "$bindingKey": "ui-file|owner-kind|object-name|property|optional-field",
        "$description": "Qt Designer XML 中所有有意义的正值视觉布局槽均须绑定 Token。",
        "$rules": {
            "allowedLayers": ["semantic", "component"],
            "excluded": [
                "显式零值边距或间距",
                "QWIDGETSIZE_MAX (16777215)",
                "Expanding / MinimumExpanding spacer 的 sizeHint",
                "stretch 与 sizePolicy 等结构属性",
            ],
        },
        "bindings": bindings,
    }
    primitives_path.write_text(
        json.dumps(primitives, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    bindings_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    return {"slots": len(slots), "bindings": len(bindings)}


def validate_layout_contract(
    bindings_payload: dict[str, Any],
    ui_paths: Iterable[Path],
    profile_values: dict[str, Any],
    *,
    compare_source_values: bool = True,
) -> dict[str, Any]:
    """检查正值槽逐项有绑定、无失效项且来自语义/组件层，默认核对 Designer 预览值。

    compare_source_values=False 只关闭预览值比较，不豁免缺失绑定或无效 Token。
    此覆盖率仅描述扫描范围内的显式正值属性，不代表所有运行时代码都已 Token 化。
    """
    bindings = bindings_payload.get("bindings", {})
    if not isinstance(bindings, dict):
        raise TypeError("ui_layout_bindings.json bindings must be an object")
    slots = collect_layout_slots(ui_paths)
    failures: list[str] = []
    for binding_id, slot in slots.items():
        token_path = bindings.get(binding_id)
        if not token_path:
            failures.append(f"未绑定: {binding_id}")
            continue
        if not str(token_path).startswith(("semantic.", "component.")):
            failures.append(f"页面直接消费非语义 Token: {binding_id} -> {token_path}")
            continue
        if token_path not in profile_values:
            failures.append(f"Token 不存在: {binding_id} -> {token_path}")
            continue
        value = profile_values[token_path]
        if not isinstance(value, (int, float)):
            failures.append(f"布局 Token 不是数值: {token_path}")
            continue
        if compare_source_values and round(float(value)) != slot.value:
            failures.append(
                f"默认 Profile 与 .ui 不一致: {binding_id}={slot.value}, {token_path}={value}"
            )
    for binding_id in bindings:
        if binding_id not in slots:
            failures.append(f"失效绑定: {binding_id}")
    if failures:
        raise ValueError(".ui 布局合同失败:\n" + "\n".join(failures[:20]))
    return {
        "meaningfulPositiveSlots": len(slots),
        "boundSlots": len(bindings),
        "coveragePercent": 100.0 if not slots else round(len(bindings) * 100.0 / len(slots), 2),
        "directBaseBindings": 0,
        "exceptions": list(bindings_payload.get("$rules", {}).get("excluded", [])),
    }


def _apply_values_to_tree(
    ui_path: Path,
    bindings_payload: dict[str, Any],
    profile_values: dict[str, Any],
) -> tuple[ET.ElementTree, bool]:
    """按当前 Profile 改写树中的数值；取整为 Qt XML 整数，不在这里写入磁盘。"""
    tree = ET.parse(ui_path)
    bindings = bindings_payload.get("bindings", {})
    changed = False
    for binding_id, (_, element) in _slot_elements_from_root(ui_path.name, tree.getroot()).items():
        token_path = bindings.get(binding_id)
        if not token_path:
            raise KeyError(f"Cannot compile unbound .ui layout slot: {binding_id}")
        if token_path not in profile_values:
            raise KeyError(f"Cannot resolve .ui layout Token: {token_path}")
        value = profile_values[token_path]
        if not isinstance(value, (int, float)):
            raise TypeError(f".ui layout Token must be numeric: {token_path}")
        rendered = str(round(float(value)))
        if element.text != rendered:
            element.text = rendered
            changed = True
    return tree, changed


def _slot_elements_from_root(file_name: str, root: ET.Element) -> dict[str, tuple[str, ET.Element]]:
    # 直接返回目标树中的元素以便原位改值。规则与 _slot_elements 重复，新增属性时
    # 必须同步维护两个扫描入口，避免静态检查能识别、运行时却没有编译的情况。
    result: dict[str, tuple[str, ET.Element]] = {}
    for widget in root.iter("widget"):
        owner = widget.get("name", "")
        for prop in _direct_properties(widget):
            name = prop.get("name", "")
            if name == "geometry":
                rect = prop.find("rect")
                if rect is not None:
                    for field in ("x", "y", "width", "height"):
                        element = rect.find(field)
                        if _positive_int(element) is not None:
                            result[_binding_id(file_name, "widget", owner, name, field)] = ("dimension", element)
            elif name in SIZE_PROPERTIES:
                size = prop.find("size")
                if size is not None:
                    for field in ("width", "height"):
                        element = size.find(field)
                        if _positive_int(element) is not None:
                            result[_binding_id(file_name, "widget", owner, name, field)] = ("dimension", element)
            elif name in NUMBER_SIZE_PROPERTIES:
                element = prop.find("number")
                if _positive_int(element) is not None:
                    result[_binding_id(file_name, "widget", owner, name)] = ("dimension", element)
    for layout in root.iter("layout"):
        owner = layout.get("name", "")
        for prop in _direct_properties(layout):
            name = prop.get("name", "")
            element = prop.find("number")
            if name in LAYOUT_PROPERTIES and _positive_int(element) is not None:
                result[_binding_id(file_name, "layout", owner, name)] = ("space", element)
    for spacer in root.iter("spacer"):
        if _is_expanding_spacer(spacer):
            continue
        owner = spacer.get("name", "")
        for prop in _direct_properties(spacer):
            if prop.get("name") != "sizeHint":
                continue
            size = prop.find("size")
            if size is None:
                continue
            for field in ("width", "height"):
                element = size.find(field)
                if _positive_int(element) is not None:
                    result[_binding_id(file_name, "spacer", owner, "sizeHint", field)] = ("dimension", element)
    return result


def compile_ui_xml(
    ui_path: Path,
    bindings_payload: dict[str, Any],
    profile_values: dict[str, Any],
) -> bytes:
    """生成当前 Profile 的 UTF-8 XML 字节交给 QUiLoader，不覆盖 Designer 源文件。"""
    tree, _ = _apply_values_to_tree(ui_path, bindings_payload, profile_values)
    ET.indent(tree, space=" ")
    return ET.tostring(tree.getroot(), encoding="utf-8", xml_declaration=True)


def sync_ui_layouts(
    bindings_payload: dict[str, Any],
    ui_paths: Iterable[Path],
    default_profile_values: dict[str, Any],
) -> int:
    """只将默认 Profile 同步回源 .ui 保持预览一致；其他 Profile 不产生独立 .ui。"""
    changed_files = 0
    for ui_path in ui_paths:
        tree, changed = _apply_values_to_tree(ui_path, bindings_payload, default_profile_values)
        if not changed:
            continue
        ET.indent(tree, space=" ")
        tree.write(ui_path, encoding="utf-8", xml_declaration=True)
        changed_files += 1
    return changed_files
