"""Thin adapter from the second route's styled tree to layout-engine records.

The caller selects the root by scoped source identity. Component boundaries
are handed to a component adapter; they are never flattened as plain divs.
"""
import hashlib
import json
import re

from layout_engine import LayoutEngine, LayoutError


class StyledLayoutError(ValueError):
    pass


LAYOUT_PROPERTIES = {
    "display", "flex-direction", "gap", "padding-top", "padding-right",
    "padding-bottom", "padding-left", "align-items", "justify-content",
    "width", "height", "box-sizing", "flex-shrink", "flex-grow", "flex-basis",
    "overflow",
}
VISUAL_PROPERTIES = {
    "color", "background", "background-color", "background-image",
    "background-size", "background-position", "background-repeat",
    "border", "border-radius", "border-color", "border-width", "border-style",
    "font-family", "font-size", "font-style", "font-weight", "line-height",
    "letter-spacing", "text-align", "text-decoration", "white-space",
    "word-break", "opacity", "box-shadow", "visibility", "cursor",
}
# A definite length is a plain pixel value. Percentages and "auto" are not
# definite: a percentage still depends on a parent, "auto" is content sized.
DEFINITE_LENGTH = re.compile(r"^(?:\d+(?:\.\d+)?|\.\d+)(?:px)?$")


def is_definite_length(value):
    return isinstance(value, str) and bool(DEFINITE_LENGTH.match(value))



def select_root(tree, instance_path, node_id):
    matches = []

    def visit(node):
        source = node.get("source", {})
        if (node.get("kind") == "element"
                and source.get("instancePath") == instance_path
                and node.get("attrs", {}).get("id") == node_id):
            matches.append(node)
        for child in node.get("children", []):
            visit(child)
    visit(tree)
    if len(matches) != 1:
        raise StyledLayoutError("ROOT_IDENTITY_NOT_UNIQUE: " + str(len(matches)))
    return matches[0]


def object_name(node):
    source = node.get("source")
    if not source or "instancePath" not in source or "offset" not in source:
        raise StyledLayoutError("SCOPED_SOURCE_REQUIRED")
    identity = json.dumps([source.get("file"), source["instancePath"],
                           source["offset"], node.get("attrs", {}).get("id")],
                          ensure_ascii=False, separators=(",", ":"))
    # A same-batch Pixso DSL may provide a human-readable layer/component
    # name.  It is evidence, not an identity key: retain the source-derived
    # digest as a suffix so duplicate names and repeated instances remain
    # unambiguous.  Without DSL evidence we intentionally keep the old stable
    # anonymous name for backwards-compatible generation.
    source_name = node.get("designName")
    source_guid = str(source.get("designNodeId") or node.get("attrs", {}).get("id") or "")
    if source_name:
        readable = re.sub(r"[^\w]+", "_", str(source_name), flags=re.UNICODE).strip("_")
        readable = re.sub(r"_+", "_", readable)[:48]
        guid = re.sub(r"[^0-9A-Za-z]+", "_", source_guid).strip("_")
        if readable:
            return "px_" + readable + ("_" + guid if guid else "") + "_" + hashlib.sha256(identity.encode("utf-8")).hexdigest()[:10]
    return "node_" + hashlib.sha256(identity.encode("utf-8")).hexdigest()[:20]


