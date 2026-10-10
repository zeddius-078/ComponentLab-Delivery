from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
README_PATH = ROOT / "README.md"
OUTPUT_PATH = ROOT / "Orders-Design-System-任务量与依赖甘特图.png"

FONT_REGULAR = Path(r"C:\Windows\Fonts\msyh.ttc")
FONT_BOLD = Path(r"C:\Windows\Fonts\msyhbd.ttc")

SIZE_UNITS = {"小": 1, "中": 2, "大": 4}
PHASES = ["M1", "M2", "M3", "M4", "后续"]
PHASE_TITLES = {
    "M1": "范围、任务与排期基线",
    "M2": "标准组件与基础资产",
    "M3": "平台实施与真实业务试点",
    "M4": "重新生成与复查",
    "后续": "试点后的完整建设",
}
PHASE_COLORS = {
    "M1": "#147D87",
    "M2": "#2D6FB7",
    "M3": "#6B57A5",
    "M4": "#B9652F",
    "后续": "#2F7A55",
}
PHASE_TINTS = {
    "M1": "#E7F4F4",
    "M2": "#EAF1F9",
    "M3": "#F0EDF7",
    "M4": "#F8EEE8",
    "后续": "#EAF4EE",
}


@dataclass
class Task:
    task_id: str
    name: str
    result: str
    size: str
    dependencies: list[str]
    phase: str
    start: int = 0
    end: int = 0
    row_y: int = 0


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONT_BOLD if bold else FONT_REGULAR), size=size)


def parse_readme() -> tuple[list[Task], str]:
    text = README_PATH.read_text(encoding="utf-8")
    version_match = re.search(r"当前版本：`([^`]+)`", text)
    version = version_match.group(1) if version_match else "unknown"
    tasks: list[Task] = []

    for line in text.splitlines():
        if not re.match(r"^\| (?:M[1-4]-\d{2}|后续-\d{2}) \|", line):
            continue
        parts = [part.strip() for part in line.strip().strip("|").split("|")]
        if len(parts) != 5:
            raise ValueError(f"任务行必须包含 5 列：{line}")
        task_id, name, result, size, dependency_text = parts
        if size not in SIZE_UNITS:
            raise ValueError(f"{task_id} 使用了未知工作量：{size}")
        dependencies = [] if dependency_text == "-" else dependency_text.split("、")
        phase = "后续" if task_id.startswith("后续-") else task_id.split("-")[0]
        tasks.append(Task(task_id, name, result, size, dependencies, phase))

    ids = [task.task_id for task in tasks]
    if len(tasks) != 73:
        raise ValueError(f"README 应包含 73 项任务，实际为 {len(tasks)} 项")
    if len(ids) != len(set(ids)):
        raise ValueError("README 中存在重复任务编号")
    missing = sorted({dep for task in tasks for dep in task.dependencies if dep not in set(ids)})
    if missing:
        raise ValueError(f"存在找不到的前置任务：{', '.join(missing)}")
    return tasks, version


def schedule(tasks: list[Task]) -> None:
    by_id = {task.task_id: task for task in tasks}
    unresolved = set(by_id)
    while unresolved:
        progressed = False
        for task in tasks:
            if task.task_id not in unresolved:
                continue
            if any(dep in unresolved for dep in task.dependencies):
                continue
            task.start = max((by_id[dep].end for dep in task.dependencies), default=0)
            task.end = task.start + SIZE_UNITS[task.size]
            unresolved.remove(task.task_id)
            progressed = True
        if not progressed:
            raise ValueError(f"任务依赖存在循环：{', '.join(sorted(unresolved))}")


def text_width(draw: ImageDraw.ImageDraw, value: str, text_font: ImageFont.FreeTypeFont) -> int:
    box = draw.textbbox((0, 0), value, font=text_font)
    return box[2] - box[0]


def fit_text(
    draw: ImageDraw.ImageDraw,
    value: str,
    text_font: ImageFont.FreeTypeFont,
    max_width: int,
) -> str:
    if text_width(draw, value, text_font) <= max_width:
        return value
    suffix = "..."
    kept = value
    while kept and text_width(draw, kept + suffix, text_font) > max_width:
        kept = kept[:-1]
    return kept + suffix


