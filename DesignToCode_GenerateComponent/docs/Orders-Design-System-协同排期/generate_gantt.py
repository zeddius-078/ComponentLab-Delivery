from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from datetime import datetime
from difflib import SequenceMatcher
from pathlib import Path
from typing import Any
from zoneinfo import ZoneInfo

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
README_PATH = ROOT / "README.md"
OUTPUT_PATH = ROOT / "Orders-Design-System-任务量与依赖甘特图.png"
SOURCE_JSON_PATH = ROOT / "gantt_source.json"
SOURCE_CSV_PATH = ROOT / "gantt_source.csv"
REPORT_PATH = ROOT / "gantt_sync_report.json"

FONT_REGULAR = Path(r"C:\Windows\Fonts\msyh.ttc")
FONT_BOLD = Path(r"C:\Windows\Fonts\msyhbd.ttc")

SIZE_UNITS = {"小": 1, "中": 2, "大": 4}
PHASES = ["M1", "M2", "M3", "M4", "后续"]
EXPECTED_PHASE_COUNTS = {"M1": 9, "M2": 16, "M3": 18, "M4": 12, "后续": 18}
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
    phase: str
    task_name: str
    expected_result: str
    workload_label: str
    work_units: int
    predecessors: list[str]
    task_status: str
    commitment_scope: str
    start_date: str | None
    end_date: str | None
    date_status: str
    source_section: str
    baseline_start_unit: int = 0
    baseline_end_unit: int = 0
    row_y: int = 0


def now_iso() -> str:
    return datetime.now(ZoneInfo("Asia/Shanghai")).isoformat(timespec="seconds")


def read_readme() -> tuple[str, bytes]:
    raw = README_PATH.read_bytes()
    return raw.decode("utf-8"), raw


def parse_metadata(text: str, raw: bytes) -> dict[str, Any]:
    version_match = re.search(r"当前版本：`([^`]+)`", text)
    updated_match = re.search(r"更新时间：`([^`]+)`", text)
    milestone_confirmed = bool(re.search(r"9\s*月\s*30\s*日进行阶段汇报", text))
    return {
        "readme_version": version_match.group(1) if version_match else "unknown",
        "readme_updated": updated_match.group(1) if updated_match else None,
        "readme_sha256": hashlib.sha256(raw).hexdigest(),
        "generated_at": now_iso(),
        "milestones": [
            {
                "milestone_id": "Milestone/2026-09-30",
                "date": "2026-09-30",
                "label": "阶段汇报",
                "source": "README 3.1 业务范围和时间点",
            }
        ]
        if milestone_confirmed
        else [],
    }


def source_section_for(task_id: str) -> str:
    if task_id.startswith("后续-"):
        return "README / 4 / 首批试点后的后续任务池"
    return f"README / 4 / {task_id.split('-')[0]} 细分任务"


def parse_tasks(text: str) -> list[Task]:
    tasks: list[Task] = []
    row_re = re.compile(r"^\|\s*((?:M[1-4]|后续)-\d{2})\s*\|")
    for line in text.splitlines():
        if not row_re.match(line):
            continue
        parts = [part.strip() for part in line.strip().strip("|").split("|")]
        if len(parts) != 5:
            raise ValueError(f"任务行必须包含 5 列：{line}")
        task_id, name, result, workload, dependency_text = parts
        if workload not in SIZE_UNITS:
            raise ValueError(f"{task_id} 使用未知工作量：{workload}")
        predecessors = (
            []
            if dependency_text == "-"
            else [value.strip() for value in re.split(r"[、,，]", dependency_text) if value.strip()]
        )
        phase = "后续" if task_id.startswith("后续-") else task_id.split("-")[0]
        commitment_scope = "POST_PILOT" if phase == "后续" else "PILOT_CANDIDATE"
        tasks.append(
            Task(
                task_id=task_id,
                phase=phase,
                task_name=name,
                expected_result=result,
                workload_label=workload,
                work_units=SIZE_UNITS[workload],
                predecessors=predecessors,
                task_status="UNSET",
                commitment_scope=commitment_scope,
                start_date=None,
                end_date=None,
                date_status="UNSET",
                source_section=source_section_for(task_id),
            )
        )
    return tasks


