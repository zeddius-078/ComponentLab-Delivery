"""Compare resolved source slots with real page instances, not SDK probe defaults.

This validation module does not generate page code. Unapplied attributes and
visual/source discrepancies remain warnings outside its deliberately narrow scope.
"""


def compare_content(bindings, snapshot):
    actual = {}
    for item in snapshot["objects"]:
        if item["name"] in actual:
            raise ValueError("DUPLICATE_RUNTIME_OBJECT")
        actual[item["name"]] = item
    checks, failures = [], []
    known_slots = {"profileName", "searchPlaceholder", "routeLabels", "primaryItems",
                   "secondaryItems", "primaryIconSources", "currentIndex", "secondaryIndex",
                   "sampleData", "leadingIcon", "accessibleName"}
    seen = set()
    for binding in bindings:
        name = binding["objectName"]
        if name in seen:
            raise ValueError("DUPLICATE_COMPONENT_BINDING")
        seen.add(name)
        if name not in actual or actual[name]["class"] != binding["qtClass"]:
            failures.append({"objectName": name, "field": "class", "expected": binding["qtClass"],
                             "actual": actual.get(name, {}).get("class")})
            continue
        expected = dict(binding["boundSlots"])
        unknown = set(expected) - known_slots
        if unknown:
            raise ValueError("UNSUPPORTED_CONTENT_READBACK: " + ", ".join(sorted(unknown)))
        if "leadingIcon" in expected:
            # Icon non-null is enforced by generated C++ before the public setter.
            # No private SDK icon storage is queried here.
            del expected["leadingIcon"]
        props = binding["declaredProps"]
        skipped = {item["name"] for item in binding.get("unappliedProperties", [])}
        for source, target in (("Density", "density"), ("IconPlacement", "iconPlacement")):
            if source in props and source not in skipped:
                expected[target] = props[source]
        if "Collapsed" in props and "Collapsed" not in skipped:
            if props["Collapsed"] not in ("False", "True"):
                raise ValueError("UNSUPPORTED_COLLAPSED_READBACK")
            expected["collapsed"] = props["Collapsed"] == "True"
        if binding["qtClass"] == "commonWidget::Z_PushButton" and "State" not in skipped:
            state = props.get("State")
            if state in ("default", "disabled", "loading"):
                expected["enabled"] = state != "disabled"
                expected["visualState"] = state
        if props.get("Content") == "icon-only" and "Content" not in skipped:
            expected["iconOnly"] = True
            expected["text"] = ""
        if "sampleData" in expected:
            expected["modelEditable"] = False
        for key, value in expected.items():
            observed = actual[name]["properties"].get(key)
            check = {"objectName": name, "source": binding["source"], "field": key,
                     "expected": value, "actual": observed, "pass": observed == value}
            checks.append(check)
            if not check["pass"]:
                failures.append(check)
    if not bindings:
        raise ValueError("COMPONENT_BINDINGS_REQUIRED")
    return {"status": "FAIL" if failures else "PASS", "componentCount": len(bindings),
            "checks": checks, "failures": failures,
            "scope": "mapped slots and supported initial states only; unconsumed content remains reported"}
