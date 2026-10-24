"""Read-only local asset collection for a deterministic Qt resource bundle."""
import hashlib
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from component_engine import identifier


class AssetError(ValueError):
    pass


def css_url(value):
    match = re.fullmatch(r"""url\(\s*(?:"([^"]+)"|'([^']+)'|([^'"\s()]+))\s*\)""", value)
    if not match:
        raise AssetError("SINGLE_LOCAL_CSS_URL_REQUIRED")
    return next(group for group in match.groups() if group is not None)


def bind_collections(collections, definitions, bundle):
    """Resolve ordered anatomy evidence to explicitly configured slot values.

    No component identity is promoted here. The component engine still requires
    a verified parent rule, and rejects undeclared slots and incompatible types.
    """
    if set(collections) != set(definitions):
        raise AssetError("COLLECTION_DEFINITIONS_MISMATCH")
    slots = {}
    for name, spec in definitions.items():
        collection = collections[name]
        items = collection["items"]
        selected = collection["selectedIndex"]
        if type(selected) is not int or not 0 <= selected < len(items):
            raise AssetError("INVALID_COLLECTION_INDEX")
        for field, slot in spec.get("outputs", {}).items():
            if slot in slots or not isinstance(slot, str):
                raise AssetError("DUPLICATE_OR_INVALID_COLLECTION_SLOT")
            if field == "labels":
                value = [item["text"]["text"] for item in items]
                if not all(isinstance(label, str) for label in value):
                    raise AssetError("INVALID_COLLECTION_LABEL")
            elif field == "selectedIndex":
                value = selected
            elif field == "icons":
                value = []
                for item in items:
                    if len(item["images"]) != 1:
                        raise AssetError("COLLECTION_ICON_NOT_UNIQUE")
                    image = item["images"][0]
                    reference = image["reference"]
                    if image["kind"] == "css-background":
                        reference = css_url(reference)
                    elif image["kind"] != "image":
                        raise AssetError("UNSUPPORTED_COLLECTION_IMAGE")
                    value.append(bundle.add(reference, image["source"]["file"]))
            else:
                raise AssetError("UNKNOWN_COLLECTION_OUTPUT")
            slots[slot] = value
    return slots


def no_redirects(path):
    for item in (path, *path.parents):
        if item.exists():
            attrs = getattr(item.lstat(), "st_file_attributes", 0)
            if item.is_symlink() or attrs & 0x400:
                raise AssetError("ASSET_REDIRECT_REFUSED")


def bind_icons(record, definitions, bundle):
    """Explicit icon-only content binding with recorded, opt-in system fallback.

    Missing images may use the configured standard icon. Invalid supplied files
    still fail; the fallback must not conceal unsafe or corrupt resources.
    """
    slots, consumed, fallbacks = {}, [], []
    for target, spec in definitions.items():
        if any(record.get("declaredProps", {}).get(key) != value
               for key, value in spec.get("when", {}).items()):
            continue
        if spec.get("source") != "single-visible-image":
            raise AssetError("UNSUPPORTED_ICON_SELECTOR")
        images = record["content"]["images"]
        if len(images) > 1:
            raise AssetError("ICON_SLOT_NOT_UNIQUE")
        if images:
            image = images[0]
            reference = css_url(image["reference"]) if image["kind"] == "css-background" else image["reference"]
            if image["kind"] not in ("css-background", "image"):
                raise AssetError("UNSUPPORTED_ICON_IMAGE")
            slots[target] = {"kind": "resource", "path": bundle.add(reference, image["source"]["file"])}
            consumed.append(image)
        else:
            icon = spec.get("missingFallback")
            if icon != "SP_FileIcon":
                raise AssetError("APPROVED_SYSTEM_ICON_REQUIRED")
            slots[target] = {"kind": "qt-standard", "name": icon}
            fallbacks.append({"slot": target, "reason": "SOURCE_ICON_MISSING",
                              "replacement": "QStyle::" + icon,
                              "source": "Qt system style; not a design asset"})
    return slots, consumed, fallbacks


