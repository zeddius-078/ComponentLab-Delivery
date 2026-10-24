"""Configuration-driven static component lowering, independent of Vue runtime.

Only typed, declared public API calls are emitted. This is a compile-time
generator, not a runtime reflection dispatcher. Mapping authors remain
responsible for proving that a declared API exists in the selected public SDK.
"""
import json
from pathlib import PurePosixPath
import re
import xml.etree.ElementTree as ET


class ComponentError(ValueError):
    pass


def identifier(value, qualified=False):
    # Qt uic keeps Unicode letters from objectName in generated identifiers and
    # MSVC/GCC accept them under C++11 with UTF-8 sources. Digits remain barred
    # from the first position, and punctuation is rejected.
    atom = r"[^\W\d]\w*"
    pattern = atom + (r"(?:::" + atom + r")*" if qualified else "")
    if not isinstance(value, str) or not re.fullmatch(pattern, value):
        raise ComponentError("INVALID_CPP_IDENTIFIER")
    return value


def literal(value, kind):
    if kind == "string-list" and isinstance(value, list) and all(
            isinstance(item, str) for item in value):
        return "QStringList{" + ", ".join(literal(item, "string") for item in value) + "}"
    if kind == "bool" and type(value) is bool:
        return "true" if value else "false"
    if kind == "int" and type(value) is int and -(2 ** 31) <= value < 2 ** 31:
        return str(value)
    if kind == "enum":
        return identifier(value, qualified=True)
    if kind == "string" and isinstance(value, str):
        # QString::fromUtf8(hex bytes, length) avoids encoding and escape
        # ambiguity, including quotes, embedded NUL and non-ASCII text.
        data = value.encode("utf-8")
        encoded = "".join("\\x%02x" % byte for byte in data)
        return 'QString::fromUtf8("%s", %d)' % (encoded, len(data))
    raise ComponentError("INVALID_TYPED_VALUE: " + kind)


def no_duplicate_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ComponentError("DUPLICATE_JSON_KEY: " + key)
        result[key] = value
    return result


def read_mapping(path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=no_duplicate_keys)


