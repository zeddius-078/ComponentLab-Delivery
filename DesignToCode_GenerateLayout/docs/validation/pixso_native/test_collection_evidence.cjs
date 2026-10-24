'use strict';
const assert = require('node:assert/strict');
const {collections} = require('../../../Source/pixso_native/collection_evidence.cjs');
const parent = {source: {instancePath: 'root/nav'}};
const definitions = {primary: {source: {import: 'Item.vue', tag: 'Item'},
  stateProperty: 'State', states: {Default: false, Selected: true}, imageCount: 1}};
function item(id, text, state = 'Default') {
  return {source: {instancePath: 'root/nav/' + id, import: 'Item.vue', tag: 'Item'},
    visible: true, declaredProps: {State: state}, content: {
      text: [{text, source: {file: 'Page.vue', offset: 1}}],
      images: [{reference: 'url(icon.svg)', source: {file: 'Page.vue', line: 2}}]},
    conflicts: []};
}
const records = [item('one', '首页'), item('two', '&nbsp;', 'Selected'), item('three', '设置')];
let count = 0;
const result = collections(parent, records, definitions).primary;
assert.deepEqual(result.labels, ['首页', '&nbsp;', '设置']); count++;
assert.equal(result.selectedIndex, 1); count++;
assert.equal(result.resourceStatus, 'requires-qrc-binding'); count++;
assert.equal(result.items[0].text.source.file, 'Page.vue'); count++;
const unrelated = {...item('other', 'wrong'), source: {
  instancePath: 'root/other/item', import: 'Item.vue', tag: 'Item'}};
assert.equal(collections(parent, [...records, unrelated], definitions).primary.items.length, 3); count++;
for (const [input, error] of [
  [[item('one', 'x')], /SELECTION_NOT_UNIQUE/],
  [[item('one', 'x', 'Selected'), item('two', 'y', 'Selected')], /SELECTION_NOT_UNIQUE/],
  [[item('one', 'x', 'Disabled')], /UNSUPPORTED_COLLECTION_STATE/],
  [[records[0], records[0]], /DUPLICATE_COLLECTION_INSTANCE/],
  [[item('one', 'x'), item('one/nested', 'y', 'Selected')], /NESTED_COLLECTION_UNSUPPORTED/],
  [[{...records[1], content: {text: [], images: []}}], /TEXT_SLOT_NOT_UNIQUE/],
  [[{...records[1], content: {text: records[1].content.text, images: []}}], /IMAGE_COUNT_MISMATCH/],
  [[{...records[1], conflicts: [{code: 'fixture'}]}], /CONTENT_CONFLICT/],
  [[], /EMPTY_COMPONENT_COLLECTION/]
]) {
  assert.throws(() => collections(parent, input, definitions), error); count++;
}
console.log(JSON.stringify({status: '通过', tests: count, scope: 'ordered anatomy collection evidence'}));
