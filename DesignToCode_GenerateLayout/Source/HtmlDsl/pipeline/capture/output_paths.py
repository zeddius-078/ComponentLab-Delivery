"""Prevent capture/verification commands from writing into maintained project files."""
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[4]


def output_path(value):
    path = Path(value).absolute()
    # Resolve existing parents as well as the target: junctions must not bypass the boundary.
    resolved = path.resolve()
    for candidate in (path, resolved):
        if candidate.is_relative_to(PROJECT) and not candidate.is_relative_to(PROJECT / "build"):
            raise ValueError("OUTPUT_MUST_BE_IN_BUILD")
    for parent in (path, *path.parents):
        if parent.is_symlink() or getattr(parent, "is_junction", lambda: False)():
            raise ValueError("OUTPUT_PATH_IS_LINK")
    return resolved
