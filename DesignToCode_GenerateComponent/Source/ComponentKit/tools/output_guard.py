"""Generation is allowed only in the disposable prepared output/work workspace."""
from pathlib import Path


def require_build_workspace(path):
    value = Path(path).resolve()
    for ancestor in (value, *value.parents):
        if (ancestor / "Source/Publish-ComponentSDK.ps1").is_file():
            if value.is_relative_to(ancestor / "output/work"):
                return
            break
    raise RuntimeError("READ_ONLY_SOURCE: run Source/Publish-ComponentSDK.ps1 or python -B Source/PrepareBuild.py")
