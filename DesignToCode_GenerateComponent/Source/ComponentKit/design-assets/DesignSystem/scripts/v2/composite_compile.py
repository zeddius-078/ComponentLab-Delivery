"""Validate V2 Composite packages and publish only reviewed token slices.

The Motion package may remain candidate while an explicitly reviewed slice is
released. Candidate slices are validated but never enter platform outputs.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "DesignSystem/source"
REPORT = ROOT / "DesignSystem/generated/reports/composite-diagnostics.json"
COMPOSITE_OUTPUT = ROOT / "DesignSystem/generated/composites"
ALIAS = re.compile(r"^\{([a-z0-9]+(?:[.-][a-z0-9]+)*)\}$")
COMPONENTS = ("button", "input", "select")


def load(path: Path) -> Any:
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Duplicate JSON key in {path.name}: {key}")
            result[key] = value
        return result

    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique)


class MotionValidator:
    def __init__(self, phase: str):
        # The report is a generated contract and must be byte-stable between
        # local generation and CI drift checks.  Keep the requested phase for
        # validation policy only; diagnostics use the stable package phase.
        self.phase = phase
        self.report_phase = "composite-validation"
        self.registry = load(SOURCE / "composite-registry.json")["types"]["motion"]
        self.package = load(SOURCE / "motion.json")
        self.component_registry = load(SOURCE / "component-registry.json")
        self.primitive = load(SOURCE / "primitive.json")["root"]["primitive"]["motion"]
        self.entries: dict[str, dict] = {}
        self.atoms: dict[str, dict] = {}
        self.diagnostics: list[dict] = []
        self.released = set(self.package.get("policy", {}).get("releasedSlices", []))
        self._flatten(self.package.get("tokens", {}), "", self.entries, composite=True)
        self._flatten(self.primitive, "primitive.motion", self.atoms, composite=False)

    def _flatten(self, node: Any, prefix: str, out: dict[str, dict], *, composite: bool) -> None:
        if not isinstance(node, dict):
            return
        if (composite and any(key in node for key in ("tracks", "sequence"))) or (
            not composite and "$type" in node
        ):
            out[prefix] = node
            return
        for key, value in node.items():
            if key.startswith("$"):
                continue
            self._flatten(value, f"{prefix}.{key}" if prefix else key, out, composite=composite)

    def add(self, severity: str, code: str, source_path: str, message: str) -> None:
        self.diagnostics.append({
            "severity": severity,
            "phase": self.report_phase,
            "code": code,
            "sourceFile": "DesignSystem/source/motion.json",
            "sourcePath": source_path,
            "message": message,
        })

    def atom(self, value: Any, prefix: str, source_path: str) -> dict | None:
        if not isinstance(value, str):
            self.add("error", "MOTION-LITERAL-NOT-ALLOWED", source_path,
                     "Motion 值必须引用 Primitive，不能写平台无关数值字面量。")
            return None
        match = ALIAS.fullmatch(value)
        target = match.group(1) if match else ""
        leaf = self.atoms.get(target)
        if not leaf:
            self.add("error", "MOTION-REFERENCE-MISSING", source_path,
                     f"无法解析 Motion Primitive 引用：{value}")
            return None
        if not target.startswith(f"primitive.motion.{prefix}."):
            self.add("error", "MOTION-REFERENCE-DOMAIN", source_path,
                     f"{value} 不属于 primitive.motion.{prefix}。")
            return None
        return leaf

    def resolved_atom(self, value: Any, prefix: str, source_path: str) -> Any:
        leaf = self.atom(value, prefix, source_path)
        return leaf.get("$value") if leaf else None

    def scalar(self, value: Any, domain: str, source_path: str) -> None:
        leaf = self.atom(value, domain, source_path)
        if not leaf:
            return
        number = leaf.get("$value")
        if domain == "scale" and (not isinstance(number, (int, float)) or number <= 0):
            self.add("error", "MOTION-SCALE-DOMAIN", source_path, "Scale 必须是大于 0 的无量纲数。")
        if domain == "rotation" and leaf.get("unit") != "deg":
            self.add("error", "MOTION-ROTATION-UNIT", source_path, "Rotation 必须使用 deg。")
        if domain == "opacity" and (
            not isinstance(number, (int, float)) or not 0 <= number <= 1
        ):
            self.add("error", "MOTION-OPACITY-DOMAIN", source_path, "Opacity 必须位于 0..1。")

    def track_value(self, prop: str, value: Any, space: str, source_path: str) -> None:
        if prop == "position":
            if space not in self.registry["positionSpaces"]:
                self.add("error", "MOTION-POSITION-SPACE", source_path,
                         f"Position space 未登记：{space!r}。")
            if not isinstance(value, dict) or set(value) != {"x", "y"}:
                self.add("error", "MOTION-POSITION-VALUE", source_path,
                         "Position 必须同时提供 x/y。")
                return
            for axis in ("x", "y"):
                self.atom(value[axis], "distance", f"{source_path}/{axis}")
            return
        if prop in ("scale", "rotation", "opacity"):
            self.scalar(value, prop, source_path)
            return
        if prop not in self.registry["extendedProperties"]:
            self.add("error", "MOTION-PROPERTY-UNKNOWN", source_path,
                     f"Motion property 未登记：{prop!r}。")

    def timing(self, timing: Any, source_path: str) -> None:
        if not isinstance(timing, dict):
            self.add("error", "MOTION-TIMING-MISSING", source_path, "Track Composite 缺少 timing。")
            return
        self.atom(timing.get("duration"), "duration", f"{source_path}/duration")
        self.atom(timing.get("delay"), "delay", f"{source_path}/delay")
        self.atom(timing.get("easing"), "easing", f"{source_path}/easing")

    def repeat(self, repeat: Any, source_path: str) -> None:
        if not isinstance(repeat, dict):
            self.add("error", "MOTION-REPEAT-MISSING", source_path, "Motion 缺少 repeat。")
            return
        count = repeat.get("count")
        if count != "infinite" and (not isinstance(count, int) or isinstance(count, bool) or count < 1):
            self.add("error", "MOTION-REPEAT-COUNT", f"{source_path}/count",
                     "repeat.count 必须为正整数或 infinite。")
        if repeat.get("direction") not in self.registry["repeatDirections"]:
            self.add("error", "MOTION-REPEAT-DIRECTION", f"{source_path}/direction",
                     "repeat.direction 未登记。")

    def validate_track_entry(self, identity: str, entry: dict) -> None:
        base = "/tokens/" + identity.replace(".", "/")
        tracks = entry.get("tracks")
        if not isinstance(tracks, list) or not tracks:
            self.add("error", "MOTION-TRACKS-MISSING", f"{base}/tracks", "tracks 必须是非空数组。")
            return
        for index, track in enumerate(tracks):
            path = f"{base}/tracks/{index}"
            if not isinstance(track, dict):
                self.add("error", "MOTION-TRACK-INVALID", path, "Track 必须是对象。")
                continue
            prop = track.get("property", "")
            if prop not in self.registry["coreProperties"] + self.registry["extendedProperties"]:
                self.add("error", "MOTION-PROPERTY-UNKNOWN", f"{path}/property",
                         f"Motion property 未登记：{prop!r}。")
                continue
            if "keyframes" in track:
                frames = track["keyframes"]
                if not isinstance(frames, list) or len(frames) < 2:
                    self.add("error", "MOTION-KEYFRAMES", f"{path}/keyframes",
                             "keyframes 至少需要 2 帧。")
                    continue
                positions = [frame.get("at") for frame in frames if isinstance(frame, dict)]
                if len(positions) != len(frames) or any(
                    not isinstance(at, (int, float)) or isinstance(at, bool) or not 0 <= at <= 1
                    for at in positions
                ) or positions != sorted(positions):
                    self.add("error", "MOTION-KEYFRAME-TIME", f"{path}/keyframes",
                             "keyframes.at 必须位于 0..1 且非递减。")
                for frame_index, frame in enumerate(frames):
                    if isinstance(frame, dict):
                        self.track_value(prop, frame.get("value"), track.get("space", ""),
                                         f"{path}/keyframes/{frame_index}/value")
            elif "from" in track and "to" in track:
                self.track_value(prop, track["from"], track.get("space", ""), f"{path}/from")
                self.track_value(prop, track["to"], track.get("space", ""), f"{path}/to")
            else:
                self.add("error", "MOTION-TRACK-FRAMES", path,
                         "Track 必须提供 from/to 或 keyframes。")
        self.timing(entry.get("timing"), f"{base}/timing")
        self.repeat(entry.get("repeat"), f"{base}/repeat")
        stagger = entry.get("stagger")
        if stagger is not None:
            if not isinstance(stagger, dict):
                self.add("error", "MOTION-STAGGER", f"{base}/stagger", "stagger 必须是对象。")
            else:
                self.atom(stagger.get("interval"), "delay", f"{base}/stagger/interval")
                if stagger.get("direction") not in self.registry["staggerDirections"]:
                    self.add("error", "MOTION-STAGGER-DIRECTION", f"{base}/stagger/direction",
                             "stagger.direction 未登记。")

    def validate_sequences(self) -> None:
        graph: dict[str, list[str]] = {}
        for identity, entry in self.entries.items():
            if "sequence" not in entry:
                continue
            sequence = entry["sequence"]
            steps = sequence.get("steps") if isinstance(sequence, dict) else None
            if not isinstance(steps, list) or not steps:
                self.add("error", "MOTION-SEQUENCE-STEPS",
                         "/tokens/" + identity.replace(".", "/") + "/sequence/steps",
                         "sequence.steps 必须是非空数组。")
                continue
            graph[identity] = []
            for index, step in enumerate(steps):
                target = step.get("motion") if isinstance(step, dict) else None
                if target not in self.entries:
                    self.add("error", "MOTION-SEQUENCE-REFERENCE",
                             f"/tokens/{identity.replace('.', '/')}/sequence/steps/{index}",
                             f"Sequence 引用不存在：{target!r}。")
                else:
                    graph[identity].append(target)

        visiting: set[str] = set()
        visited: set[str] = set()

        def visit(identity: str) -> None:
            if identity in visiting:
                self.add("error", "MOTION-SEQUENCE-CYCLE", "/tokens/" + identity.replace(".", "/"),
                         "Motion Sequence 存在循环引用。")
                return
            if identity in visited:
                return
            visiting.add(identity)
            for target in graph.get(identity, []):
                visit(target)
            visiting.remove(identity)
            visited.add(identity)

        for identity in graph:
            visit(identity)

    def validate_hooks(self) -> None:
        package_hooks = self.package.get("hooks", {})
        registry_components = self.component_registry.get("components", {})
        for component in COMPONENTS:
            recipe = load(SOURCE / "component-recipes" / f"{component}.json")
            declared = set(registry_components.get(component, {}).get("motionHooks", []))
            bindings = recipe.get("motion", {})
            if not isinstance(bindings, dict):
                self.add("error", "MOTION-HOOKS-INVALID", f"/hooks/{component}",
                         f"{component} motion 必须是对象。")
                continue
            expected = package_hooks.get(component, {})
            for hook, reference in bindings.items():
                target = reference.removeprefix("{motion.").removesuffix("}") \
                    if isinstance(reference, str) else ""
                if hook not in declared:
                    self.add("error", "MOTION-HOOK-NOT-DECLARED", f"/hooks/{component}/{hook}",
                             f"{component}.{hook} 未在 Component Registry 登记。")
                if target not in self.entries:
                    self.add("error", "MOTION-HOOK-REFERENCE", f"/hooks/{component}/{hook}",
                             f"Motion Hook 引用不存在：{reference!r}。")
                if expected.get(hook) != target:
                    self.add("error", "MOTION-HOOK-MAP-DRIFT", f"/hooks/{component}/{hook}",
                             "Recipe 与 motion.json hooks 映射不一致。")

    def validate(self) -> dict:
        if self.package.get("kind") != "motion" or self.registry.get("package") != "motion.json":
            self.add("error", "MOTION-REGISTRY-PACKAGE", "/", "Motion Registry 与 package 不一致。")
        for identity, entry in self.entries.items():
            expected_status = "active" if identity in self.released else "candidate"
            if entry.get("$status") != expected_status:
                self.add("error", "MOTION-STATUS", "/tokens/" + identity.replace(".", "/"),
                         f"Motion slice {identity} 必须为 {expected_status}。")
            if "tracks" in entry:
                self.validate_track_entry(identity, entry)
        self.validate_sequences()
        self.validate_hooks()
        missing = sorted(self.released - set(self.entries))
        for identity in missing:
            self.add("error", "MOTION-RELEASE-SLICE-MISSING", "/policy/releasedSlices",
                     f"已发布 Motion slice 不存在：{identity}。")
        for identity in sorted(self.released & set(self.entries)):
            entry = self.entries[identity]
            for alias in self._aliases(entry):
                leaf = self.atoms.get(alias)
                if leaf and leaf.get("$status") != "active":
                    self.add("error", "MOTION-RELEASE-DEPENDENCY-CANDIDATE",
                             "/tokens/" + identity.replace(".", "/"),
                             f"已发布 slice 依赖未发布 Primitive：{alias}。")
        blocked = sorted(set(self.entries) - self.released)
        if blocked:
            self.add("warning", "MOTION-CANDIDATE-SLICES-BLOCKED", "/$status",
                     "未评审 Motion slices 不生成平台产物；已发布 slices 不受阻断。")
        errors = sum(item["severity"] == "error" for item in self.diagnostics)
        warnings = sum(item["severity"] == "warning" for item in self.diagnostics)
        return {
            "phase": self.report_phase,
            "motionStatus": self.package.get("$status"),
            # Slice-scoped release: candidate slices remain unavailable, but
            # they must not block an already reviewed runtime slice such as
            # loading.spinner from being published and consumed.
            "releaseBlocked": bool(errors or not self.released),
            "releasedSlices": sorted(self.released),
            "blockedSlices": blocked,
            "platformOutputsGenerated": not errors and bool(self.released),
            "compositeCount": len(self.entries),
            "validatedComponents": list(COMPONENTS),
            "summary": {"errors": errors, "warnings": warnings},
            "diagnostics": self.diagnostics,
        }

    def _aliases(self, value: Any) -> set[str]:
        result: set[str] = set()
        if isinstance(value, dict):
            for child in value.values():
                result.update(self._aliases(child))
        elif isinstance(value, list):
            for child in value:
                result.update(self._aliases(child))
        elif isinstance(value, str):
            match = ALIAS.fullmatch(value)
            if match:
                result.add(match.group(1))
        return result

    def platform_payload(self, platform: str) -> dict:
        motion: dict[str, Any] = {}
        for identity in sorted(self.released):
            entry = self.entries.get(identity)
            if not entry or "tracks" not in entry:
                continue
            tracks = []
            for index, track in enumerate(entry["tracks"]):
                prop = track["property"]
                base = f"/tokens/{identity.replace('.', '/')}/tracks/{index}"
                domain = prop if prop in ("scale", "rotation", "opacity") else ""
                resolved = {"property": prop}
                if "from" in track:
                    resolved["from"] = self.resolved_atom(track["from"], domain, base + "/from")
                    resolved["to"] = self.resolved_atom(track["to"], domain, base + "/to")
                    leaf = self.atom(track["to"], domain, base + "/to")
                    if leaf and leaf.get("unit"):
                        resolved["unit"] = leaf["unit"]
                tracks.append(resolved)
            timing = entry["timing"]
            metadata = entry.get("metadata", {})
            motion[identity] = {
                "status": "active",
                "tracks": tracks,
                "timing": {
                    "durationMs": self.resolved_atom(timing["duration"], "duration", "/timing/duration"),
                    "delayMs": self.resolved_atom(timing["delay"], "delay", "/timing/delay"),
                    "easing": self.resolved_atom(timing["easing"], "easing", "/timing/easing"),
                },
                "repeat": entry["repeat"],
                "activation": metadata.get("activation"),
                "renderer": metadata.get("renderer"),
            }
        return {
            "$description": f"Generated {platform} Motion slices from reviewed V2 sources. Do not hand-edit.",
            "$meta": {
                "packageStatus": self.package.get("$status"),
                "releaseUnit": self.package.get("policy", {}).get("releaseScope"),
                "releasedSlices": sorted(self.released),
                "candidateSlicesBlocked": True,
            },
            "motion": motion,
        }


def main(argv: list[str] | None = None) -> int:
    from .output_guard import require_build_workspace
    require_build_workspace(ROOT)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("local", "ci", "release"), default="local")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    validator = MotionValidator(args.phase)
    report = validator.validate()
    payload = (json.dumps(report, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    outputs = {
        COMPOSITE_OUTPUT / "qt.json": (json.dumps(validator.platform_payload("Qt"), ensure_ascii=False, indent=2) + "\n").encode("utf-8"),
        COMPOSITE_OUTPUT / "vue.json": (json.dumps(validator.platform_payload("Vue"), ensure_ascii=False, indent=2) + "\n").encode("utf-8"),
    }
    if args.check:
        if not REPORT.exists() or REPORT.read_bytes() != payload:
            print("Composite diagnostic drift")
            return 1
        for path, content in outputs.items():
            if not path.exists() or path.read_bytes() != content:
                print(f"Composite platform output drift: {path}")
                return 1
    else:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        if not REPORT.exists() or REPORT.read_bytes() != payload:
            REPORT.write_bytes(payload)
        COMPOSITE_OUTPUT.mkdir(parents=True, exist_ok=True)
        for path, content in outputs.items():
            if not path.exists() or path.read_bytes() != content:
                path.write_bytes(content)
    summary = report["summary"]
    print(f"Motion Composite {'check' if args.check else 'validation'} | "
          f"composites={report['compositeCount']} | errors={summary['errors']} | "
          f"warnings={summary['warnings']} | releaseBlocked={report['releaseBlocked']}")
    return 1 if summary["errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
