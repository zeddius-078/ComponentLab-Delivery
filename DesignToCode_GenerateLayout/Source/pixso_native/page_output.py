"""Emit the five-file static Qt page unit from second-route lowered nodes.

This module does not parse Vue, select a mapping, execute a browser, or run the
first route. SDK installation and actual API compatibility are separate gates.
"""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

from acquire import new_output
from component_engine import ComponentEngine, ComponentError, identifier, literal
from layout_engine import LayoutEngine


def xml_bytes(root):
    ET.indent(root, space="  ")
    return b'<?xml version="1.0" encoding="UTF-8"?>\n' + ET.tostring(root, encoding="utf-8") + b"\n"


def surface_color(value):
    if not isinstance(value, str):
        raise ValueError("INVALID_SURFACE_COLOR")
    if re.fullmatch(r"#[0-9a-fA-F]{3}(?:[0-9a-fA-F]{3})?", value):
        return value.lower()
    if value == "transparent":
        return "rgba(0,0,0,0)"
    match = re.fullmatch(
        r"rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(0(?:\.\d+)?|1(?:\.0+)?)\s*\)",
        value)
    if match and all(0 <= int(match[i]) <= 255 for i in (1, 2, 3)):
        return "rgba(%d,%d,%d,%d)" % (
            int(match[1]), int(match[2]), int(match[3]), round(float(match[4]) * 255))
    raise ValueError("UNSUPPORTED_SURFACE_COLOR")


def page_surfaces(document, styles):
    widgets = {widget.get("name"): widget for widget in document.iter("widget")}
    root_name = document.find("./widget").get("name")
    rules, setup = [], []
    for name, declarations in sorted(styles.items()):
        identifier(name)
        if name not in widgets or widgets[name].get("class") != "QWidget":
            raise ValueError("PAGE_STYLE_REQUIRES_OWN_CONTAINER: " + name)
        if set(declarations) - {"background-color"}:
            raise ValueError("UNLOWERED_PAGE_STYLE: " + name)
        if "background-color" in declarations:
            color = surface_color(declarations["background-color"])
            rules.append("QWidget#%s { background-color: %s; }" % (name, color))
            receiver = "this" if name == root_name else "ui->" + name
            setup.append(receiver + "->setAttribute(Qt::WA_StyledBackground, true);")
    return "\n".join(rules), setup


def render_page(page_name, root, mapping, sdk_release, profile, assets=None, page_styles=None):
    """Pure generation; no writes until all declarations have been validated."""
    identifier(page_name)
    identifier(profile.replace("-", "_"))
    host = mapping.get("pageHost")
    if not isinstance(host, dict) or set(host) != {"class", "header", "constructor"}:
        raise ComponentError("PAGE_HOST_CONTRACT_REQUIRED")
    host_class = identifier(host["class"], qualified=True)
    host_header = host["header"]
    if (not isinstance(host_header, str)
            or not re.fullmatch(r"[A-Za-z0-9_./-]+", host_header)
            or host_header.startswith("/") or ".." in host_header.split("/")):
        raise ComponentError("INVALID_PAGE_HOST_HEADER")
    if host["constructor"] == "profile-parent":
        host_arguments = literal(profile, "string") + ", parent"
    elif host["constructor"] == "parent-only":
        host_arguments = "parent"
    else:
        raise ComponentError("UNSUPPORTED_PAGE_HOST_CONSTRUCTOR")
    if host_class == page_name:
        raise ComponentError("PAGE_HOST_NAME_COLLISION")
    component = ComponentEngine(mapping, sdk_release, profile)
    document = LayoutEngine().document(page_name, root, component.emit)
    if document.find("./widget").get("class") != "QWidget":
        raise ComponentError("PAGE_ROOT_MUST_BE_CONTAINER")
    widgets = list(document.iter("widget"))
    qss, surface_setup = page_surfaces(document, page_styles or {})
    for widget in widgets:
        if widget.get("name") in ("ui", "parent", page_name):
            raise ComponentError("RESERVED_PAGE_MEMBER: " + widget.get("name"))
    document.append(component.declarations())
    resources = ET.SubElement(document, "resources")
    ET.SubElement(resources, "include", location="../view/res/" + page_name + ".qrc")
    ET.SubElement(document, "connections")
    headers = sorted({qt["header"] for qt in component.customwidgets.values()})
    includes = "\n".join('#include <' + header + '>' for header in headers)
    custom = [widget for widget in widgets if widget.get("class") in component.customwidgets]
    declarations = "\n".join("    %s* %s() const;" % (w.get("class"), w.get("name")) for w in custom)
    getters = "\n\n".join(
        "%s* %s::%s() const { return ui->%s; }" %
        (w.get("class"), page_name, w.get("name"), w.get("name")) for w in custom)
    header = """#pragma once
#include <memory>
#include <%s>
%s

namespace Ui { class %s; }
class %s final : public %s {
    Q_OBJECT
public:
    explicit %s(QWidget* parent = nullptr);
    ~%s() override;
%s
private:
    std::unique_ptr<Ui::%s> ui;
};
""" % (host_header, includes, page_name, page_name, host_class,
       page_name, page_name, declarations, page_name)
    calls = "\n".join("    " + call for call in component.initializers + surface_setup)
    source = """#include "%s.h"
#include "ui_%s.h"
#include <QFile>
#include <QResource>
#include <QStringList>
#include <stdexcept>
%s

%s::%s(QWidget* parent)
    : %s(%s), ui(new Ui::%s) {
    Q_INIT_RESOURCE(%s);
    ui->setupUi(this);
%s
    QFile stylesheet(QStringLiteral(":/pages/%s/qss/%s.qss"));
    if (!stylesheet.open(QIODevice::ReadOnly))
        throw std::runtime_error("PAGE_STYLESHEET_MISSING");
    setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
}
%s::~%s() = default;

%s
""" % (page_name, page_name, "\n".join("#include <" + h + ">" for h in sorted(component.source_headers)),
       page_name, page_name, host_class, host_arguments,
       page_name, page_name, calls, page_name, page_name, page_name, page_name, getters)
    qrc = ET.Element("RCC")
    resource = ET.SubElement(qrc, "qresource", prefix="/pages/" + page_name)
    ET.SubElement(resource, "file", alias="qss/" + page_name + ".qss").text = "qss/" + page_name + ".qss"
    if assets is not None:
        if assets.page_name != page_name:
            raise ValueError("RESOURCE_PAGE_MISMATCH")
        assets.register(resource)
    # Only exact ordinary-container selectors; SDK appearance stays SDK-owned.
    files = {
        "form/" + page_name + ".ui": xml_bytes(document),
        "include/" + page_name + ".h": header.encode("utf-8"),
        "src/" + page_name + ".cpp": source.encode("utf-8"),
        "view/res/qss/" + page_name + ".qss":
            ("/* Page-specific styles only; public component visuals belong to the SDK. */\n"
             + qss + "\n").encode("utf-8"),
        "view/res/" + page_name + ".qrc": xml_bytes(qrc),
    }
    if assets is not None:
        files.update(assets.files)
    return files


def write_page(destination, files):
    """Work-stage output only. A validated delivery promotion is not implied."""
    for name, data in files.items():
        path = Path(name)
        if path.is_absolute() or ".." in path.parts or "\\" in name or not isinstance(data, bytes):
            raise ValueError("INVALID_PAGE_OUTPUT")
        if not re.fullmatch(r"[A-Za-z0-9_./-]+", name):
            raise ValueError("INVALID_PAGE_OUTPUT")
    output = new_output(Path(destination))
    for name, data in files.items():
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    return output
