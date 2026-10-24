'use strict';
const assert = require('node:assert/strict');
const path = require('node:path');
const {createRequire} = require('node:module');
const {Cascade, substitute} = require('../../../Source/pixso_native/css_cascade.cjs');
const modules = process.argv[2];
if (!modules || !path.isAbsolute(modules)) throw new Error('ABSOLUTE_TOOL_DIRECTORY_REQUIRED');
const external = createRequire(path.join(modules, '__fixture_loader__.cjs'));
const postcss = external('postcss');
let count = 0;
const cascade = new Cascade(postcss);
cascade.add(`
:root { --base: #123456; --alias: var(--base); color: var(--alias); }
.host .item { display: none; }
.host > .item { color: red; }
.item { display: block; color: blue !important; }
[data-mode="dark"] { --base: #000000; }
#chosen { display: flex; }
`, 'fixture.css');
const html = {tag: 'html', attrs: {}};
const base = cascade.resolve(html, []);
assert.equal(base.values.color, '#123456'); count++;
const host = {tag: 'div', attrs: {class: 'host', 'data-mode': 'dark'}};
const dark = cascade.resolve(host, [html], base);
assert.equal(dark.variables['--base'], '#000000'); count++;
assert.equal(dark.variables['--alias'], '#123456'); count++; // computed before inheritance
const child = {tag: 'div', attrs: {class: 'item', style: 'color: green'}};
const result = cascade.resolve(child, [html, host], dark);
assert.equal(result.values.display, 'none'); count++;
assert.equal(result.values.color, 'blue'); count++; // stylesheet !important beats ordinary inline
assert.equal(result.origins.display.selector, '.host .item'); count++;
child.attrs.id = 'chosen';
assert.equal(cascade.resolve(child, [html, host], dark).values.display, 'flex'); count++;
child.attrs.style = 'color: green !important';
assert.equal(cascade.resolve(child, [html, host], dark).values.color, 'green'); count++;
assert.equal(substitute('var(--a, var(--b, 4px))', {}), '4px'); count++;
assert.equal(substitute('calc(var(--a) + 2px)', {'--a': '3px'}), 'calc(3px + 2px)'); count++;
assert.throws(() => substitute('var(--missing)', {}), /UNRESOLVED_CSS_VAR/); count++;
assert.throws(() => substitute('var(--a)', {'--a': 'var(--b)', '--b': 'var(--a)'}), /CYCLIC/); count++;
const direct = new Cascade(postcss);
direct.add('.host > .item {display:none}', 'fixture.css');
assert.equal(direct.resolve(child, [html, host, {tag: 'div', attrs: {}}]).values.display, undefined); count++;
const pseudo = new Cascade(postcss);
pseudo.add('ul li::before {content:"x"}', 'fixture.css');
assert.throws(() => pseudo.resolve({tag: 'li'}, [{tag: 'ul'}]), /PSEUDO/); count++;
assert.doesNotThrow(() => pseudo.resolve({tag: 'div'}, [])); count++;
assert.throws(() => direct.add('@media (min-width: 1px) {div{display:none}}', 'fixture.css'),
  /UNSUPPORTED_CSS_RULE/); count++;
function box(css, style) {
  const c = new Cascade(postcss);
  c.add(css, 'box.css');
  return c.resolve({tag: 'div', attrs: {class: 'box', style}}, []);
}
assert.deepEqual(box('.box {padding: 1px 2px 3px 4px}').values,
  {'padding-top': '1px', 'padding-right': '2px',
    'padding-bottom': '3px', 'padding-left': '4px'}); count++;
assert.equal(box('.box {padding-left: 9px; padding: 2px}').values['padding-left'], '2px'); count++;
assert.equal(box('.box {padding: 2px; padding-left: 9px}').values['padding-left'], '9px'); count++;
assert.equal(box('.box {padding-left: 9px !important; padding: 2px}').values['padding-left'], '9px'); count++;
assert.equal(box('.box {padding: 2px !important; padding-left: 9px}').values['padding-left'], '2px'); count++;
assert.equal(box('div.box {padding-left: 9px}.box {padding: 2px}').values['padding-left'], '9px'); count++;
assert.equal(box('.box {padding-left: 9px}', 'padding: 2px').values['padding-left'], '2px'); count++;
assert.deepEqual(box('.box {--p:1px 2px 3px; padding:var(--p)}').values,
  {'padding-top': '1px', 'padding-right': '2px',
    'padding-bottom': '3px', 'padding-left': '2px'}); count++;
assert.equal(box('.box {margin: 0 auto}').values['margin-right'], 'auto'); count++;
assert.equal(box('.box {padding: calc(2px + 3px) 4px}').values['padding-top'],
  'calc(2px + 3px)'); count++; // retained, not numerically evaluated
assert.throws(() => box('.box {padding: 1px 2px 3px 4px 5px}'), /BOX_SHORTHAND/); count++;
console.log(JSON.stringify({status: '通过', tests: count, scope: 'declared CSS cascade, not browser layout'}));
