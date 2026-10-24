'use strict';
const assert = require('node:assert/strict');
const {parseTemplate, TemplateSyntaxError} = require('../../../Source/pixso_native/template_syntax.cjs');
let count = 0;
const tree = parseTemplate(`<ComponentLabButton Intent="primary" :visible="false">
<template #label><p>{{ "quoted }} and <" }}</p></template>
</ComponentLabButton>`);
assert.equal(tree.children[0].tag, 'ComponentLabButton'); count++;
assert.equal(tree.children[0].attrs[1].value, 'false'); count++;
assert.equal(tree.children[0].children[1].attrs[0].name, '#label'); count++;
assert.equal(tree.children[0].children[1].children[0].children[0].expression,
  '"quoted }} and <"'); count++;
const fragments = parseTemplate('<div/><span/>');
assert.equal(fragments.children.length, 2); count++;
assert.equal(parseTemplate('<img src="a.svg"><br>').children.length, 2); count++;
assert.equal(parseTemplate('{{ ({key: "}"}) }}').children[0].expression, '({key: "}"})'); count++;
for (const input of ['<div></span>', '<div>', '<div a="1" a="2"/>',
  '<div a=1/>', '{{ unfinished', '<!-- unfinished', '{{ }}', '<!doctype html>']) {
  assert.throws(() => parseTemplate(input), TemplateSyntaxError); count++;
}
console.log(JSON.stringify({status: '通过', tests: count,
  scope: 'bounded syntax only; no Vue runtime or full template semantic equivalence'}));
