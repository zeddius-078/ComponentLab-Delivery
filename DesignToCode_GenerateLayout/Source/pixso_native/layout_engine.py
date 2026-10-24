"""Static flex-to-Qt layout lowering; no browser or first-route dependency.

Input is a bounded, source-backed declaration record from the second route's
parser. This module does NOT parse Vue or claim computed browser measurements.
Unsupported CSS fails explicitly rather than flattening into fixed coordinates.
"""
import json
import math
from pathlib import Path
import re
import xml.etree.ElementTree as ET


class LayoutError(ValueError):
    pass


def pixels(value, field):
    if isinstance(value, str) and re.fullmatch(r"[+-]?(?:0+(?:\.0*)?|\.0+)", value):
        value = 0
    if isinstance(value, str) and re.fullmatch(r"\d+(?:\.\d+)?px", value):
        value = float(value[:-2])
    if (isinstance(value, bool) or not isinstance(value, (int, float))
            or not math.isfinite(value) or value < 0 or int(value) != value):
        raise LayoutError("UNSUPPORTED_LENGTH: " + field)
    return int(value)


def property_value(parent, name, kind, value):
    prop = ET.SubElement(parent, "property", name=name)
    ET.SubElement(prop, kind).text = str(value)


class LayoutEngine:
    def __init__(self, mapping_path=None):
        path = mapping_path or Path(__file__).with_name("layout-mapping.json")
        self.mapping = json.loads(Path(path).read_text(encoding="utf-8"))
        if self.mapping.get("schemaVersion") != 1:
            raise LayoutError("LAYOUT_MAPPING_VERSION")
        if self.mapping["directions"] != {"row": "QHBoxLayout", "column": "QVBoxLayout"}:
            raise LayoutError("UNSUPPORTED_LAYOUT_CLASS")
        self.names = set()
        self.resolved_axes = {}

    def name(self, value):
        if not isinstance(value, str) or not re.fullmatch(r"[^\W\d]\w*", value):
            raise LayoutError("INVALID_OBJECT_NAME")
        if value in self.names:
            raise LayoutError("DUPLICATE_OBJECT_NAME: " + value)
        self.names.add(value)
        return value

    def fixed_size(self, widget, css, parent_direction, parent_css=None, viewport=None,
                   parent_axes=None, sibling_count=None):
        """Only lower proved, non-shrinking border-box lengths.

        CSS auto is deliberately left to Qt's preferred policy, not labelled Hug.
        Percentage/flex sizing needs a separate constraint resolver.
        """
        fixed = {}
        fill = set()
        percent_axes = set()
        if parent_axes is None:
            parent_axes = set()
            for axis in ("width", "height"):
                try:
                    pixels((parent_css or {}).get(axis), axis)
                    parent_axes.add(axis)
                except LayoutError:
                    pass
        grow = str(css.get("flex-grow", 0))
        if grow not in ("0", "1"):
            raise LayoutError("UNSUPPORTED_FLEX_GROW")
        main_axis = None
        if parent_direction in ("row", "column"):
            main_axis = "width" if parent_direction == "row" else "height"
        basis = css.get("flex-basis")
        if basis is not None:
            # A definite flex basis replaces the main-axis length. Only a zero
            # basis is lowered: the base size is then zero, so the item takes
            # the free main-axis space, which is the same result as the proven
            # flex-grow path, and the shrink factor cannot change it.
            if str(basis) not in ("0", "0px"):
                raise LayoutError("UNSUPPORTED_FLEX_BASIS: " + str(basis))
            if grow != "1":
                raise LayoutError("FLEX_BASIS_ZERO_REQUIRES_FLEX_GROW")
            if main_axis is None:
                raise LayoutError("FLEX_BASIS_PARENT_REQUIRED")
            css = {key: value for key, value in css.items() if key != main_axis}
            fill.add(main_axis)
        elif grow == "1":
            if not parent_direction:
                raise LayoutError("FLEX_GROW_PARENT_REQUIRED")
            if css.get(main_axis, "auto") != "auto" or str(css.get("flex-shrink", 1)) != "0":
                raise LayoutError("FLEX_GROW_REQUIRES_AUTO_NONSHRINKING_BASIS")
            fill.add(main_axis)
        for axis in ("width", "height"):
            value = css.get(axis)
            if value is None or value == "auto":
                continue
            if css.get("box-sizing") != "border-box":
                raise LayoutError("BORDER_BOX_REQUIRED: " + axis)
            if value == "100%" and parent_direction is None and viewport:
                fill.add(axis)
                percent_axes.add(axis)
                continue
            main = "width" if parent_direction == "row" else "height"
            if value == "100%" and parent_direction:
                if axis not in parent_axes:
                    raise LayoutError("DEFINITE_PERCENTAGE_PARENT_REQUIRED: " + axis)
                if axis == main and sibling_count != 1:
                    raise LayoutError("MAIN_PERCENTAGE_REQUIRES_ONLY_CHILD")
                # In a flex cross-axis, border-box 100% occupies the parent's
                # content width/height. Item alignment is suppressed below.
                fill.add(axis)
                percent_axes.add(axis)
                continue
            if parent_direction and axis == main and str(css.get("flex-shrink", 1)) != "0":
                raise LayoutError("SHRINKABLE_FIXED_LENGTH_UNSUPPORTED: " + axis)
            fixed[axis] = pixels(value, axis)
            if fixed[axis] > 16777215:
                raise LayoutError("QT_WIDGET_SIZE_LIMIT: " + axis)
        if "flex-shrink" in css and str(css["flex-shrink"]) not in ("0", "1"):
            raise LayoutError("UNSUPPORTED_FLEX_SHRINK")
        if fill:
            prop = ET.SubElement(widget, "property", name="sizePolicy")
            policy = ET.SubElement(prop, "sizepolicy", {
                "hsizetype": "Expanding" if "width" in fill else "Preferred",
                "vsizetype": "Expanding" if "height" in fill else "Preferred",
            })
            ET.SubElement(policy, "horstretch").text = "0"
            ET.SubElement(policy, "verstretch").text = "0"
        definite = set(fixed) | percent_axes
        if grow == "1" and main_axis in parent_axes:
            definite.add(main_axis)
        if not fixed:
            return definite
        # Setting both bounds preserves fixed dimensions independently of Qt
        # style hints. No page QSS overrides are injected into SDK controls.
        for prop_name, default in (("minimumSize", 0), ("maximumSize", 16777215)):
            prop = ET.SubElement(widget, "property", name=prop_name)
            size = ET.SubElement(prop, "size")
            for axis in ("width", "height"):
                ET.SubElement(size, axis).text = str(fixed.get(axis, default))
        return definite

    def widget(self, node, component_emitter=None, parent_direction=None, parent_css=None,
               parent_axes=None, sibling_count=None):
        """component_emitter owns SDK boundaries; unknown components never flatten."""
        if not node.get("source"):
            raise LayoutError("SOURCE_EVIDENCE_REQUIRED")
        name = self.name(node["name"])
        if node["kind"] == "component":
            if component_emitter is None:
                raise LayoutError("COMPONENT_MAPPING_REQUIRED: " + name)
            css = node.get("css", {})
            if set(css) - {"width", "height", "box-sizing", "flex-shrink", "flex-grow",
                           "flex-basis"}:
                raise LayoutError("UNLOWERED_COMPONENT_PLACEMENT")
            widget = component_emitter(node, name)
            self.fixed_size(widget, css, parent_direction, parent_css,
                            parent_axes=parent_axes, sibling_count=sibling_count)
            return widget
        if node["kind"] != "container":
            raise LayoutError("UNSUPPORTED_NODE_KIND")
        widget = ET.Element("widget", {"class": "QWidget", "name": name})
        css = node.get("css", {})
        viewport = node.get("viewport")
        if viewport is not None:
            if parent_direction is not None:
                raise LayoutError("VIEWPORT_ONLY_ON_PAGE_ROOT")
            if not isinstance(viewport, dict) or not viewport.get("source"):
                raise LayoutError("VIEWPORT_SOURCE_REQUIRED")
            for axis in ("width", "height"):
                length = pixels(viewport.get(axis), "viewport-" + axis)
                if not 1 <= length <= 16777215:
                    raise LayoutError("INVALID_VIEWPORT_SIZE")
            # This initializes a page viewport; it does not position children
            # or constrain a percentage root to a fixed width/height.
            prop = ET.SubElement(widget, "property", name="geometry")
            rect = ET.SubElement(prop, "rect")
            ET.SubElement(rect, "x").text = "0"
            ET.SubElement(rect, "y").text = "0"
            for axis in ("width", "height"):
                ET.SubElement(rect, axis).text = str(pixels(viewport[axis], axis))
        allowed = {"display", "flex-direction", "gap", "padding-top", "padding-right",
                   "padding-bottom", "padding-left", "align-items", "justify-content",
                   "width", "height", "box-sizing", "flex-shrink", "flex-grow",
                   "flex-basis", "overflow"}
        unknown = sorted(set(css) - allowed)
        if unknown:
            raise LayoutError("UNLOWERED_LAYOUT_DECLARATIONS: " + ", ".join(unknown))
        if "overflow" in css:
            if css["overflow"] != "hidden":
                raise LayoutError("UNSUPPORTED_OVERFLOW")
            # QWidget children are clipped to their parent rectangle. Keep a
            # traceable declaration; no extra clipping is added to hide errors.
            prop = ET.SubElement(widget, "property", name="d2cOverflow", stdset="0")
            ET.SubElement(prop, "string").text = "hidden"
        axes = self.fixed_size(widget, css, parent_direction, parent_css, viewport,
                               parent_axes, sibling_count)
        self.resolved_axes[name] = axes
        if css.get("display") != "flex":
            raise LayoutError("EXPLICIT_FLEX_REQUIRED")
        direction = css.get("flex-direction", "row")
        if direction not in self.mapping["directions"]:
            raise LayoutError("UNSUPPORTED_FLEX_DIRECTION")
        cross = css.get("align-items", "stretch")
        if cross not in self.mapping["crossAlignment"]:
            raise LayoutError("UNSUPPORTED_CROSS_ALIGNMENT")
        justify = css.get("justify-content", "flex-start")
        if justify not in ("flex-start", "flex-end", "center", "space-between"):
            raise LayoutError("UNSUPPORTED_MAIN_ALIGNMENT")
        # Explicit zeros prevent Qt platform defaults from inserting unproven gaps.
        layout = ET.SubElement(widget, "layout", {
            "class": self.mapping["directions"][direction],
            "name": self.name(name + "_layout"),
        })
        property_value(layout, "spacing", "number", pixels(css.get("gap", 0), "gap"))
        for side in ("left", "top", "right", "bottom"):
            property_value(layout, side + "Margin", "number",
                           pixels(css.get("padding-" + side, 0), "padding-" + side))
        children = node.get("children", [])
        growing = [str(child.get("css", {}).get("flex-grow", 0)) == "1"
                   for child in children]
        if sum(growing) > 1:
            raise LayoutError("MULTIPLE_FLEX_GROW_BASES_UNSUPPORTED")
        main_axis = "width" if direction == "row" else "height"
        filling = [child.get("css", {}).get(main_axis) == "100%" for child in children]
        stretching = [grow or full for grow, full in zip(growing, filling)]
        if any(stretching):
            if any(growing[index] and child["kind"] == "component"
                   for index, child in enumerate(children)):
                raise LayoutError("SDK_FLEX_GROW_CONTRACT_REQUIRED")
            # A sole growing item absorbs free main-axis space before
            # justify-content is applied. Do not let alignment spacers take it.
            layout.set("stretch", ",".join("1" if value else "0" for value in stretching))
        if not any(stretching) and justify in ("flex-end", "center") and children:
            self.spacer(layout, name + "_before", direction)
        for index, child in enumerate(children):
            item = ET.SubElement(layout, "item")
            alignment = self.mapping["crossAlignment"][cross][direction]
            cross_axis = "height" if direction == "row" else "width"
            if child.get("css", {}).get(cross_axis) == "100%":
                if child["kind"] == "component":
                    raise LayoutError("SDK_PERCENTAGE_SIZE_CONTRACT_REQUIRED")
                alignment = None
            if alignment:
                item.set("alignment", alignment)
            if filling[index] and child["kind"] == "component":
                raise LayoutError("SDK_PERCENTAGE_SIZE_CONTRACT_REQUIRED")
            item.append(self.widget(child, component_emitter, direction, css, axes, len(children)))
            if not any(stretching) and justify == "space-between" and index < len(children) - 1:
                self.spacer(layout, name + "_between_" + str(index), direction)
        if not any(stretching) and justify in ("flex-start", "center") and children:
            self.spacer(layout, name + "_after", direction)
        return widget

    def spacer(self, layout, name, direction):
        item = ET.SubElement(layout, "item")
        spacer = ET.SubElement(item, "spacer", name=self.name(name))
        property_value(spacer, "orientation", "enum",
                       "Qt::Horizontal" if direction == "row" else "Qt::Vertical")
        property_value(spacer, "sizeType", "enum", "QSizePolicy::Expanding")
        prop = ET.SubElement(spacer, "property", name="sizeHint")
        size = ET.SubElement(prop, "size")
        ET.SubElement(size, "width").text = "0"
        ET.SubElement(size, "height").text = "0"

    def document(self, page_name, root, component_emitter=None):
        self.names.clear()
        self.resolved_axes.clear()
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", page_name):
            raise LayoutError("INVALID_PAGE_NAME")
        ui = ET.Element("ui", version="4.0")
        ET.SubElement(ui, "class").text = page_name
        ui.append(self.widget(root, component_emitter))
        return ui
