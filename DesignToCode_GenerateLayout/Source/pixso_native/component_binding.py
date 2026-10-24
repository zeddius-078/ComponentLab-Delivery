"""Join static instance records to mapping candidates without inventing identity.

Matching a module/tag only locates a candidate. Generation additionally requires
the caller's source-file identity, the exact module hash and a verified rule.
Props and slot values are passed unchanged to the existing component engine.
"""
from component_engine import ComponentError
from generation_policy import export_first, skipped_props


def index_instances(tree):
    result = {}

    def visit(node):
        if node.get("kind") == "component":
            scope = node.get("source", {}).get("instancePath")
            if not scope or scope in result:
                raise ComponentError("INSTANCE_SCOPE_NOT_UNIQUE")
            result[scope] = node
        for child in node.get("children", []):
            visit(child)
    visit(tree)
    return result


class ComponentBinding:
    def __init__(self, mapping, tree, file_key=None):
        self.export_first = export_first(mapping)
        self.instances = index_instances(tree)
        self.file_key = file_key
        self.candidates = {}
        for rule in mapping.get("components", []):
            source = rule["source"]
            key = (source["import"], source["tag"])
            self.candidates.setdefault(key, []).append(rule)

    def inspect(self, boundary):
        scope = boundary.get("instancePath")
        instance = self.instances.get(scope)
        if instance is None:
            raise ComponentError("MISSING_INSTANCE_SCOPE")
        source = instance["source"]
        # A styled root must refer to the same parsed module, not merely to a
        # similarly named node or a repeated internal DOM ID.
        for key in ("file", "sha256", "import", "tag"):
            if boundary.get(key) != source.get(key):
                raise ComponentError("INSTANCE_BOUNDARY_MISMATCH: " + key)
        candidates = self.candidates.get((source.get("import"), source.get("tag")), [])
        if self.file_key:
            candidates = [rule for rule in candidates
                          if rule["source"]["fileKey"] == self.file_key]
        issues = []
        report = {"source": dict(source), "props": dict(instance.get("props", {})),
                  "mappingKey": None, "issues": issues, "generationReady": False}
        if len(candidates) != 1:
            issues.append("UNMAPPED_COMPONENT_MODULE" if not candidates
                          else "AMBIGUOUS_COMPONENT_MODULE")
            return report
        rule = candidates[0]
        report["mappingKey"] = rule.get("mappingKey")
        report["candidateIdentity"] = {
            key: rule["source"][key] for key in ("fileKey", "componentSetId")}
        if not self.file_key:
            issues.append("SOURCE_FILE_IDENTITY_REQUIRED")
        expected_hash = rule["source"].get("moduleSha256")
        if not expected_hash:
            issues.append("SOURCE_MODULE_BINDING_UNVERIFIED")
        elif expected_hash != source.get("sha256"):
            issues.append("SOURCE_MODULE_HASH_MISMATCH")
        if rule.get("status") != "verified":
            issues.append("COMPONENT_CONTRACT_NOT_VERIFIED")
        props = instance.get("props", {})
        definitions = rule.get("props", {})
        report["unmappedProps"] = sorted(set(props) - set(definitions))
        if report["unmappedProps"]:
            issues.append("UNMAPPED_COMPONENT_PROPS")
        report["missingProps"] = sorted(
            key for key, spec in definitions.items()
            if key not in props and "default" not in spec)
        if report["missingProps"]:
            issues.append("MISSING_COMPONENT_PROP")
        unsupported = []
        for key, spec in definitions.items():
            value = props.get(key, spec.get("default"))
            choices = spec.get("cases", spec.get("values"))
            if choices is not None and (not isinstance(value, str) or value not in choices):
                unsupported.append(key)
        report["unsupportedPropValues"] = sorted(unsupported)
        if unsupported:
            issues.append("UNSUPPORTED_PROP_VALUE")
        report["warnings"] = []
        if self.export_first:
            permitted = {"UNMAPPED_COMPONENT_PROPS", "MISSING_COMPONENT_PROP",
                         "UNSUPPORTED_PROP_VALUE"}
            if rule.get("status") == "partial":
                permitted.add("COMPONENT_CONTRACT_NOT_VERIFIED")
            report["warnings"] = [issue for issue in issues if issue in permitted]
            issues[:] = [issue for issue in issues if issue not in permitted]
            report["unappliedProperties"] = [
                {"name": key, "present": key in props, "value": props.get(key)}
                for key in skipped_props(rule, props)]
        # Slots/resources/layout are separate contracts. This report can never
        # establish whole-component readiness by a module/prop match alone.
        report["bindingReady"] = not issues
        report["slotsStatus"] = "requires-explicit-resolution"
        return report

    def bind(self, boundary, slots):
        report = self.inspect(boundary)
        if report["issues"]:
            raise ComponentError("COMPONENT_BINDING_FAILED: " + ", ".join(report["issues"]))
        if not isinstance(slots, dict):
            raise ComponentError("RESOLVED_SLOTS_REQUIRED")
        source = dict(report["source"])
        source.update(report["candidateIdentity"])
        return {"source": source, "props": dict(report["props"]), "slots": dict(slots)}


