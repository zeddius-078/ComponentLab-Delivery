'use strict';
// Read-only source inventory. Vue 2's parseComponent is used ONLY to delimit
// SFC blocks; never compileTemplate/compileScript or execute application code.
// Template semantics remain explicitly unverified by this command.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {createRequire} = require('node:module');
const {evaluate} = require('./static_expression.cjs');
const {parseTemplate} = require('./template_syntax.cjs');

function propertyName(prop) {
  if (prop.computed || prop.type !== 'ObjectProperty')
    throw new Error('UNSUPPORTED_PROP_DECLARATION');
  return prop.key.type === 'Identifier' ? prop.key.name : prop.key.value;
}

function inspect(source, filename, tools) {
  const sfc = tools.sfc.parseComponent(source, {pad: false, outputSourceRange: true});
  if (sfc.errors && sfc.errors.length) throw new Error(JSON.stringify(sfc.errors));
  const imports = [], props = {}, unresolved = [];
  for (const block of [sfc.script, sfc.scriptSetup].filter(Boolean)) {
    const ast = tools.babel.parse(block.content, {sourceType: 'module', plugins: ['typescript']});
    function walk(node) {
      if (!node || typeof node !== 'object') return;
      if (node.type === 'ImportDeclaration') {
        imports.push({from: node.source.value, bindings: node.specifiers.map(s => ({
          local: s.local.name, kind: s.type,
          imported: s.imported ? (s.imported.name || s.imported.value) : null
        }))});
      }
      if (node.type === 'CallExpression' && node.callee.type === 'Identifier'
          && node.callee.name === 'defineProps') {
        const arg = node.arguments[0];
        if (!arg || arg.type !== 'ObjectExpression') {
          unresolved.push('defineProps requires object declaration for this adapter');
        } else {
          for (const prop of arg.properties) {
            const name = propertyName(prop);
            if (Object.prototype.hasOwnProperty.call(props, name)) throw new Error('DUPLICATE_PROP');
            const entry = {hasDefault: false};
            if (prop.value.type !== 'ObjectExpression') {
              unresolved.push(`prop ${name}: shorthand/type declaration not yet lowered`);
            } else for (const member of prop.value.properties) {
              const key = propertyName(member);
              if (key === 'type' && member.value.type === 'Identifier') entry.type = member.value.name;
              else if (key === 'default') {
                try {
                  entry.default = evaluate(member.value, {});
                  entry.hasDefault = true;
                } catch (error) {
                  unresolved.push(`prop ${name} default: ${error.message}`);
                }
              } else if (key === 'required') {
                entry.required = evaluate(member.value, {});
              } else unresolved.push(`prop ${name}: unsupported field ${key}`);
            }
            props[name] = entry;
          }
        }
      }
      for (const [key, value] of Object.entries(node)) {
        if (['loc', 'start', 'end', 'extra', 'comments', 'tokens'].includes(key)) continue;
        if (Array.isArray(value)) value.forEach(walk);
        else if (value && typeof value === 'object') walk(value);
      }
    }
    walk(ast.program);
  }
  const styles = sfc.styles.map(block => {
    const css = tools.postcss.parse(block.content, {from: filename});
    const declarations = {};
    let rules = 0;
    css.walkRules(() => rules++);
    css.walkDecls(decl => {
      declarations[decl.prop] = (declarations[decl.prop] || 0) + 1;
    });
    return {scoped: Boolean(block.scoped), lang: block.lang || 'css', rules, declarations};
  });
  const syntax = sfc.template ? parseTemplate(sfc.template.content) : null;
  const elements = {};
  const directives = {};
  function inventory(node) {
    if (!node) return;
    if (node.kind === 'element') {
      elements[node.tag] = (elements[node.tag] || 0) + 1;
      for (const attr of node.attrs) {
        if (/^(v-|:|#|@)/.test(attr.name))
          directives[attr.name] = (directives[attr.name] || 0) + 1;
      }
    }
    (node.children || []).forEach(inventory);
  }
  inventory(syntax);
  return {
    file: filename, sha256: crypto.createHash('sha256').update(source).digest('hex'),
    templatePresent: Boolean(sfc.template), templateSemantics: 'not_verified',
    imports, props, styles, unresolved, templateSyntax: {elements, directives}
  };
}

function main() {
  const [input, modules] = process.argv.slice(2);
  if (!input || !modules || !path.isAbsolute(input) || !path.isAbsolute(modules))
    throw new Error('ABSOLUTE_INPUT_AND_TOOL_DIRECTORY_REQUIRED');
  const external = createRequire(path.join(modules, '__native_parser_loader__.cjs'));
  const tools = {sfc: external('@vue/compiler-sfc'), babel: external('@babel/parser'),
                 postcss: external('postcss')};
  const files = [];
  function visit(dir) {
    for (const entry of fs.readdirSync(dir, {withFileTypes: true}).sort((a, b) =>
      a.name < b.name ? -1 : a.name > b.name ? 1 : 0)) {
      const file = path.join(dir, entry.name);
      if (entry.isSymbolicLink()) throw new Error('INPUT_SYMLINK_REFUSED');
      if (entry.isDirectory()) visit(file);
      else if (entry.name.endsWith('.vue')) files.push(
        inspect(fs.readFileSync(file, 'utf8'), path.relative(input, file).split(path.sep).join('/'), tools));
    }
  }
  visit(input);
  console.log(JSON.stringify({
    scope: 'SFC block, script and CSS inventory only; no Vue template compilation or rendering',
    tools: Object.fromEntries(['@vue/compiler-sfc', '@babel/parser', 'postcss'].map(name =>
      [name, external(name + '/package.json').version])),
    files
  }, null, 2));
}

if (require.main === module) main();
module.exports = {inspect};