def normalize_title(value: str) -> str:
    return re.sub(r"[\s`'\"“”‘’、，,。；;：:（）()\-_/]+", "", value).lower()


def duplicate_candidates(tasks: list[Task]) -> list[dict[str, Any]]:
    by_title: dict[str, list[str]] = defaultdict(list)
    for task in tasks:
        by_title[normalize_title(task.task_name)].append(task.task_id)
    candidates: list[dict[str, Any]] = [
        {"kind": "EXACT_NORMALIZED", "task_ids": ids}
        for ids in by_title.values()
        if len(ids) > 1
    ]
    for index, left in enumerate(tasks):
        left_name = normalize_title(left.task_name)
        for right in tasks[index + 1 :]:
            right_name = normalize_title(right.task_name)
            ratio = SequenceMatcher(None, left_name, right_name).ratio()
            if ratio >= 0.97 and left_name != right_name:
                candidates.append(
                    {
                        "kind": "SIMILAR_TITLE_REVIEW",
                        "task_ids": [left.task_id, right.task_id],
                        "similarity": round(ratio, 3),
                    }
                )
    return candidates


def topological_schedule(tasks: list[Task]) -> list[str]:
    by_id = {task.task_id: task for task in tasks}
    unresolved = set(by_id)
    order: list[str] = []
    while unresolved:
        progressed = False
        for task in tasks:
            if task.task_id not in unresolved:
                continue
            if any(dep in unresolved for dep in task.predecessors):
                continue
            task.baseline_start_unit = max(
                (by_id[dep].baseline_end_unit for dep in task.predecessors), default=0
            )
            task.baseline_end_unit = task.baseline_start_unit + task.work_units
            unresolved.remove(task.task_id)
            order.append(task.task_id)
            progressed = True
        if not progressed:
            raise ValueError(f"任务依赖存在循环：{', '.join(sorted(unresolved))}")
    return order


