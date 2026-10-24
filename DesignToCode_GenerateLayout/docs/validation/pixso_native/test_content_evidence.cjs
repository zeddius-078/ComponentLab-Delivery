'use strict';
const assert = require('node:assert/strict');
const {evidence} = require('../../../Source/pixso_native/content_evidence.cjs');
let count = 0;
const source = {instancePath: 'root/a', import: 'Fixture.vue', tag: 'Fixture'};
const mapping = {components: [{source, contentExpectations: {
  property: 'Content', values: {'icon-only': {text: false, images: 1},
    'text-only': {text: true, images: 0}}
}}]};
function input(content, children) {
  return {componentTree: {kind: 'component', source, props: {Content: content}, children: []},
    styledTree: {kind: 'element', visible: true, componentBoundaries: [source], children}};
}
const text = {kind: 'text', text: 'Label', source: {file: 'Fixture.vue', offset: 10}};
assert.equal(evidence(input('icon-only', [text]), mapping)[0].conflicts[0].code,
  'DECLARED_CONTENT_CONFLICT'); count++;
assert.equal(evidence(input('text-only', [text]), mapping)[0].conflicts.length, 0); count++;
const icon = {kind: 'element', visible: true, style: {
  values: {'background-image': 'url(@/icon.svg)'},
  origins: {'background-image': {file: 'Fixture.vue', line: 20}}
}, children: []};
assert.equal(evidence(input('icon-only', [icon]), mapping)[0].conflicts.length, 0); count++;
assert.equal(evidence(input('icon-only', [{...icon, visible: false}]), mapping)[0].content.images.length, 0); count++;
assert.equal(evidence(input('other', []), mapping)[0].conflicts[0].code,
  'UNSUPPORTED_CONTENT_VALUE'); count++;
assert.equal(evidence(input('text-only', [text]), mapping)[0].content.text[0].source.offset, 10); count++;
console.log(JSON.stringify({status: '通过', tests: count, scope: 'declared vs visible content evidence'}));
