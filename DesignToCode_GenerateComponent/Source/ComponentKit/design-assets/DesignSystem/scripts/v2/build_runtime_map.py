"""Build the ComponentLab runtime map from V2 binding manifests."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

COMPONENTS = ("button", "checkbox", "radio", "table", "select", "input", "menu", "tree")


def _walk_refs(value: Any, path: tuple[str, ...] = ()) -> list[tuple[str, str]]:
    found: list[tuple[str, str]] = []
    if isinstance(value, dict):
        for key, child in value.items():
            found.extend(_walk_refs(child, path + (str(key),)))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            found.extend(_walk_refs(child, path + (str(index),)))
    elif isinstance(value, str) and value.startswith("{") and value.endswith("}"):
        token = value[1:-1].strip()
        if token:
            found.append(("/".join(path), token))
    return found


def build_map(root: Path) -> dict[str, Any]:
    source = root / "DesignSystem/source"
    # The runtime compiler emits profile-resolved values under
    # generated/runtime/tokens_qt.json.  The V2 map is a presentation and
    # diagnostics surface, so it must show the resolved value instead of
    # leaving canonicalValue blank while the SDK itself already has the value.
    resolved_values: dict[str, Any] = {}
    runtime_path = root / "DesignSystem/generated/runtime/tokens_qt.json"
    if runtime_path.exists():
        try:
            runtime = json.loads(runtime_path.read_text(encoding="utf-8"))
            profile = runtime.get("profiles", {}).get("standard-light", {})
            for value_type, values in profile.items():
                if not isinstance(values, dict):
                    continue
                def collect(node: Any, path: str = "") -> None:
                    if isinstance(node, dict):
                        for key, child in node.items():
                            collect(child, f"{path}.{key}" if path else str(key))
                        return
                    # Runtime output namespaces values by DTCG type
                    # (color.*, number.*, typography.*, ...), while the map
                    # exposes the canonical path without that type prefix.
                    resolved_values[path] = node
                    prefix = f"{value_type}."
                    if path.startswith(prefix):
                        resolved_values[path[len(prefix):]] = node
                collect(values)
        except (OSError, json.JSONDecodeError):
            resolved_values = {}
    # Resolve the canonical component path against the same light profile used
    # by the SDK default.  If a sparse/pattern binding has no concrete token,
    # retain an empty value rather than inventing one.
    resolver = None
    try:
        from v2.model import TokenModel
        from v2.resolver import Resolver
        from v2.values import qt_value
        model = TokenModel(source)
        resolver = (Resolver(model, model.context("standard", "light")), qt_value)
    except Exception:
        resolver = None

    def resolved(path: str) -> Any:
        if path in resolved_values:
            return resolved_values[path]
        if resolver is None or "{" in path or "*" in path:
            return ""
        try:
            return resolver[1](resolver[0].resolve(path))
        except Exception:
            return ""

    components: list[dict[str, Any]] = []
    for component in COMPONENTS:
        binding_path = source / "figma-bindings" / f"{component}.json"
        if not binding_path.exists():
            continue
        binding = json.loads(binding_path.read_text(encoding="utf-8"))
        recipe_path = source / "component-recipes" / f"{component}.json"
        recipe = json.loads(recipe_path.read_text(encoding="utf-8")) if recipe_path.exists() else {}
        namespace = binding.get("variableNamespace", {})
        prefix = str(namespace.get("prefix", f"{component}/"))
        canonical_prefix = str(namespace.get("canonicalPrefix", f"component.{component}."))
        rows: list[dict[str, Any]] = []
        seen: set[str] = set()
        for recipe_path_name, token in _walk_refs(recipe):
            canonical = f"{canonical_prefix}{recipe_path_name.replace('/', '.')}"
            if canonical in seen:
                continue
            seen.add(canonical)
            # A recipe may keep a reference under a structural path such as
            # component.button.overrides.0.value.  That path is useful for
            # traceability but is not itself a token; resolve the referenced
            # semantic/primitive token for the value column in that case.
            canonical_value = resolved(canonical)
            if canonical_value == "":
                canonical_value = resolved(token)
            rows.append({"variable": prefix + recipe_path_name, "family": "DSM/Component",
                         "value": token, "canonicalPath": canonical, "canonicalValue": canonical_value,
                         "status": "specified",
                         "note": "V2 recipe binding; value is resolved by the token compiler."})
        for rule in binding.get("bindingRules", []):
            canonical = rule.get("canonicalToken") or rule.get("canonicalPattern")
            if not canonical:
                continue
            variable = rule.get("figmaVariable") or (prefix + str(canonical).replace(canonical_prefix, "", 1))
            key = f"{variable}|{canonical}"
            if key in seen:
                continue
            seen.add(key)
            rows.append({"variable": variable, "family": "DSM/Component", "value": "",
                         "canonicalPath": canonical, "canonicalValue": "", "status": "specified",
                         "note": "V2 binding rule; concrete value is supplied by the recipe/compiler."})
        components.append({"componentId": component,
                           "publicComponentSet": binding.get("publicComponentSet", f"ComponentLab/{component.title()}"),
                           "figmaComponentSet": binding.get("figmaComponentSet", ""),
                           "figmaComponentSetStatus": binding.get("figmaComponentSetStatus", "verified"),
                           "figmaVariantCount": binding.get("figmaVariantCount", 0),
                           "figmaVariantCountStatus": binding.get("figmaVariantCountStatus", "verified"),
                           "variantAxes": binding.get("variantAxes", {}),
                           "variableNamespace": namespace, "bindings": rows,
                           "source": f"source/figma-bindings/{component}.json",
                           "valueAuthority": binding.get("valueAuthority", "source/component-recipes/*.json")})
    return {"$description": "ComponentLab V2 runtime map generated from V2 binding manifests and recipes.",
            "schemaVersion": "2.0",
            "authority": {"componentBindings": "source/figma-bindings/*.json",
                          "componentValues": "source/component-recipes/*.json",
                          "legacyAuditSnapshot": "value-only evidence; never a variable-name source"},
            "components": components}


def write_map(root: Path, output: Path) -> dict[str, Any]:
    from .output_guard import require_build_workspace
    require_build_workspace(output)
    result = build_map(root)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return result