def validate(tasks: list[Task]) -> dict[str, Any]:
    errors: list[dict[str, Any]] = []
    warnings: list[dict[str, Any]] = []
    ids = [task.task_id for task in tasks]
    id_set = set(ids)
    duplicate_ids = sorted(task_id for task_id, count in Counter(ids).items() if count > 1)
    if duplicate_ids:
        errors.append({"code": "DUPLICATE_TASK_ID", "values": duplicate_ids})
    missing_predecessors = sorted(
        {dep for task in tasks for dep in task.predecessors if dep not in id_set}
    )
    if missing_predecessors:
        errors.append({"code": "MISSING_PREDECESSOR", "values": missing_predecessors})
    self_dependencies = sorted(
        task.task_id for task in tasks if task.task_id in task.predecessors
    )
    if self_dependencies:
        errors.append({"code": "SELF_DEPENDENCY", "values": self_dependencies})
    invalid_workloads = sorted(
        {task.workload_label for task in tasks if task.workload_label not in SIZE_UNITS}
    )
    if invalid_workloads:
        errors.append({"code": "INVALID_WORKLOAD", "values": invalid_workloads})
    phase_counts = Counter(task.phase for task in tasks)
    phase_differences = {
        phase: {"expected": expected, "actual": phase_counts.get(phase, 0)}
        for phase, expected in EXPECTED_PHASE_COUNTS.items()
        if phase_counts.get(phase, 0) != expected
    }
    if phase_differences:
        errors.append({"code": "PHASE_COUNT_MISMATCH", "values": phase_differences})
    if len(tasks) != sum(EXPECTED_PHASE_COUNTS.values()):
        errors.append(
            {
                "code": "TOTAL_COUNT_MISMATCH",
                "expected": sum(EXPECTED_PHASE_COUNTS.values()),
                "actual": len(tasks),
            }
        )
    date_order_errors = [
        task.task_id
        for task in tasks
        if task.start_date and task.end_date and task.end_date < task.start_date
    ]
    if date_order_errors:
        errors.append({"code": "END_BEFORE_START", "values": date_order_errors})
    duplicate_review = duplicate_candidates(tasks)
    if duplicate_review:
        warnings.append({"code": "POSSIBLE_DUPLICATE_TASK", "values": duplicate_review})
    cycle_error: str | None = None
    if not missing_predecessors and not self_dependencies and not duplicate_ids:
        try:
            topological_schedule(tasks)
        except ValueError as exc:
            cycle_error = str(exc)
            errors.append({"code": "DEPENDENCY_CYCLE", "message": cycle_error})
    post_pilot_date_leaks = [
        task.task_id
        for task in tasks
        if task.commitment_scope == "POST_PILOT" and task.date_status != "UNSET"
    ]
    if post_pilot_date_leaks:
        errors.append({"code": "POST_PILOT_DATE_LEAK", "values": post_pilot_date_leaks})
    return {
        "valid": not errors,
        "errors": errors,
        "warnings": warnings,
        "counts": {
            "total_tasks": len(tasks),
            "initial_pilot_tasks": sum(phase_counts.get(p, 0) for p in ["M1", "M2", "M3", "M4"]),
            "post_pilot_tasks": phase_counts.get("后续", 0),
            "phase_counts": dict(phase_counts),
            "dependency_count": sum(len(task.predecessors) for task in tasks),
            "confirmed_date_tasks": sum(task.date_status == "CONFIRMED" for task in tasks),
            "partial_date_tasks": sum(task.date_status == "PARTIAL" for task in tasks),
            "unset_date_tasks": sum(task.date_status == "UNSET" for task in tasks),
            "workload_counts": dict(Counter(task.workload_label for task in tasks)),
            "work_units_total": sum(task.work_units for task in tasks),
        },
        "calendar_available": any(task.date_status == "CONFIRMED" for task in tasks),
        "calendar_blocker": "等待 M1-08 确认任务级开始日期、结束日期与 9 月 30 日前承诺范围",
        "cycle_error": cycle_error,
    }


def file_audit() -> list[dict[str, str]]:
    records: list[dict[str, str]] = []
    generated_names = {SOURCE_JSON_PATH.name, SOURCE_CSV_PATH.name, REPORT_PATH.name}
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(ROOT).as_posix()
        if rel == "README.md":
            category, reason = "AUTHORITATIVE", "唯一任务事实源"
        elif rel.startswith("_obsolete/"):
            category, reason = "OBSOLETE", "README 明确标记为历史留存"
        elif rel == OUTPUT_PATH.name:
            category, reason = "REFERENCE", "由 README 生成的当前任务量与依赖结构参考"
        elif rel == "generate_gantt.py":
            category, reason = "REFERENCE", "README 驱动的可复用解析与生成工具"
        elif rel.startswith("__pycache__/") or rel in generated_names:
            category, reason = "GENERATED", "可由脚本重新生成"
        else:
            category, reason = "UNKNOWN", "README 未声明用途，需人工确认"
        records.append({"path": rel, "category": category, "reason": reason})
    return records


def build_payload(tasks: list[Task], metadata: dict[str, Any], validation: dict[str, Any]) -> dict[str, Any]:
    topological_order = topological_schedule(tasks) if validation["valid"] else []
    return {
        "schema_version": "orders-gantt-source/1.0",
        "source": {
            "path": str(README_PATH),
            "readme_version": metadata["readme_version"],
            "readme_updated": metadata["readme_updated"],
            "readme_sha256": metadata["readme_sha256"],
            "generated_at": metadata["generated_at"],
            "source_of_truth": True,
        },
        "sync": {
            "mode": "BASELINE",
            "task_count": len(tasks),
            "dependency_count": validation["counts"]["dependency_count"],
            "confirmed_date_task_count": validation["counts"]["confirmed_date_tasks"],
            "calendar_available": validation["calendar_available"],
            "calendar_blocker": validation["calendar_blocker"],
        },
        "workload_mapping": SIZE_UNITS,
        "milestones": metadata["milestones"],
        "topological_order": topological_order,
        "tasks": [asdict(task) for task in tasks],
    }