def rounded_rect(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    fill: str,
    radius: int = 12,
    outline: str | None = None,
    width: int = 1,
) -> None:
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def draw_chart(tasks: list[Task], version: str) -> None:
    width = 11600
    left = 100
    right = width - 100
    title_top = 56
    title_height = 300
    column_header_height = 128
    phase_row_height = 78
    task_row_height = 88
    footer_height = 230

    grouped = {phase: [task for task in tasks if task.phase == phase] for phase in PHASES}
    table_top = title_top + title_height
    body_top = table_top + column_header_height
    body_height = len(tasks) * task_row_height + len(PHASES) * phase_row_height
    height = body_top + body_height + footer_height

    image = Image.new("RGB", (width, height), "#F5F7F9")
    draw = ImageDraw.Draw(image)

    title_font = font(64, bold=True)
    subtitle_font = font(28)
    chip_font = font(26, bold=True)
    header_font = font(28, bold=True)
    phase_font = font(28, bold=True)
    id_font = font(24, bold=True)
    task_font = font(28, bold=True)
    small_font = font(22)
    unit_font = font(20)

    rounded_rect(draw, (left, title_top, right, table_top - 18), fill="#FFFFFF", radius=16, outline="#D9E0E6")
    draw.text((left + 34, title_top + 34), "Orders Design System 全量任务量与依赖甘特图", font=title_font, fill="#1E2A33")
    draw.text(
        (left + 36, title_top + 124),
        "任务量与依赖基线，不是日期承诺｜实际日历排期在团队可用时间确认后生成",
        font=subtitle_font,
        fill="#5E6B75",
    )

    counts = Counter(task.size for task in tasks)
    chips = [
        ("首批试点 55 项", "#EAF1F9", "#2D6FB7"),
        ("后续任务 18 项", "#EAF4EE", "#2F7A55"),
        (f"小 {counts['小']} 项", "#EEF2F5", "#425466"),
        (f"中 {counts['中']} 项", "#FFF4D9", "#8A6116"),
        (f"大 {counts['大']} 项", "#F8E8E4", "#A44B37"),
    ]
    chip_x = left + 36
    chip_y = title_top + 202
    for label, fill, color in chips:
        chip_w = text_width(draw, label, chip_font) + 52
        rounded_rect(draw, (chip_x, chip_y, chip_x + chip_w, chip_y + 56), fill=fill, radius=10)
        draw.text((chip_x + 26, chip_y + 11), label, font=chip_font, fill=color)
        chip_x += chip_w + 18

    generated = datetime.now().strftime("%Y-%m-%d %H:%M")
    meta = f"README {version}｜73 项唯一任务｜生成时间 {generated}"
    meta_w = text_width(draw, meta, small_font)
    draw.text((right - meta_w - 34, chip_y + 17), meta, font=small_font, fill="#75818A")

    id_x = left
    task_x = id_x + 430
    size_x = task_x + 2800
    dep_x = size_x + 500
    timeline_x = dep_x + 1700
    timeline_right = right
    timeline_width = timeline_right - timeline_x

    draw.rectangle((left, table_top, right, body_top), fill="#E9EEF2")
    columns = [
        (id_x, task_x, "编号"),
        (task_x, size_x, "任务"),
        (size_x, dep_x, "工作量"),
        (dep_x, timeline_x, "前置任务"),
        (timeline_x, timeline_right, "相对工作顺序"),
    ]
    for x1, x2, label in columns:
        draw.line((x1, table_top, x1, height - footer_height), fill="#CFD8DF", width=2)
        label_w = text_width(draw, label, header_font)
        draw.text((x1 + max(18, (x2 - x1 - label_w) // 2), table_top + 44), label, font=header_font, fill="#34414A")
    draw.line((right, table_top, right, height - footer_height), fill="#CFD8DF", width=2)

    max_unit = max(task.end for task in tasks)
    grid_step = 5 if max_unit <= 120 else 10
    unit_px = timeline_width / max_unit
    for unit in range(0, max_unit + 1, grid_step):
        x = int(timeline_x + unit * unit_px)
        draw.line((x, body_top, x, body_top + body_height), fill="#DFE5EA", width=2)
        label = str(unit)
        draw.text((x + 7, table_top + 78), label, font=unit_font, fill="#71808B")

    y = body_top
    row_index = 0
    task_positions: dict[str, tuple[int, int, int]] = {}
    for phase in PHASES:
        phase_tasks = grouped[phase]
        color = PHASE_COLORS[phase]
        tint = PHASE_TINTS[phase]
        draw.rectangle((left, y, right, y + phase_row_height), fill=tint)
        draw.rectangle((left, y, left + 12, y + phase_row_height), fill=color)
        total_units = sum(SIZE_UNITS[task.size] for task in phase_tasks)
        phase_label = f"{phase}  {PHASE_TITLES[phase]}｜{len(phase_tasks)} 项｜合计 {total_units} 个相对工作单元"
        draw.text((left + 32, y + 22), phase_label, font=phase_font, fill=color)
        y += phase_row_height

        for task in phase_tasks:
            task.row_y = y
            fill = "#FFFFFF" if row_index % 2 == 0 else "#FAFBFC"
            draw.rectangle((left, y, right, y + task_row_height), fill=fill)
            draw.line((left, y + task_row_height, right, y + task_row_height), fill="#E5EAEE", width=1)

            draw.text((id_x + 24, y + 29), task.task_id, font=id_font, fill=color)
            task_label = fit_text(draw, task.name, task_font, size_x - task_x - 48)
            draw.text((task_x + 24, y + 25), task_label, font=task_font, fill="#1F2A33")

            pill_fill = {"小": "#EEF2F5", "中": "#FFF1CC", "大": "#F7DFD8"}[task.size]
            pill_text = {"小": "#425466", "中": "#825D11", "大": "#9A4330"}[task.size]
            pill_w = 86
            pill_x = size_x + (dep_x - size_x - pill_w) // 2
            rounded_rect(draw, (pill_x, y + 20, pill_x + pill_w, y + 68), fill=pill_fill, radius=10)
            size_label_w = text_width(draw, task.size, task_font)
            draw.text((pill_x + (pill_w - size_label_w) // 2, y + 25), task.size, font=task_font, fill=pill_text)

            dependency_label = "无" if not task.dependencies else "、".join(task.dependencies)
            dependency_label = fit_text(draw, dependency_label, small_font, timeline_x - dep_x - 44)
            draw.text((dep_x + 22, y + 31), dependency_label, font=small_font, fill="#5A6872")

            start_x = int(timeline_x + task.start * unit_px)
            end_x = int(timeline_x + task.end * unit_px)
            center_y = y + task_row_height // 2
            task_positions[task.task_id] = (start_x, end_x, center_y)
            row_index += 1
            y += task_row_height

    by_id = {task.task_id: task for task in tasks}
    for task in tasks:
        if not task.dependencies:
            continue
        critical = max(task.dependencies, key=lambda dep: by_id[dep].end)
        _, predecessor_end_x, predecessor_y = task_positions[critical]
        task_start_x, _, task_y = task_positions[task.task_id]
        connector_color = "#B8C3CB"
        if predecessor_end_x == task_start_x:
            bend_x = task_start_x - 8
        else:
            bend_x = max(predecessor_end_x + 8, task_start_x - 18)
        draw.line((predecessor_end_x, predecessor_y, bend_x, predecessor_y), fill=connector_color, width=2)
        draw.line((bend_x, predecessor_y, bend_x, task_y), fill=connector_color, width=2)
        draw.line((bend_x, task_y, task_start_x, task_y), fill=connector_color, width=2)

    for task in tasks:
        start_x, end_x, center_y = task_positions[task.task_id]
        color = PHASE_COLORS[task.phase]
        bar_top = center_y - 18
        bar_bottom = center_y + 18
        rounded_rect(draw, (start_x, bar_top, max(end_x, start_x + 12), bar_bottom), fill=color, radius=10)
        if end_x - start_x >= 92:
            label = task.size
            label_w = text_width(draw, label, small_font)
            draw.text(((start_x + end_x - label_w) // 2, center_y - 14), label, font=small_font, fill="#FFFFFF")

    footer_top = body_top + body_height
    draw.rectangle((left, footer_top, right, height - 45), fill="#FFFFFF")
    draw.line((left, footer_top, right, footer_top), fill="#CFD8DF", width=2)
    footer_title = "读图口径"
    draw.text((left + 26, footer_top + 34), footer_title, font=header_font, fill="#25333C")
    footer_text = (
        "小=1、中=2、大=4，仅用于比较任务量；横轴表示按前置关系推导出的相对顺序，不等于天数。"
        "详细范围、完成结果和未决事项以同目录 README.md 为唯一事实来源。"
    )
    draw.text((left + 26, footer_top + 92), footer_text, font=small_font, fill="#586771")
    draw.text((left + 26, footer_top + 142), f"生成器：{Path(__file__).name}", font=small_font, fill="#7B8790")

    image.save(OUTPUT_PATH, format="PNG", optimize=True, compress_level=6)
    print(f"tasks={len(tasks)} max_unit={max_unit} size={width}x{height}")
    print(OUTPUT_PATH)


def main() -> None:
    tasks, version = parse_readme()
    schedule(tasks)
    draw_chart(tasks, version)


if __name__ == "__main__":
    main()