class ComponentEngine:
    def __init__(self, mapping, sdk_release, profile):
        if mapping.get("schemaVersion") != 1:
            raise ComponentError("COMPONENT_MAPPING_VERSION")
        if mapping.get("sdkRelease") != sdk_release:
            raise ComponentError("SDK_RELEASE_MISMATCH")
        if profile not in mapping.get("profiles", []):
            raise ComponentError("UNSUPPORTED_PROFILE")
        self.mapping = mapping
        self.rules = {}
        targets = set()
        self.initializers = []
        self.customwidgets = {}
        self.source_headers = set()
        for rule in mapping.get("components", []):
            source = rule["source"]
            key = (source["fileKey"], source["componentSetId"],
                   source["import"], source["tag"])
            if key in self.rules:
                raise ComponentError("DUPLICATE_SOURCE_IDENTITY")
            qt = rule["qt"]
            target = identifier(qt["class"], qualified=True)
            if target in targets:
                raise ComponentError("DUPLICATE_TARGET_IDENTITY")
            targets.add(target)
            identifier(qt["extends"], qualified=True)
            header = qt["header"]
            if (not isinstance(header, str)
                    or not re.fullmatch(r"[A-Za-z0-9_./-]+", header)
                    or PurePosixPath(header).is_absolute()
                    or ".." in PurePosixPath(header).parts):
                raise ComponentError("INVALID_PUBLIC_HEADER")
            if qt.get("constructor") != "parent-only":
                raise ComponentError("UIC_CONSTRUCTOR_NOT_VERIFIED")
            self.rules[key] = rule

    def emit(self, node, name):
        from generation_policy import export_first, skipped_props
        permissive = export_first(self.mapping)
        identifier(name)
        source = node["source"]
        key = (source.get("fileKey"), source.get("componentSetId"),
               source.get("import"), source.get("tag"))
        rule = self.rules.get(key)
        if rule is None:
            raise ComponentError("UNMAPPED_COMPONENT_IDENTITY: " + name)
        if rule.get("status") != "verified" and not (permissive and rule.get("status") == "partial"):
            raise ComponentError("COMPONENT_CONTRACT_NOT_VERIFIED: " + name)
        props = node.get("props", {})
        definitions = rule.get("props", {})
        if set(props) - set(definitions) and not permissive:
            raise ComponentError("UNMAPPED_COMPONENT_PROPS: " +
                                 ", ".join(sorted(set(props) - set(definitions))))
        slots = node.get("slots", {})
        slot_rules = rule.get("slots", {})
        if set(slots) - set(slot_rules):
            raise ComponentError("UNMAPPED_COMPONENT_SLOTS")
        calls = []
        receivers = {}
        # Icons must exist before a public icon-only mode setter is invoked.
        for slot, value in slots.items():
            spec = slot_rules[slot]
            if "phase" in spec:
                if spec["phase"] != "before-props" or spec.get("kind") != "text" or spec.get("type") != "string" or "receiver" in spec:
                    raise ComponentError("UNSUPPORTED_SLOT_PHASE")
                calls.append(self.call(name, spec, value))
                continue
            if spec.get("kind") != "icon":
                continue
            if set(spec) != {"kind", "method"} or not isinstance(value, dict):
                raise ComponentError("INVALID_ICON_CONTRACT")
            method = identifier(spec["method"])
            local = name + "_icon_" + identifier(slot)
            if set(value) == {"kind", "name"} and value["kind"] == "qt-standard" and value["name"] == "SP_FileIcon":
                expression = "QApplication::style()->standardIcon(QStyle::SP_FileIcon)"
                self.source_headers.update({"QApplication", "QStyle"})
            elif set(value) == {"kind", "path"} and value["kind"] == "resource" and isinstance(value["path"], str) and value["path"].startswith(":/pages/"):
                expression = "QIcon(" + literal(value["path"], "string") + ")"
            else:
                raise ComponentError("INVALID_ICON_VALUE")
            self.source_headers.add("QIcon")
            calls.extend(["const QIcon %s = %s;" % (local, expression),
                          'if (%s.isNull()) throw std::runtime_error("COMPONENT_ICON_UNAVAILABLE");' % local,
                          "ui->%s->%s(%s);" % (name, method, local)])
        if sum(spec.get("kind") == "table-model" for spec in slot_rules.values()) > 1:
            raise ComponentError("ONE_TABLE_MODEL_PER_COMPONENT")
        for prop, spec in definitions.items():
            if permissive and prop in skipped_props(rule, props):
                continue
            if prop in props:
                value = props[prop]
            elif "default" in spec:
                value = spec["default"]
            else:
                raise ComponentError("MISSING_COMPONENT_PROP: " + prop)
            if "cases" in spec:
                if set(spec) - {"cases", "default"}:
                    raise ComponentError("AMBIGUOUS_PROP_TRANSFORM: " + prop)
                if not isinstance(value, str) or value not in spec["cases"]:
                    raise ComponentError("UNSUPPORTED_PROP_VALUE: " + prop)
                operations = spec["cases"][value]
                if not isinstance(operations, list) or not operations:
                    raise ComponentError("EMPTY_PROP_TRANSFORM: " + prop)
                for operation in operations:
                    if set(operation) != {"method", "type", "value"}:
                        raise ComponentError("INVALID_PROP_OPERATION: " + prop)
                    calls.append(self.call(name, operation, operation["value"]))
                continue
            if "values" in spec:
                if not isinstance(value, str) or value not in spec["values"]:
                    raise ComponentError("UNSUPPORTED_PROP_VALUE: " + prop)
                value = spec["values"][value]
            calls.append(self.call(name, spec, value))
        for slot, value in slots.items():
            spec = slot_rules[slot]
            if spec.get("kind") == "icon" or spec.get("phase") == "before-props":
                continue
            if spec.get("kind") == "table-model":
                if spec != {"kind": "table-model", "method": "setModel", "readOnly": True}:
                    raise ComponentError("UNSUPPORTED_TABLE_MODEL_CONTRACT")
                if rule["qt"]["extends"] != "QTableView":
                    raise ComponentError("TABLE_VIEW_BASE_REQUIRED")
                calls.extend(self.table_model(name, value))
                self.source_headers.add("QStandardItemModel")
                continue
            if spec.get("kind") not in ("text", "string-list", "index"):
                raise ComponentError("UNSUPPORTED_SLOT_KIND: " + slot)
            required_type = {"text": "string", "string-list": "string-list", "index": "int"}
            if spec.get("type") != required_type[spec["kind"]]:
                raise ComponentError("SLOT_TYPE_MISMATCH: " + slot)
            receiver = None
            if "receiver" in spec:
                alias = identifier(spec["receiver"])
                child = rule.get("publicChildren", {}).get(alias)
                if not isinstance(child, dict) or set(child) != {"accessor", "class", "header"}:
                    raise ComponentError("PUBLIC_CHILD_CONTRACT_REQUIRED")
                accessor = identifier(child["accessor"])
                child_class = identifier(child["class"], qualified=True)
                header = child["header"]
                if (not isinstance(header, str) or not re.fullmatch(r"[A-Za-z0-9_./-]+", header)
                        or PurePosixPath(header).is_absolute() or ".." in PurePosixPath(header).parts):
                    raise ComponentError("INVALID_PUBLIC_HEADER")
                receiver = name + "_child_" + alias
                if alias not in receivers:
                    calls.extend([
                        "%s* %s = ui->%s->%s();" % (child_class, receiver, name, accessor),
                        'if (!%s) throw std::runtime_error("PUBLIC_CHILD_UNAVAILABLE");' % receiver])
                    receivers[alias] = receiver
                    self.source_headers.add(header)
            calls.append(self.call(name, spec, value, receiver=receiver))
        qt = rule["qt"]
        self.customwidgets[qt["class"]] = qt
        self.initializers.extend(calls)
        return ET.Element("widget", {"class": qt["class"], "name": name})

    @staticmethod
    def table_model(name, value):
        identifier(name)
        if not isinstance(value, dict) or set(value) != {"headers", "rows"}:
            raise ComponentError("TABLE_MODEL_DATA_REQUIRED")
        headers, rows = value["headers"], value["rows"]
        if (not isinstance(headers, list) or not headers or not isinstance(rows, list)
                or not all(isinstance(text, str) for text in headers)
                or any(not isinstance(row, list) or len(row) != len(headers)
                       or not all(isinstance(text, str) for text in row) for row in rows)):
            raise ComponentError("INVALID_TABLE_MODEL_MATRIX")
        model = name + "_model"
        calls = ["auto* %s = new QStandardItemModel(%d, %d, ui->%s);" %
                 (model, len(rows), len(headers), name),
                 "%s->setHorizontalHeaderLabels(%s);" % (model, literal(headers, "string-list"))]
        for r, row in enumerate(rows):
            for c, text in enumerate(row):
                item = "%s_item_%d_%d" % (name, r, c)
                calls += ["auto* %s = new QStandardItem(%s);" % (item, literal(text, "string")),
                          "%s->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);" % item,
                          "%s->setItem(%d, %d, %s);" % (model, r, c, item)]
        calls.append("ui->%s->setModel(%s);" % (name, model))
        return calls

    @staticmethod
    def call(name, spec, value, receiver=None):
        if "receiver" in spec and receiver is None:
            raise ComponentError("PUBLIC_CHILD_RECEIVER_ONLY_SUPPORTED_FOR_SLOTS")
        method = identifier(spec["method"])
        target = identifier(receiver) if receiver is not None else "ui->" + identifier(name)
        return "%s->%s(%s);" % (target, method, literal(value, spec["type"]))

    def declarations(self):
        result = ET.Element("customwidgets")
        for class_name, qt in sorted(self.customwidgets.items()):
            widget = ET.SubElement(result, "customwidget")
            ET.SubElement(widget, "class").text = class_name
            ET.SubElement(widget, "extends").text = qt["extends"]
            ET.SubElement(widget, "header").text = qt["header"]
        return result
