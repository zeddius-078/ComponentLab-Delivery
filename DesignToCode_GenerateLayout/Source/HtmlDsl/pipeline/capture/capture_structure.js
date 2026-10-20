// Read-only Pixso Plugin API script. Supply fileKey/rootId through the caller.
// This is a resolved scene snapshot, NOT canonical DSL or a formal capture.
const expectedFileKey = __FILE_KEY__;
const rootId = __ROOT_ID__;
if (pixso.fileKey !== expectedFileKey) throw Error("STRUCTURE_WRONG_FILE");
const root = pixso.getNodeById(rootId);
if (!root) throw Error("STRUCTURE_ROOT_MISSING");
const fields = [
  "id", "type", "name", "key", "visible", "x", "y", "width", "height",
  "rotation", "opacity", "layoutMode", "itemSpacing",
  "paddingLeft", "paddingRight", "paddingTop", "paddingBottom",
  "primaryAxisAlignItems", "counterAxisAlignItems", "clipsContent",
  "fills", "strokes", "effects", "cornerRadius", "characters", "fontName",
  "fontSize", "textAlignHorizontal", "componentProperties", "variantProperties"
];
function snapshot(node) {
  const result = {};
  for (const field of fields) {
    const value = node[field];
    if (value !== undefined && typeof value !== "symbol") result[field] = value;
  }
  if (node.type === "INSTANCE") {
    const master = node.mainComponent;
    result.mainComponent = master ? {
      id: master.id, key: master.key, name: master.name,
      familyId: master.parent && master.parent.type === "COMPONENT_SET"
        ? master.parent.id : master.id
    } : null;
  }
  if (node.children) result.children = node.children.map(snapshot);
  return result;
}
return {
  schema: "design-structure/v1",
  provenance: "pixso-plugin-resolved-scene",
  fileKey: pixso.fileKey,
  fileName: pixso.root.name,
  capturedAt: new Date().toISOString(),
  root: snapshot(root)
};
