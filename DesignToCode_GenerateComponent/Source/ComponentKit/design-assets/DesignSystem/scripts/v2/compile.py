"""ComponentLab V2 token compiler CLI.

Source (DesignSystem/source, canonical) -> context resolution -> typed values
-> platform adapters. Produces the Qt runtime envelope, CSS, DTCG exports, the
token catalog and the mapping report. No legacy import, no legacy names and no
compatibility views remain in the pipeline.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

from . import adapters
from .model import TokenModel, ValidationError
from .resolver import Resolver

V2_VERSION = "2.0.0"
ROOT = Path(__file__).resolve().parents[3]  # design-assets
SOURCE_DIR = ROOT / "DesignSystem/source"
CONFIG_DIR = ROOT / "DesignSystem/config"
GENERATED = ROOT / "DesignSystem/generated"

CONTEXT_NAMES = ["standard-light", "standard-dark", "wide-light", "wide-dark"]


def main(argv: list[str] | None = None) -> int:
    from .output_guard import require_build_workspace
    require_build_workspace(ROOT)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    # Composite validation runs before Component materialization. Candidate
    # Motion is validated and reported, but intentionally not published to a
    # platform runtime until its design review changes the package to active.
    from . import composite_compile
    composite_args = ["--phase", "ci" if args.check else "local"]
    if args.check:
        composite_args.append("--check")
    composite_status = composite_compile.main(composite_args)
    if composite_status != 0:
        raise ValidationError("Composite pipeline failed "
                              f"(exit {composite_status})")

    # Component layer pipeline first: Registry-Recipe validation + generation.
    # Same rules in local (write), ci (--check) and release (publish checks).
    from . import component_compile
    component_args = ["--phase", "ci" if args.check else "local"]
    if args.check:
        component_args.append("--check")
    component_status = component_compile.main(component_args)
    if component_status != 0:
        raise ValidationError("Component Registry-Recipe pipeline failed "
                              f"(exit {component_status})")
    from . import typography_compile
    typography_args = ["--phase", "ci" if args.check else "local"]
    if args.check:
        typography_args.append("--check")
    typography_status = typography_compile.main(typography_args)
    if typography_status != 0:
        raise ValidationError("Typography Composite pipeline failed "
                              f"(exit {typography_status})")
    model = TokenModel(root / "DesignSystem/source")

    contexts = {}
    for profile_mode, theme_mode in (("standard", "light"), ("standard", "dark"),
                                     ("wide", "light"), ("wide", "dark")):
        name = f"{profile_mode}-{theme_mode}"
        records = model.context(profile_mode, theme_mode)
        contexts[name] = Resolver(model, records).resolve_all()

    runtime = adapters.qt_runtime.build(model)

    outputs: dict[Path, bytes] = {}

    def register(path: Path, text: str) -> None:
        outputs[path] = text.encode("utf-8")

    register(GENERATED / "runtime/tokens_qt.json",
             json.dumps(runtime, ensure_ascii=False, indent=2) + "\n")
    register(GENERATED / "css/tokens.css", adapters.css.build(contexts))
    for name in CONTEXT_NAMES:
        register(GENERATED / "dtcg" / f"{name}.tokens.json",
                 adapters.dtcg.build(name, contexts[name]))
    register(GENERATED / "reports/token_catalog.csv",
             adapters.catalog.build(model, contexts))

    resolved_revision = hashlib.sha256(json.dumps(
        {"profiles": runtime["profiles"]}, sort_keys=True,
        ensure_ascii=False).encode("utf-8")).hexdigest()[:12]

    report = {
        "$description": "V2 mapping and conflict report. Machine-checked inputs for the acceptance review.",
        "schemaVersion": "2.0",
        "sourceHash": model.source_revision(),
        "resolvedSourceRevision": resolved_revision,
        "canonicalTokenCounts": {
            layer: len(model.layers[layer])
            for layer in ("primitive", "profile", "semantic", "component")
        },
        "productTokenCount": len(model.product),
    }
    register(GENERATED / "reports/v2-mapping-report.json",
             json.dumps(report, ensure_ascii=False, indent=2) + "\n")

    # The ComponentLab map is a V2 product, not the legacy audit snapshot.
    # Generate it from binding manifests and recipes in the same deterministic
    # pass so SDK publication cannot silently reintroduce old token names.
    from .build_runtime_map import build_map
    register(CONFIG_DIR / "figma-component-map-v2.json",
             json.dumps(build_map(root), ensure_ascii=False, indent=2) + "\n")

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
            "note": "The V2 canonical source is the single source of truth; the legacy sToken "
                    "snapshot and importer middleware have been removed from the repository. "
                    "Figma rebinding to V2 collections is tracked in the rebind plan.",
        },
        "darkPublishable": False,
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
          f"resolved={resolved_revision} | "
          f"sha256={hashlib.sha256(outputs[GENERATED / 'runtime/tokens_qt.json']).hexdigest()[:16]}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValidationError as exc:
        print(f"V2 validation error: {exc}")
        sys.exit(1)
