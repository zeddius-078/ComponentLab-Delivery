'use strict';
// Component anatomy remains data for one public SDK control, not child widgets.
// This extracts evidence only; resource URLs still require assets.py/QRC binding.
function collections(parent, records, definitions) {
  const result = {};
  for (const [name, spec] of Object.entries(definitions)) {
    const matches = records.filter(item =>
      item.source.instancePath.startsWith(parent.source.instancePath + '/') &&
      item.source.import === spec.source.import && item.source.tag === spec.source.tag &&
      item.visible !== false);
    if (!matches.length) throw new Error('EMPTY_COMPONENT_COLLECTION: ' + name);
    const scopes = new Set(), items = [], selected = [];
    for (const item of matches) {
      if (scopes.has(item.source.instancePath)) throw new Error('DUPLICATE_COLLECTION_INSTANCE');
      scopes.add(item.source.instancePath);
      // Do not flatten a recursively nested menu into its ancestor's item list.
      if (matches.some(other => other !== item &&
        item.source.instancePath.startsWith(other.source.instancePath + '/')))
        throw new Error('NESTED_COLLECTION_UNSUPPORTED');
      const state = item.declaredProps[spec.stateProperty];
      if (!Object.prototype.hasOwnProperty.call(spec.states, state))
        throw new Error('UNSUPPORTED_COLLECTION_STATE: ' + String(state));
      if (spec.states[state] !== true && spec.states[state] !== false)
        throw new Error('INVALID_COLLECTION_STATE_RULE');
      if (spec.states[state]) selected.push(items.length);
      if (item.content.text.length !== 1)
        throw new Error('COLLECTION_TEXT_SLOT_NOT_UNIQUE');
      if (item.content.images.length !== spec.imageCount)
        throw new Error('COLLECTION_IMAGE_COUNT_MISMATCH');
      if ((item.conflicts || []).length) throw new Error('COLLECTION_CONTENT_CONFLICT');
      items.push({source: item.source, text: item.content.text[0],
        images: item.content.images, declaredProps: item.declaredProps});
    }
    if (selected.length !== 1) throw new Error('COLLECTION_SELECTION_NOT_UNIQUE');
    result[name] = {items, selectedIndex: selected[0], labels: items.map(item => item.text.text),
      resourceStatus: spec.imageCount ? 'requires-qrc-binding' : 'not-applicable',
      identityStatus: 'anatomy candidates; parent component contract still required'};
  }
  return result;
}
module.exports = {collections};
