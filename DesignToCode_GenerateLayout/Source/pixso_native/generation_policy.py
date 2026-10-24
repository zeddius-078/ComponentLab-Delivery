"""Explicit generation policy; warnings never certify design equivalence."""
from component_engine import ComponentError


def export_first(mapping):
    policy = mapping.get("generationPolicy", "strict")
    if policy not in ("strict", "export-first"):
        raise ComponentError("UNKNOWN_GENERATION_POLICY")
    return policy == "export-first"


def skipped_props(rule, props):
    """Return exact unsupported/missing properties, without guessing defaults."""
    definitions = rule.get("props", {})
    result = set(props) - set(definitions)
    for key, spec in definitions.items():
        if key not in props and "default" not in spec:
            result.add(key)
            continue
        value = props.get(key, spec.get("default"))
        choices = spec.get("cases", spec.get("values"))
        if choices is not None and (not isinstance(value, str) or value not in choices):
            result.add(key)
    return sorted(result)
