"""Prevent capture/verification commands from writing into maintained project files."""
import os
from pathlib import Path
import stat

PROJECT = Path(__file__).resolve().parents[4]

def reject_links(path):
    for parent in (path, *path.parents):
        if parent.is_symlink():
            raise ValueError("OUTPUT_PATH_IS_LINK")
        # Python 3.9 has no Path.is_junction(); use Windows reparse attributes.
        if parent.exists() and getattr(parent.lstat(), "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400):
            raise ValueError("OUTPUT_PATH_IS_LINK")

def output_path(value):
    path = Path(os.path.abspath(value))
    reject_links(path)
    # Resolve existing parents as well as the target: junctions must not bypass the boundary.
    resolved = path.resolve()
    tool, work = os.environ.get("D2C_RUNTIME_ROOT"), os.environ.get("D2C_WORK_ROOT")
    if tool or work:
        if not tool or not work:
            raise ValueError("OFFLINE_ROOTS_REQUIRED")
        for root in (Path(tool), Path(work)):
            if not root.is_absolute() or not root.is_dir():
                raise ValueError("OFFLINE_ROOT_INVALID")
            reject_links(root)
        tool, work = Path(tool).resolve(), Path(work).resolve()
        if work.is_relative_to(tool):
            raise ValueError("WORK_ROOT_INSIDE_RUNTIME")
        allowed = ("work", "logs", "delivery")
        if resolved.is_relative_to(tool) or not any(resolved.is_relative_to(work / "output" / name) for name in allowed):
            raise ValueError("OUTPUT_MUST_BE_IN_WORK_BUILD")
        return resolved
    for candidate in (path, resolved):
        if not any(candidate.is_relative_to(PROJECT / "output" / name) for name in ("work", "logs", "delivery")):
            raise ValueError("OUTPUT_MUST_BE_IN_WORK_BUILD")
    return resolved
