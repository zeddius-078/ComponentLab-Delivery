"""Compose a bounded, clipped CSS vector canvas; not a general DOM renderer.

The caller must separately validate how this canvas fits its component slot.
No component identity, viewport or SDK icon size is inferred here.
"""
import hashlib
import math
import re
import xml.etree.ElementTree as ET

from assets import AssetError, css_url, validate_svg


def length(value, basis=None):
    match = re.fullmatch(r"(\d+(?:\.\d+)?)(px|%)", str(value))
    if not match or (match[2] == "%" and basis is None):
        raise AssetError("VECTOR_EXPLICIT_LENGTH_REQUIRED")
    result = float(match[1]) * (basis / 100 if match[2] == "%" else 1)
    if not math.isfinite(result):
        raise AssetError("VECTOR_NONFINITE_LENGTH")
    return result


def number(value):
    return format(value, ".12g")


def geometry_style(node, allowed):
    values = node.get("style", {}).get("values", {})
    for key, value in values.items():
        if key in allowed or key == "word-break":
            continue
        if key == "box-sizing" and value == "border-box":
            continue
        if (key.startswith("margin-") or key.startswith("padding-")) and value in ("0", "0px"):
            continue
        raise AssetError("VECTOR_UNSUPPORTED_STYLE: " + key)
    return values


def compose_canvas(node, bundle):
    """Return one resource from direct absolute background layers in source order."""
    if node.get("kind") != "element" or node.get("visible") is False:
        raise AssetError("VECTOR_VISIBLE_CANVAS_REQUIRED")
    css = geometry_style(node, {"width", "height", "position", "left", "top", "overflow"})
    if css.get("overflow") != "hidden" or css.get("position") != "absolute":
        raise AssetError("VECTOR_CLIPPED_CANVAS_REQUIRED")
    width, height = length(css.get("width")), length(css.get("height"))
    if not 0 < width <= 4096 or not 0 < height <= 4096:
        raise AssetError("VECTOR_CANVAS_SIZE_LIMIT")
    svg = ET.Element("svg", {"xmlns": "http://www.w3.org/2000/svg",
        "width": number(width), "height": number(height),
        "viewBox": "0 0 %s %s" % (number(width), number(height))})
    layers = []
    for child in node.get("children", []):
        if child.get("kind") == "text" and not child.get("text", "").strip():
            continue
        if child.get("kind") != "element" or child.get("children"):
            raise AssetError("VECTOR_DIRECT_LEAF_REQUIRED")
        if child.get("visible") is False:
            continue
        style = geometry_style(child, {"position", "left", "top", "right", "bottom",
            "width", "height", "background-image", "background-size", "background-repeat"})
        if (style.get("position") != "absolute" or style.get("background-size") != "100% 100%"
                or style.get("background-repeat") != "no-repeat"):
            raise AssetError("VECTOR_STRETCHED_BACKGROUND_REQUIRED")
        x, y = length(style.get("left"), width), length(style.get("top"), height)
        w, h = length(style.get("width"), width), length(style.get("height"), height)
        if w <= 0 or h <= 0 or x + w > width or y + h > height:
            raise AssetError("VECTOR_LAYER_OUTSIDE_CANVAS")
        source = child.get("style", {}).get("origins", {}).get("background-image")
        if not source or not source.get("file"):
            raise AssetError("VECTOR_SOURCE_REQUIRED")
        resource = bundle.add(css_url(style.get("background-image", "")), source["file"])
        alias = resource.split("/assets/", 1)[1]
        if not alias.endswith(".svg"):
            raise AssetError("VECTOR_SVG_REQUIRED")
        fragment = ET.fromstring(bundle.files["view/res/assets/" + alias])
        # ID/reference/style rewriting would require a separate SVG contract.
        # The observed fragments contain only explicit paths, not defs or CSS.
        for element in fragment.iter():
            if element.tag.rsplit("}", 1)[-1] not in {"svg", "path", "g"}:
                raise AssetError("VECTOR_COMPLEX_SVG_UNSUPPORTED")
            if any("url(" in value or key.rsplit("}", 1)[-1] in {"href", "style"}
                   for key, value in element.attrib.items()):
                raise AssetError("VECTOR_SVG_REFERENCE_UNSUPPORTED")
            element.attrib.pop("id", None)
        if "viewBox" not in fragment.attrib:
            raise AssetError("VECTOR_VIEWBOX_REQUIRED")
        try:
            vx, vy, vw, vh = [float(part) for part in re.split(r"[ ,]+", fragment.get("viewBox").strip())]
        except (ValueError, TypeError) as error:
            raise AssetError("VECTOR_INVALID_VIEWBOX") from error
        if not all(math.isfinite(v) for v in (vx, vy, vw, vh)) or vw <= 0 or vh <= 0:
            raise AssetError("VECTOR_INVALID_VIEWBOX")
        # Qt 5 SVG does not preserve these nested SVG viewports as a browser
        # does. Lower the explicit viewBox to group transforms instead.
        positioned = ET.SubElement(svg, "g", {"transform":
            "translate(%s %s) scale(%s %s) translate(%s %s)" %
            tuple(number(v) for v in (x, y, w / vw, h / vh, -vx, -vy))})
        inherited = {key: value for key, value in fragment.attrib.items()
                     if key not in {"viewBox", "width", "height", "customFrame", "preserveAspectRatio"}}
        group = ET.SubElement(positioned, "g", inherited)
        for child_svg in fragment:
            for element in child_svg.iter():
                element.tag = element.tag.rsplit("}", 1)[-1]
            group.append(child_svg)
        layers.append({"source": child.get("source"), "resource": resource,
                       "rect": [x, y, w, h]})
    if not layers:
        raise AssetError("VECTOR_EMPTY_CANVAS")
    data = ET.tostring(svg, encoding="utf-8", xml_declaration=True) + b"\n"
    validate_svg(data)
    sha = hashlib.sha256(data).hexdigest()
    alias = "assets/" + sha + ".svg"
    bundle.files["view/res/" + alias] = data
    bundle.evidence.append({"kind": "composed-css-vector-canvas", "sha256": sha,
        "alias": alias, "canvas": [width, height], "source": node.get("source"),
        "layers": layers, "slotPlacement": "not-validated"})
    return ":/pages/" + bundle.page_name + "/" + alias
