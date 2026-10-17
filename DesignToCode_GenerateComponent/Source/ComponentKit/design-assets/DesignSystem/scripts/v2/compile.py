"""ComponentLab V2 token compiler CLI.

Source (DesignSystem/source, canonical) -> context resolution -> typed values
-> platform adapters. Produces the Qt runtime envelope, CSS, DTCG exports, the
legacy sToken compatibility export, the token catalog and the mapping report.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from pathlib import Path

from . import adapters
from .model import TokenModel, ValidationError, load_json
from .resolver import Resolver

V2_VERSION = "2.0.0"
ROOT = Path(__file__).resolve().parents[3]  # design-assets
SOURCE_DIR = ROOT / "DesignSystem/source"
CONFIG_DIR = ROOT / "DesignSystem/config"
GENERATED = ROOT / "DesignSystem/generated"
ARCHIVE_DIR = ROOT / "DesignSystem/archive/commerly"

CONTEXT_NAMES = ["standard-light", "standard-dark", "wide-light", "wide-dark"]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    model = TokenModel(root / "DesignSystem/source")

    contexts = {}
    for profile_mode, theme_mode in (("standard", "light"), ("standard", "dark"),
                                     ("wide", "light"), ("wide", "dark")):
        name = f"{profile_mode}-{theme_mode}"
        records = model.context(profile_mode, theme_mode)
        contexts[name] = Resolver(model, records).resolve_all()

    legacy_map = load_json(root / "DesignSystem/config/legacy-map.json")
    decisions = legacy_map.get("decisions", [])
    pending = legacy_map.get("pendingConflicts", [])

    profiles, commerly_meta, conflicts = adapters.qt_runtime.build_legacy_profiles(
        model, contexts, decisions, root / "DesignSystem/archive/commerly")

    # default runtime profile groups (top level envelope)
    default_groups = profiles["light-comfortable"]

    stoken_snapshot = root / "DesignSystem/imports/stoken.json"
    design_sha = hashlib.sha256(stoken_snapshot.read_bytes()).hexdigest()

    runtime = {
        "$description": "Generated Qt runtime token adapter. Do not hand-edit.",
        "$meta": {
            "source": {
                "reference": "ComponentLab V2 canonical token source",
                "canonicalDir": "DesignSystem/source",
            },
            "commonSourceRevision": model.source_revision()[:12],
            "platformSourceRevision": hashlib.sha256(
                Path(__file__).read_bytes()).hexdigest()[:12],
            "defaultProfile": "light-comfortable",
            "axisOrder": ["theme", "density"],
            "axes": {
                "theme": {"default": "light", "values": ["light", "dark"],
                          "description": "Design theme mode from the semantic layer."},
                "density": {"default": "comfortable", "values": ["comfortable", "compact"],
                            "description": "Legacy runtime axis. Recorded separately; "
                                           "NOT equivalent to the design profile axis (standard/wide)."},
            },
            "v2": {
                "schemaVersion": "2.0",
                "tokenSetVersion": V2_VERSION,
                "contexts": CONTEXT_NAMES,
                "contextMap": {
                    "light-comfortable": "standard-light",
                    "light-compact": "standard-light",
                    "dark-comfortable": "standard-dark",
                    "dark-compact": "standard-dark",
                },
                "wideRuntimeConsumer": "none-registered",
                "darkPublishable": False,
                "legacyPolicy": {
                    "direction": "one-way V2 -> legacy names",
                    "exitConditions": [
                        "runtime C++ components migrate from base.stoken.* and legacy role keys",
                        "contracts and compilers consume canonical paths only",
                        "legacy density axis replaced or removed",
                    ],
                    "owner": "ComponentKit",
                },
            },
            "stoken": {
                "snapshotFile": "DesignSystem/imports/stoken.json",
                "sha256": design_sha,
                "role": "frozen design evidence; not a write target",
            },
        },
    }
    runtime.update(default_groups)
    runtime["profiles"] = profiles
    runtime["contexts"] = {
        name: _context_groups(contexts[name]) for name in CONTEXT_NAMES
    }

    outputs: dict[Path, bytes] = {}

    def register(path: Path, text: str) -> None:
        outputs[path] = text.encode("utf-8")

    register(GENERATED / "runtime/tokens_qt.json",
             json.dumps(runtime, ensure_ascii=False, indent=2) + "\n")
    register(GENERATED / "css/tokens.css", adapters.css.build(contexts))
    for name in CONTEXT_NAMES:
        register(GENERATED / "dtcg" / f"{name}.tokens.json",
                 adapters.dtcg.build(name, contexts[name]))
    register(GENERATED / "compat/stoken.json",
             adapters.compat_stoken.build(model, contexts["standard-light"]))
    register(GENERATED / "reports/token_catalog.csv",
             adapters.catalog.build(model, contexts))

    resolved_revision = hashlib.sha256(json.dumps(
        {"profiles": profiles, "contexts": {name: _context_groups(contexts[name])
                                            for name in CONTEXT_NAMES}},
        sort_keys=True, ensure_ascii=False).encode("utf-8")).hexdigest()[:12]

    report = {
        "$description": "V2 mapping and conflict report. Machine-checked inputs for the acceptance review.",
        "schemaVersion": "2.0",
        "sourceHash": model.source_revision(),
        "resolvedSourceRevision": resolved_revision,
        "designSnapshotSha256": design_sha,
        "pendingConflicts": pending,
        "legacyCanonicalConflicts": conflicts,
        "privateMaterialCount": len(legacy_map.get("privateMaterial", [])),
        "decisionCount": len(decisions),
        "canonicalTokenCounts": {
            layer: len(model.layers[layer]) for layer in ("primitive", "profile", "semantic", "component")
        },
    }
    register(GENERATED / "reports/v2-mapping-report.json",
             json.dumps(report, ensure_ascii=False, indent=2) + "\n")

    manifest = {
        "schemaVersion": "2.0",
        "tokenSetVersion": V2_VERSION,
        "sourceHash": model.source_revision(),
        "resolvedSourceRevision": resolved_revision,
        "contexts": CONTEXT_NAMES,
        "generator": {
            "name": "componentlab-token-compiler",
            "version": V2_VERSION,
            "entry": "design-assets/tools/export_tokens.py",
        },
        "designSnapshot": {
            "figmaFileKey": "w92BKW1EcutKmTXkwzNHbf",
            "file": "DesignSystem/imports/stoken.json",
            "sha256": design_sha,
            "collections": ["01.primitive", "02.profile", "03.semantic", "04.component"],
            "note": "Collection/mode counts reflect the frozen export; Figma node rebinding to V2 collections is tracked in the rebind plan.",
        },
        "darkPublishable": False,
        "legacyCompat": {
            "direction": "one-way",
            "exitConditions": runtime["$meta"]["v2"]["legacyPolicy"]["exitConditions"],
        },
    }
    register(CONFIG_DIR / "manifest.json",
             json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")

    if args.check:
        failures = []
        for path, data in outputs.items():
            if not path.exists() or path.read_bytes() != data:
                failures.append(str(path.relative_to(root)))
        if failures:
            raise ValidationError("Generated drift: " + ", ".join(failures))
        print("V2 check OK | contexts=4 | tokens=%d | resolved=%s" %
              (len(contexts["standard-light"]), resolved_revision))
        return 0

    for path, data in outputs.items():
        if path.exists() and path.read_bytes() == data:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    print(f"V2 generated | contexts=4 | canonical tokens={len(contexts['standard-light'])} | "
          f"resolved={resolved_revision} | sha256={hashlib.sha256(runtime_bytes(outputs[GENERATED / 'runtime/tokens_qt.json'])).hexdigest()[:16]}")
    return 0


def runtime_bytes(data: bytes) -> bytes:
    return data


def _context_groups(resolved) -> dict:
    from .values import group_for, qt_value
    groups: dict = {}
    for path, record in sorted(resolved.items()):
        groups.setdefault(group_for(record.type), {})[path] = qt_value(record)
    return groups


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValidationError as exc:
        print(f"V2 validation error: {exc}")
        sys.exit(1)