def inspect_boundaries(tree, boundaries, mapping, file_key=None):
    binding = ComponentBinding(mapping, tree, file_key)
    return [binding.inspect(boundary)
            for item in boundaries for boundary in item["boundaries"]]


def bind_text_slots(record, definitions):
    """Resolve visible text by the owning component's scoped slot, not DOM ID."""
    slots, consumed = {}, []
    for target, selector in definitions.items():
        candidates = []
        for text in record["content"]["text"]:
            if any(trace.get("instancePath") == record["source"]["instancePath"]
                   and trace.get("file") == selector["file"]
                   and trace.get("name") in selector["slots"]
                   for trace in text.get("slots", [])):
                candidates.append(text)
        if len(candidates) != 1:
            raise ComponentError("TEXT_SLOT_NOT_UNIQUE: " + target)
        if candidates[0] in consumed:
            raise ComponentError("TEXT_SLOT_CONSUMED_TWICE")
        slots[target] = candidates[0]["text"]
        consumed.append(candidates[0])
    return slots, consumed


def bind_table(record, definition):
    """Read an explicitly mapped matrix; never infer columns from text order."""
    if not definition or not definition.get("headers") or not definition.get("rows"):
        raise ComponentError("TABLE_MATRIX_REQUIRED")
    for prop, value in definition.get("when", {}).items():
        if record.get("declaredProps", {}).get(prop) != value:
            raise ComponentError("TABLE_VARIANT_UNSUPPORTED")
    width = len(definition["headers"])
    if any(len(row) != width for row in definition["rows"]):
        raise ComponentError("TABLE_MATRIX_NOT_RECTANGULAR")
    selectors = {}
    matrices = [definition["headers"], *definition["rows"]]
    for row, cells in enumerate(matrices):
        for column, slot in enumerate(cells):
            if not isinstance(slot, str):
                raise ComponentError("TABLE_SLOT_NAME_REQUIRED")
            selectors["%s_%s" % (row, column)] = {
                "file": definition["file"], "slots": [slot]}
    values, consumed = bind_text_slots(record, selectors)
    matrix = [[values["%s_%s" % (row, column)] for column in range(width)]
              for row in range(len(matrices))]
    return {"headers": matrix[0], "rows": matrix[1:]}, consumed


def bind_text_lists(record, definitions):
    """Bind an explicitly ordered set of scoped text slots to a public list API."""
    selectors, order = {}, {}
    for target, definition in definitions.items():
        names = definition.get("slots")
        if not isinstance(names, list) or not names or not all(isinstance(s, str) for s in names):
            raise ComponentError("ORDERED_TEXT_SLOTS_REQUIRED")
        order[target] = []
        for slot in names:
            key = str(len(selectors))
            selectors[key] = {"file": definition["file"], "slots": [slot]}
            order[target].append(key)
    values, consumed = bind_text_slots(record, selectors)
    return {target: [values[key] for key in keys] for target, keys in order.items()}, consumed
