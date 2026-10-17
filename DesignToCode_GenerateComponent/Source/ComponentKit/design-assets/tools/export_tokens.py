"""ComponentLab V2 token compiler entry point.

Compiles the canonical V2 source (DesignSystem/source) into platform artifacts:
Qt runtime envelope, CSS, DTCG exports, legacy sToken compatibility export,
token catalog and mapping report.

Usage: python -B design-assets/tools/export_tokens.py [--root DESIGN_ASSETS] [--check]
"""
from __future__ import annotations

import sys
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]
SCRIPTS = PACKAGE / "DesignSystem/scripts"
sys.path.insert(0, str(SCRIPTS))

from v2.compile import main  # noqa: E402


def run() -> int:
    argv = sys.argv[1:]
    if "--root" not in argv:
        argv = ["--root", str(PACKAGE), *argv]
    return main(argv)


if __name__ == "__main__":
    sys.exit(run())
