'use strict';
const assert = require('node:assert/strict');
const path = require('node:path');
const {createRequire} = require('node:module');
const {modeAssignments, materialize} = require('../../../Source/pixso_native/style_graph.cjs');
const modules = process.argv[2];
if (!modules || !path.isAbsolute(modules)) throw new Error('ABSOLUTE_TOOL_DIRECTORY_REQUIRED');
const external = createRequire(path.join(modules, '__fixture_loader__.cjs'));
const parser = external('@babel/parser');
let count = 0;
assert.deepEqual(modeAssignments(`
document.addEventListener('DOMContentLoaded', function() {
 const element = document.getElementById('id');
 if (element) { element.setAttribute('data-test-mode', 'dark'); }
});`, parser), [{id: 'id', attribute: 'data-test-mode', value: 'dark'}]); count++;
for (const script of [
  'run();',
  `document.addEventListener('DOMContentLoaded',()=>{fetch("https://example.test");});`,
  `document.addEventListener('DOMContentLoaded',()=>{const x=document.getElementById('id');
   if(x){x.setAttribute('onclick','execute()');}});`
]) {
  assert.throws(() => modeAssignments(script, parser), /UNSUPPORTED_MODE/); count++;
}
const source = {file: 'Fixture.vue', instancePath: 'root/a'};
const component = {kind: 'component', source, props: {Size: 'small'},
  attrs: {Size: 'small', id: 'instance', class: 'outer', style: 'color:red'},
  children: [{kind: 'element', tag: 'div', attrs: {id: 'internal', class: 'inner',
    style: 'color:blue'}, children: []}]};
const [dom] = materialize(component);
assert.equal(dom.attrs.id, 'instance'); count++;
assert.equal(dom.attrs.class, 'inner outer'); count++;
assert.equal(dom.attrs.style, 'color:blue;color:red'); count++;
assert.equal(dom.attrs.Size, undefined); count++;
assert.equal(component.children[0].attrs.id, 'internal'); count++;
assert.deepEqual(dom.componentBoundaries, [source]); count++;
const fragment = {...component, children: [component.children[0], component.children[0]]};
assert.throws(() => materialize(fragment), /FRAGMENT_ATTRIBUTE/); count++;
console.log(JSON.stringify({status: '通过', tests: count, scope: 'mode extraction and component fallthrough'}));