def lower_tree(root, component_adapter=None, parent_css=None, viewport=None,
               parent_axes=None, sibling_count=None):
    """Return layout and separate page styles, without silently dropping CSS.

    Component adapter owns all props, slots, styles and resources at a public
    SDK boundary. Ordinary visual declarations remain separate for QSS lowering;
    callers must consume them before claiming a complete page.
    """
    styles = {}
    trace = {}
    resolved = {}
    declarations = []

    def visit(node, parent_direction=None, parent_css=None, parent_axes=None, sibling_count=None):
        if node.get("kind") == "text":
            if not node.get("text", "").strip():
                return None
            raise StyledLayoutError("TEXT_LOWERING_REQUIRED")
        if node.get("kind") != "element":
            raise StyledLayoutError("UNSUPPORTED_STYLED_NODE")
        if node.get("inLayout") is False:
            return None
        name = object_name(node)
        if name in trace:
            raise StyledLayoutError("DUPLICATE_SCOPED_SOURCE")
        trace[name] = node["source"]
        if node.get("componentBoundaries"):
            if component_adapter is None:
                raise StyledLayoutError("COMPONENT_ADAPTER_REQUIRED: " + name)
            result = component_adapter(node, name)
            if result.get("kind") != "component" or result.get("name") != name:
                raise StyledLayoutError("INVALID_COMPONENT_ADAPTER_RESULT")
            return result
        if node.get("tag") not in ("div", "section", "main", "header", "footer", "nav"):
            raise StyledLayoutError("UNSUPPORTED_ELEMENT: " + str(node.get("tag")))
        values = node.get("style", {}).get("values", {})
        if values.get("overflow") == "hidden":
            # Qt's rectangular child clipping is not CSS rounded padding-box
            # clipping. Reject that combination until a dedicated adapter exists.
            for key, value in values.items():
                if key.startswith("border") and value not in ("0", "0px", "none"):
                    raise StyledLayoutError("BORDERED_OVERFLOW_CLIP_UNSUPPORTED")
        css = {}
        visual = {}
        for key, value in values.items():
            if key in LAYOUT_PROPERTIES:
                if key == "flex-basis" and value not in ("0", "0px"):
                    # Only a zero basis is provable here: it replaces the
                    # main-axis length and lets the item absorb the free
                    # main-axis space. Any other basis stays unsupported.
                    raise StyledLayoutError("UNLOWERED_CSS: " + key)
                css[key] = value
                if key == "flex-basis":
                    declarations.append({"objectName": name, "property": key,
                                         "value": value, "disposition": "applied",
                                         "reason": "a zero basis lets the item "
                                                   "absorb the free main-axis space"})
            elif key in VISUAL_PROPERTIES:
                visual[key] = value
            elif key.startswith("margin-") and value in ("0", "0px"):
                pass
            elif key == "position" and value in ("static", "relative"):
                # Offsets remain unsupported below; relative without offsets
                # keeps its ordinary flow box.
                pass
            elif key == "min-width" and value in ("0", "0px"):
                # A zero minimum only removes the CSS automatic minimum size.
                # The Qt minimum is already zero, so the declaration changes
                # nothing here. It is recorded, never silently applied.
                declarations.append({"objectName": name, "property": key,
                                     "value": value, "disposition": "no-op",
                                     "reason": "a zero minimum matches the Qt default"})
            elif (key == "align-self" and value == "stretch"
                  and parent_direction in ("row", "column")
                  and is_definite_length(values.get(
                      "height" if parent_direction == "row" else "width"))):
                # Stretch never applies to an item with a definite cross size.
                declarations.append({"objectName": name, "property": key,
                                     "value": value, "disposition": "no-op",
                                     "reason": "a definite cross size is not stretched"})
            else:
                raise StyledLayoutError("UNLOWERED_CSS: " + key)
        result = {"kind": "container", "name": name, "source": node["source"],
                  "css": css, "children": []}
        if node is root and viewport is not None:
            result["viewport"] = viewport
        # Check this container before descending, but do not execute SDK emitters.
        # Parent axis matters for percentage sizing and flex-shrink semantics.
        engine = LayoutEngine()
        engine.widget(result, parent_direction=parent_direction, parent_css=parent_css,
                      parent_axes=parent_axes, sibling_count=sibling_count)
        axes = engine.resolved_axes[name]
        resolved[name] = sorted(axes)
        styles[name] = visual
        children = [child for child in node.get("children", [])
                    if child.get("inLayout") is not False
                    and not (child.get("kind") == "text" and not child.get("text", "").strip())]
        for child in children:
            lowered = visit(child, css.get("flex-direction", "row"), css, axes, len(children))
            if lowered is not None:
                result["children"].append(lowered)
        return result

    result = visit(root, parent_css.get("flex-direction", "row") if parent_css else None,
                   parent_css, parent_axes, sibling_count)
    if result is None:
        raise StyledLayoutError("ROOT_NOT_IN_LAYOUT")
    return {"root": result, "pageStyles": styles, "trace": trace,
            "definiteAxes": resolved, "layoutDeclarations": declarations}


def inspect_tree(root, viewport=None):
    """Collect per-node blockers without weakening lower_tree's fail-closed gate."""
    issues, components = [], []
    containers = 0

    def visit(node, parent_css=None, parent_axes=None, sibling_count=None):
        nonlocal containers
        if node.get("inLayout") is False:
            return
        if node.get("componentBoundaries"):
            components.append({"source": node.get("source"),
                               "boundaries": node["componentBoundaries"],
                               "style": node.get("style", {}).get("values", {})})
            return
        shallow = dict(node, children=[])
        axes = set()
        try:
            result = lower_tree(shallow, parent_css=parent_css,
                                viewport=viewport if node is root else None,
                                parent_axes=parent_axes, sibling_count=sibling_count)
            axes = set(result["definiteAxes"].get(result["root"]["name"], []))
        except (StyledLayoutError, LayoutError) as error:
            # Whitespace text is an intentionally absent layout item.
            if not (node.get("kind") == "text" and not node.get("text", "").strip()):
                issues.append({"source": node.get("source"), "error": str(error),
                               "style": node.get("style", {}).get("values", {})})
        if node.get("kind") == "element":
            containers += 1
        children = [child for child in node.get("children", [])
                    if child.get("inLayout") is not False
                    and not (child.get("kind") == "text" and not child.get("text", "").strip())]
        for child in children:
            visit(child, node.get("style", {}).get("values", {}), axes, len(children))
    visit(root)
    return {"scope": "outer layout only; SDK interiors remain component-owned",
            "containers": containers, "layoutIssues": issues,
            "componentBoundaries": components, "pageGeneration": "not-run"}