def validate_svg(data):
    if b"<!DOCTYPE" in data.upper() or b"<!ENTITY" in data.upper():
        raise AssetError("SVG_EXTERNAL_DEFINITION_REFUSED")
    try:
        root = ET.fromstring(data)
    except ET.ParseError as error:
        raise AssetError("INVALID_SVG") from error
    if root.tag.rsplit("}", 1)[-1] != "svg":
        raise AssetError("SVG_ROOT_REQUIRED")
    for node in root.iter():
        if node.tag.rsplit("}", 1)[-1] in ("script", "foreignObject"):
            raise AssetError("ACTIVE_SVG_CONTENT_REFUSED")
        for key, value in node.attrib.items():
            local_key = key.rsplit("}", 1)[-1]
            if local_key.lower().startswith("on"):
                raise AssetError("ACTIVE_SVG_ATTRIBUTE_REFUSED")
            if local_key == "href" and not value.startswith("#"):
                raise AssetError("SVG_EXTERNAL_REFERENCE_REFUSED")
            for reference in re.findall(r"url\(([^)]+)\)", value):
                if not reference.strip(" '\"").startswith("#"):
                    raise AssetError("SVG_EXTERNAL_REFERENCE_REFUSED")
        if node.tag.rsplit("}", 1)[-1] == "style" and node.text:
            references = re.findall(r"url\(([^)]+)\)", node.text)
            if "@import" in node.text or any(
                    not value.strip(" '\"").startswith("#") for value in references):
                raise AssetError("SVG_EXTERNAL_STYLE_REFUSED")


class ResourceBundle:
    def __init__(self, input_root, page_name):
        identifier(page_name)
        no_redirects(Path(input_root).absolute())
        self.root = Path(input_root).resolve(strict=True)
        self.page_name = page_name
        self.files = {}
        self.evidence = []

    def add(self, reference, origin):
        if (not isinstance(reference, str) or "\\" in reference
                or any(char in reference for char in (":", "?", "#", "\0"))
                or reference.startswith("/")):
            raise AssetError("LOCAL_ASSET_REFERENCE_REQUIRED")
        origin_path = Path(origin)
        if origin_path.is_absolute() or ".." in origin_path.parts:
            raise AssetError("INVALID_ASSET_ORIGIN")
        path = (self.root / reference[2:] if reference.startswith("@/")
                else self.root / origin_path.parent / reference)
        no_redirects(path.absolute())
        path = path.resolve(strict=True)
        try:
            relative = path.relative_to(self.root).as_posix()
        except ValueError as error:
            raise AssetError("ASSET_OUTSIDE_INPUT") from error
        suffix = path.suffix.lower()
        if suffix not in (".svg", ".png", ".jpg", ".jpeg", ".ttf", ".otf"):
            raise AssetError("UNSUPPORTED_QT_ASSET_FORMAT")
        if path.stat().st_size > 64 * 1024 * 1024:
            raise AssetError("ASSET_SIZE_LIMIT")
        data = path.read_bytes()
        if suffix == ".svg":
            validate_svg(data)
        if suffix == ".png" and not data.startswith(b"\x89PNG\r\n\x1a\n"):
            raise AssetError("INVALID_PNG_SIGNATURE")
        if suffix in (".jpg", ".jpeg") and not data.startswith(b"\xff\xd8\xff"):
            raise AssetError("INVALID_JPEG_SIGNATURE")
        sha = hashlib.sha256(data).hexdigest()
        alias = "assets/" + sha + suffix
        self.files["view/res/" + alias] = data
        evidence = {"source": relative, "sha256": sha, "bytes": len(data), "alias": alias}
        if evidence not in self.evidence:
            self.evidence.append(evidence)
        return ":/pages/" + self.page_name + "/" + alias

    def register(self, qresource):
        for output in sorted(self.files):
            alias = output[len("view/res/"):]
            ET.SubElement(qresource, "file", alias=alias).text = alias
