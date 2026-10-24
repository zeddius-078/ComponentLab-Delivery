'use strict';
const assert = require('node:assert/strict');
const {createRequire} = require('node:module');
const path = require('node:path');
const {VueGraph} = require('../../../Source/pixso_native/vue_graph.cjs');
const {parseTemplate} = require('../../../Source/pixso_native/template_syntax.cjs');
const modules = process.argv[2];
if (!modules || !path.isAbsolute(modules)) throw new Error('ABSOLUTE_TOOL_DIRECTORY_REQUIRED');
const external = createRequire(path.join(modules, '__fixture_loader__.cjs'));
function moduleRecord(file, template, props = {}, imports = {}) {
  return {file, template: parseTemplate(template), props, imports, sha256: 'fixture'};
}
function render(records, supplied = {}) {
  const graph = new VueGraph(__dirname, {babel: external('@babel/parser')});
  graph.load = file => { if (!records[file]) throw new Error('MISSING_FIXTURE'); return records[file]; };
  return graph.component('Root.vue', supplied, {}, 'root');
}
function all(tree, kind) {
  return [...(tree.kind === kind ? [tree] : []), ...(tree.children || []).flatMap(c => all(c, kind))];
}
function text(tree) {return all(tree, 'text').map(n => n.text).join('');}
let count = 0;
const records = {
  'Root.vue': moduleRecord('Root.vue',
    '<Box id="a" label="one"><template #content>{{ label }}</template></Box><Box id="b" label="two"/>',
    {label: {type: 'String', hasDefault: true, default: 'parent'}}, {Box: 'Box.vue'}),
  'Box.vue': moduleRecord('Box.vue', '<div id="shared"><slot name="content">{{ label }}</slot></div>',
    {label: {type: 'String', hasDefault: true, default: 'default'}})
};
const result = render(records);
assert.equal(text(result), 'parenttwo'); count++; // slot evaluates in parent lexical scope
assert.equal(result.children[0].children[0].children[0].slotTrace[0].overridden, true); count++;
const boxes = all(result, 'component').filter(n => n.source.file === 'Box.vue');
assert.deepEqual(boxes.map(n => n.source.instancePath), ['root/a', 'root/b']); count++;
assert.deepEqual(all(result, 'element').map(n => n.attrs.id), ['shared', 'shared']); count++;
const branch = {'Root.vue': moduleRecord('Root.vue',
  '<div v-if="Visible">{{ Value ?? \'fallback\' }}</div><p v-if="!Visible">off</p>',
  {Visible: {type: 'Boolean', hasDefault: false}, Value: {type: 'String', hasDefault: false}})};
assert.equal(text(render(branch)), 'off'); count++;
assert.equal(text(render(branch, {Visible: true, Value: ''})), ''); count++;
assert.equal(text(render(branch, {Visible: true})), 'fallback'); count++;
assert.throws(() => render(branch, {Visible: 'true'}), /PROP_TYPE_MISMATCH/); count++;
assert.throws(() => render({'Root.vue': moduleRecord('Root.vue', '<Unknown/>')}), /UNRESOLVED_ELEMENT/); count++;
assert.throws(() => render({'Root.vue': moduleRecord('Root.vue', '<div v-for="x in items"/>')}),
  /UNSUPPORTED_DIRECTIVE/); count++;
assert.throws(() => render({'Root.vue': moduleRecord('Root.vue', '<Self/>', {}, {Self: 'Root.vue'})}),
  /RECURSIVE_COMPONENT/); count++;
assert.equal(text(render({'Root.vue': moduleRecord('Root.vue', '<p>A &amp; B &#x4e2d;</p>')})),
  'A & B 中'); count++;
assert.deepEqual(render(records), render(records)); count++;
const repeated = {
  'Root.vue': moduleRecord('Root.vue',
    '<Host><template #same><Leaf id="same-id"/></template></Host>', {},
    {Host: 'Host.vue', Leaf: 'Leaf.vue'}),
  'Host.vue': moduleRecord('Host.vue', '<slot name="same"/><slot name="same"/>'),
  'Leaf.vue': moduleRecord('Leaf.vue', '<p>leaf</p>')
};
const leaves = all(render(repeated), 'component').filter(n => n.source.file === 'Leaf.vue');
assert.equal(leaves.length, 2);
assert.notEqual(leaves[0].source.instancePath, leaves[1].source.instancePath); count++;
const undeclared = {'Root.vue': moduleRecord('Root.vue',
  '<div v-if="Hidden">hidden</div><p>{{ Missing ?? \'fallback\' }}</p>')};
assert.equal(text(render(undeclared)), 'fallback'); count++;
console.log(JSON.stringify({status: '通过', tests: count, scope: 'static component/slot subset'}));