def load_previous_payload() -> dict[str, Any] | None:
    if not SOURCE_JSON_PATH.exists():
        return None
    return json.loads(SOURCE_JSON_PATH.read_text(encoding="utf-8"))


def dry_run_diff(tasks: list[Task], previous: dict[str, Any] | None, validation: dict[str, Any]) -> dict[str, Any]:
    current = {task.task_id: asdict(task) for task in tasks}
    previous_tasks = {task["task_id"]: task for task in (previous or {}).get("tasks", [])}
    added = sorted(set(current) - set(previous_tasks))
    archived = sorted(set(previous_tasks) - set(current))
    updated: list[str] = []
    date_changes: list[str] = []
    dependency_changes: list[str] = []
    for task_id in sorted(set(current) & set(previous_tasks)):
        old = previous_tasks[task_id]
        new = current[task_id]
        if old != new:
            updated.append(task_id)
        if (old.get("start_date"), old.get("end_date"), old.get("date_status")) != (
            new.get("start_date"), new.get("end_date"), new.get("date_status")
        ):
            date_changes.append(task_id)
        if old.get("predecessors") != new.get("predecessors"):
            dependency_changes.append(task_id)
    return {
        "new": added,
        "update": updated,
        "move_to_archive": archived,
        "date_changes": date_changes,
        "dependency_changes": dependency_changes,
        "errors": validation["errors"],
    }


def write_standardized_files(payload: dict[str, Any], validation: dict[str, Any]) -> None:
    SOURCE_JSON_PATH.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    fieldnames = [
        "task_id", "phase", "task_name", "expected_result", "workload_label",
        "work_units", "predecessors", "task_status", "commitment_scope",
        "start_date", "end_date", "date_status", "source_section",
        "baseline_start_unit", "baseline_end_unit",
    ]
    with SOURCE_CSV_PATH.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for task in payload["tasks"]:
            row = {key: task.get(key) for key in fieldnames}
            row["predecessors"] = "、".join(task["predecessors"])
            writer.writerow(row)
    previous_figma: dict[str, Any] = {}
    if REPORT_PATH.exists():
        try:
            previous_figma = json.loads(REPORT_PATH.read_text(encoding="utf-8")).get("figma", {})
        except (json.JSONDecodeError, OSError, TypeError):
            previous_figma = {}
    figma_state = {
        "file_key": "jAOWkleaIWm6F52N93fQhb",
        "target_node": "47:1284",
        "section_id": None,
        "frame_ids": {},
        "rollback": "删除本轮 Orders Design System / Gantt Section；README 与本地生成文件不受影响",
    }
    figma_state.update(previous_figma)
    report = {
        "schema_version": "orders-gantt-sync-report/1.0",
        "source": payload["source"],
        "sync": payload["sync"],
        "validation": validation,
        "file_audit": file_audit(),
        "figma": figma_state,
    }
    REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONT_BOLD if bold else FONT_REGULAR), size=size)


def text_width(draw: ImageDraw.ImageDraw, value: str, text_font: ImageFont.FreeTypeFont) -> int:
    box = draw.textbbox((0, 0), value, font=text_font)
    return box[2] - box[0]


