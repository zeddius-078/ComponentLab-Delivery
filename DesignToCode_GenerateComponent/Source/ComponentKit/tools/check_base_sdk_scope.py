"""Validate the frozen public boundary of the Qt-native baseline SDK.

This check deliberately reads maintained inputs only.  It must pass before a
baseline package can be assembled, but it is not evidence that the package has
already been built or published.
"""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFINITIONS = ROOT / "component-definitions" / "native-components.json"
SCOPE = ROOT / "component-definitions" / "base-sdk-scope.json"
def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def validate():
    scope = read_json(SCOPE)
    native = read_json(DEFINITIONS)["components"]
    assert scope["status"] == "baseline-native"
    assert scope["nativeRegistry"] == "native-components.json"
    approved = scope["approvedQtClasses"]
    assert len(approved) == scope["validation"]["expectedNativeComponentCount"]
    assert len(set(approved)) == len(approved)
    assert {item["qtClass"] for item in native} == set(approved)
    assert len(native) == len(approved)

    approved_sdk_classes = set()
    for item in native:
        qt_class = item["qtClass"]
        sdk_class = item["sdkClass"]
        assert qt_class.startswith("Q"), item
        assert sdk_class == "Z_" + qt_class[1:], item
        approved_sdk_classes.add("commonWidget::" + sdk_class)

    deferred = scope["deferredComponentIds"]
    assert len(deferred) == len(set(deferred))
    assert not (set(deferred) & approved_sdk_classes)
    # This guard intentionally does not read the historical runtime mapping.
    # A baseline release must be classified from its frozen native registry and
    # scope alone; otherwise a legacy Factory/mapping edit could reintroduce a
    # reverse dependency into the portable migration package.

    return {
        "status": "PASS",
        "baselineStatus": scope["status"],
        "baselineNativeComponents": len(approved),
        "deferredComponents": len(deferred),
    }


if __name__ == "__main__":
    print(json.dumps(validate(), ensure_ascii=False))
