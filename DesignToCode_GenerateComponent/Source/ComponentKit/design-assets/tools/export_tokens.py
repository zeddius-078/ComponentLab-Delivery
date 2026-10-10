"""Export normalized sToken through the canonical Alias/Profile resolver."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys
import stoken_import

PACKAGE = Path(__file__).resolve().parents[1]
PARSER_DIR = PACKAGE / "DesignSystem" / "scripts"
sys.path.insert(0, str(PARSER_DIR))
spec = importlib.util.spec_from_file_location("canonical_generator", PARSER_DIR / "generate.py")
canonical = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(canonical)


def configure(root: Path) -> None:
    canonical.ROOT = root
    canonical.DESIGN_SYSTEM_DIR = root / "DesignSystem"
    canonical.SOURCE_DIR = canonical.DESIGN_SYSTEM_DIR / "source"
    for attribute, name in (
        ("PRIMITIVES_PATH", "primitives.json"), ("PROFILES_PATH", "profiles.json"),
        ("WIDGETS_PATH", "widgets.json"), ("COMPONENT_LIBRARY_PATH", "component_library.json"),
        ("UI_LAYOUT_BINDINGS_PATH", "ui_layout_bindings.json"),
        ("QSS_CAPABILITIES_PATH", "qt_qss_capabilities.json"),
    ):
        setattr(canonical, attribute, canonical.SOURCE_DIR / name)
    canonical.QT_TOKENS_PATH = root / "DesignSystem/generated/runtime/tokens_qt.json"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=PACKAGE)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    configure(args.root.resolve())
    raw = canonical.load_json(canonical.PRIMITIVES_PATH)
    raw, profile_config, import_config, import_meta = stoken_import.prepare(
        args.root, raw, canonical.load_json(canonical.PROFILES_PATH))
    first_profiles, _ = canonical.build_profiles(raw, profile_config)
    stoken_import.adjust(raw, first_profiles, import_config, canonical.collect_token_types(raw))
    profiles, metadata = canonical.build_profiles(raw, profile_config)
    types = canonical.collect_token_types(raw)
    stoken_import.finish(args.root, profiles, types, import_config, import_meta, args.check)
    default = profiles[metadata["defaultProfile"]]
    resolved_revision = hashlib.sha256(json.dumps(
        {"profiles": profiles, "types": types, "stoken": import_meta["sha256"],
         "mapping": import_meta["mappingSha256"]}, sort_keys=True,
        ensure_ascii=False).encode("utf-8")).hexdigest()[:12]
    # Mirror the canonical export envelope and call its grouping implementation.
    # The build-time adaptation changes output placement, never Alias or Profile evaluation.
    output = {
        "$description": "Generated Qt runtime with normalized sToken input. Do not hand-edit.",
        "$meta": {
            "source": metadata.get("source", {}),
            "commonSourceRevision": canonical.source_revision(),
            "upstreamCommonSourceRevision": canonical.source_revision(),
            "resolvedSourceRevision": resolved_revision,
            "platformSourceRevision": canonical.platform_source_revision(),
            "defaultProfile": metadata["defaultProfile"],
            "axisOrder": metadata["axisOrder"], "axes": metadata["axes"],
            "stoken": {k: v for k, v in import_meta.items() if k != "origins"},
            "sourcePolicy": "Canonical resolver with normalized sToken mappings.",
        },
        **canonical._group_qt_tokens(default, types),
        "profiles": {name: canonical._group_qt_tokens(values, types)
                     for name, values in profiles.items()},
    }
    data = (json.dumps(output, ensure_ascii=False, indent=2) + "\n").replace("\n", os.linesep).encode("utf-8")
    target = canonical.QT_TOKENS_PATH
    if args.check:
        if not target.exists() or target.read_bytes() != data:
            raise ValueError(f"Canonical runtime drift: {target}")
    elif not target.exists() or target.read_bytes() != data:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    print(f"Canonical profiles={len(profiles)} tokens={len(default)} "
          f"upstream={canonical.source_revision()} resolved={resolved_revision} "
          f"sha256={hashlib.sha256(data).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
