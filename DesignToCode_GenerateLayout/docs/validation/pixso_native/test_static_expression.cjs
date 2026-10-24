'use strict';
const assert = require('node:assert/strict');
const path = require('node:path');
const { createRequire } = require('node:module');
const { resolve, StaticExpressionError } = require(
  '../../../Source/pixso_native/static_expression.cjs');

// Explicit existing parser location; never npm install or silently select a
// different runtime. This CLI tests syntax interpretation, not a Vue renderer.
const modules = process.argv[2];
if (!modules || !path.isAbsolute(modules)) throw new Error('ABSOLUTE_NODE_MODULES_REQUIRED');
const external = createRequire(path.join(modules, '__native_parser_loader__.cjs'));
const parser = external('@babel/parser');
let count = 0;
function check(expression, scope, expected) {
  assert.deepEqual(resolve(expression, scope, parser), expected);
  count++;
}
check("Intent === 'primary' && Appearance === 'filled' && Size === 'small'",
  {Intent: 'primary', Appearance: 'filled', Size: 'small'}, true);
check("Intent === 'primary' && missing", {Intent: 'secondary'}, false);
check("text_339_91 ?? 'Input text'", {text_339_91: ''}, '');
check("text_339_91 ?? 'Input text'", {text_339_91: null}, 'Input text');
check('visible_339_92 ?? true', {visible_339_92: false}, false);
check("props.Size === 'Large' ? 'large' : 'small'", {props: {Size: 'Large'}}, 'large');
check('!disabled', {disabled: false}, true);
check('undefined ?? 0', {}, 0);
check('true || unknown', {}, true);
check('(size as string) === "Small"', {size: 'Small'}, true);
for (const expression of [
  'run()', 'globalThis.process', 'props.constructor',
  'props["__proto__"]', 'new Function("return 1")', 'missing',
  'props.missing', '1 == "1"', '(() => 1)()', 'props.x = 3'
]) {
  assert.throws(() => resolve(expression, {props: {}}, parser), StaticExpressionError);
  count++;
}
console.log(JSON.stringify({
  status: '通过', tests: count,
  scope: 'static expression subset only; no Vue runtime execution',
  parserVersion: external('@babel/parser/package.json').version
}));