def fit_text(draw: ImageDraw.ImageDraw, value: str, text_font: ImageFont.FreeTypeFont, max_width: int) -> str:
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
    draw.text((left + 36, title_top + 124), "任务量与依赖基线，不是日期承诺｜实际日历排期在团队可用时间确认后生成", font=subtitle_font, fill="#5E6B75")
    counts = Counter(task.workload_label for task in tasks)
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
    meta = f"README {version}｜{len(tasks)} 项唯一任务｜生成时间 {now_iso()}"
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
        (id_x, task_x, "编号"), (task_x, size_x, "任务"), (size_x, dep_x, "工作量"),
        (dep_x, timeline_x, "前置任务"), (timeline_x, timeline_right, "相对工作顺序"),
    ]
    for x1, x2, label in columns:
        draw.line((x1, table_top, x1, height - footer_height), fill="#CFD8DF", width=2)
        label_w = text_width(draw, label, header_font)
        draw.text((x1 + max(18, (x2 - x1 - label_w) // 2), table_top + 44), label, font=header_font, fill="#34414A")
    draw.line((right, table_top, right, height - footer_height), fill="#CFD8DF", width=2)
    max_unit = max(task.baseline_end_unit for task in tasks)
    grid_step = 5 if max_unit <= 120 else 10
    unit_px = timeline_width / max_unit
    for unit in range(0, max_unit + 1, grid_step):
        x = int(timeline_x + unit * unit_px)
        draw.line((x, body_top, x, body_top + body_height), fill="#DFE5EA", width=2)
        draw.text((x + 7, table_top + 78), str(unit), font=unit_font, fill="#71808B")
    y = body_top
    row_index = 0
    task_positions: dict[str, tuple[int, int, int]] = {}
    for phase in PHASES:
        phase_tasks = grouped[phase]
        color = PHASE_COLORS[phase]
        tint = PHASE_TINTS[phase]
        draw.rectangle((left, y, right, y + phase_row_height), fill=tint)
        draw.rectangle((left, y, left + 12, y + phase_row_height), fill=color)
        total_units = sum(task.work_units for task in phase_tasks)
        phase_label = f"{phase}  {PHASE_TITLES[phase]}｜{len(phase_tasks)} 项｜合计 {total_units} 个相对工作单元"
        draw.text((left + 32, y + 22), phase_label, font=phase_font, fill=color)
        y += phase_row_height
        for task in phase_tasks:
            task.row_y = y
            fill = "#FFFFFF" if row_index % 2 == 0 else "#FAFBFC"
            draw.rectangle((left, y, right, y + task_row_height), fill=fill)
            draw.line((left, y + task_row_height, right, y + task_row_height), fill="#E5EAEE", width=1)
            draw.text((id_x + 24, y + 29), task.task_id, font=id_font, fill=color)
            task_label = fit_text(draw, task.task_name, task_font, size_x - task_x - 48)
            draw.text((task_x + 24, y + 25), task_label, font=task_font, fill="#1F2A33")
            pill_fill = {"小": "#EEF2F5", "中": "#FFF1CC", "大": "#F7DFD8"}[task.workload_label]
            pill_text = {"小": "#425466", "中": "#825D11", "大": "#9A4330"}[task.workload_label]
            pill_w = 86
            pill_x = size_x + (dep_x - size_x - pill_w) // 2
            rounded_rect(draw, (pill_x, y + 20, pill_x + pill_w, y + 68), fill=pill_fill, radius=10)
            label_w = text_width(draw, task.workload_label, task_font)
            draw.text((pill_x + (pill_w - label_w) // 2, y + 25), task.workload_label, font=task_font, fill=pill_text)
            dep_label = "无" if not task.predecessors else "、".join(task.predecessors)
            dep_label = fit_text(draw, dep_label, small_font, timeline_x - dep_x - 44)
            draw.text((dep_x + 22, y + 31), dep_label, font=small_font, fill="#5A6872")
            start_x = int(timeline_x + task.baseline_start_unit * unit_px)
            end_x = int(timeline_x + task.baseline_end_unit * unit_px)
            center_y = y + task_row_height // 2
            task_positions[task.task_id] = (start_x, end_x, center_y)
            row_index += 1
            y += task_row_height
    by_id = {task.task_id: task for task in tasks}
    for task in tasks:
        if not task.predecessors:
            continue
        critical = max(task.predecessors, key=lambda dep: by_id[dep].baseline_end_unit)
        _, predecessor_end_x, predecessor_y = task_positions[critical]
        task_start_x, _, task_y = task_positions[task.task_id]
        bend_x = task_start_x - 8 if predecessor_end_x == task_start_x else max(predecessor_end_x + 8, task_start_x - 18)
        draw.line((predecessor_end_x, predecessor_y, bend_x, predecessor_y), fill="#B8C3CB", width=2)
        draw.line((bend_x, predecessor_y, bend_x, task_y), fill="#B8C3CB", width=2)
        draw.line((bend_x, task_y, task_start_x, task_y), fill="#B8C3CB", width=2)
    for task in tasks:
        start_x, end_x, center_y = task_positions[task.task_id]
        color = PHASE_COLORS[task.phase]
        rounded_rect(draw, (start_x, center_y - 18, max(end_x, start_x + 12), center_y + 18), fill=color, radius=10)
        if end_x - start_x >= 92:
            label_w = text_width(draw, task.workload_label, small_font)
            draw.text(((start_x + end_x - label_w) // 2, center_y - 14), task.workload_label, font=small_font, fill="#FFFFFF")
    footer_top = body_top + body_height
    draw.rectangle((left, footer_top, right, height - 45), fill="#FFFFFF")
    draw.line((left, footer_top, right, footer_top), fill="#CFD8DF", width=2)
    draw.text((left + 26, footer_top + 34), "读图口径", font=header_font, fill="#25333C")
    footer_text = "小=1、中=2、大=4，仅用于比较任务量；横轴表示按前置关系推导出的相对顺序，不等于天数。详细范围、完成结果和未决事项以同目录 README.md 为唯一事实来源。"
    draw.text((left + 26, footer_top + 92), footer_text, font=small_font, fill="#586771")
    draw.text((left + 26, footer_top + 142), f"生成器：{Path(__file__).name}", font=small_font, fill="#7B8790")
    image.save(OUTPUT_PATH, format="PNG", optimize=True, compress_level=6)


def main() -> None:
    parser = argparse.ArgumentParser(description="README 驱动的 Orders Design System 甘特图同步工具")
    parser.add_argument("--validate", action="store_true", help="解析并校验 README")
    parser.add_argument("--baseline", action="store_true", help="生成 BASELINE 数据与任务量依赖图")
    parser.add_argument("--calendar", action="store_true", help="检查 CALENDAR 是否可以生成")
    parser.add_argument("--dry-run", action="store_true", help="与上一版生成数据比较，不写文件")
    args = parser.parse_args()
    if not any([args.validate, args.baseline, args.calendar, args.dry_run]):
        args.validate = True
    text, raw = read_readme()
    metadata = parse_metadata(text, raw)
    tasks = parse_tasks(text)
    validation = validate(tasks)
    payload = build_payload(tasks, metadata, validation)
    result: dict[str, Any] = {"source": payload["source"], "validation": validation}
    if args.dry_run:
        result["dry_run"] = dry_run_diff(tasks, load_previous_payload(), validation)
    if args.baseline:
        if not validation["valid"]:
            raise SystemExit(json.dumps(result, ensure_ascii=False, indent=2))
        write_standardized_files(payload, validation)
        draw_chart(tasks, metadata["readme_version"])
        result["baseline"] = {
            "status": "GENERATED",
            "source_json": str(SOURCE_JSON_PATH),
            "source_csv": str(SOURCE_CSV_PATH),
            "sync_report": str(REPORT_PATH),
            "image": str(OUTPUT_PATH),
        }
    if args.calendar:
        result["calendar"] = {
            "status": "AVAILABLE" if validation["calendar_available"] else "BLOCKED",
            "confirmed_date_tasks": validation["counts"]["confirmed_date_tasks"],
            "milestones": metadata["milestones"],
            "blocker": None if validation["calendar_available"] else validation["calendar_blocker"],
        }
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
