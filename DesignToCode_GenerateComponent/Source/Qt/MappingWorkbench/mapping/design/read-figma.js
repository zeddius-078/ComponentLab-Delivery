// Execute through Figma MCP use_figma after supplying IDs from the approved ledger.
// This script is read-only. It extracts at most 64 layout/instance nodes.
const request = {pageId: "127:2710", rootId: "REPLACE_WITH_ASSEMBLY_FRAME_ID"};
const page = await figma.getNodeByIdAsync(request.pageId);
if (!page || page.type !== "PAGE") throw new Error("Page not found");
await figma.setCurrentPageAsync(page);
const root = await figma.getNodeByIdAsync(request.rootId);
if (!root || root.type !== "FRAME" || root.parent?.id !== page.id)
    throw new Error("Choose a top-level assembly frame on the registered page");
const collection = await figma.variables.getVariableCollectionByIdAsync("VariableCollectionId:131:2711");
const modeId = root.resolvedVariableModes[collection.id];
const profile = collection.modes.find(mode => mode.modeId === modeId)?.name;
if (!profile) throw new Error("Assembly Profile not resolved");
let count = 0;
async function collect(node, depth = 0) {
    if (++count > 64 || depth > 8) throw new Error("Snapshot exceeds node/depth limits");
    const result = {id: node.id, name: node.name, type: node.type};
    if (node.type === "FRAME") {
        result.layoutMode = node.layoutMode;
        for (const key of ["itemSpacing", "paddingLeft", "paddingRight", "paddingTop", "paddingBottom"])
            result[key] = node[key];
        result.boundVariables = node.boundVariables;
        result.absoluteChildren = node.children.some(child => child.layoutPositioning === "ABSOLUTE");
        result.children = [];
        for (const child of node.children) {
            if (!child.visible) throw new Error("Hidden layout children require an explicit rule: " + child.id);
            result.children.push(await collect(child, depth + 1));
        }
    } else if (node.type === "INSTANCE") {
        const component = await node.getMainComponentAsync();
        if (!component) throw new Error("Detached/missing main component: " + node.id);
        result.mainComponent = {id: component.id, key: component.key, name: component.name};
        result.componentProperties = node.componentProperties;
        result.iconComponents = {};
        for (const property of Object.values(node.componentProperties)) {
            if (property.type !== "INSTANCE_SWAP") continue;
            const icon = await figma.getNodeByIdAsync(property.value);
            if (!icon || icon.type !== "COMPONENT") throw new Error("Missing icon main component");
            result.iconComponents[property.value] = {id: icon.id, key: icon.key, name: icon.name};
        }
        const propertyFields = new Set(["characters", "visible", "mainComponent", "componentProperties"]);
        result.visualOverrides = node.overrides.flatMap(override =>
            override.overriddenFields.filter(field => !propertyFields.has(field))
                .map(field => ({id: override.id, field})));
    } else {
        throw new Error("Unsupported design node: " + node.type + " " + node.id);
    }
    return result;
}
return {schemaVersion: 1, provider: "figma", fileKey: "w92BKW1EcutKmTXkwzNHbf",
        profile, root: await collect(root)};
