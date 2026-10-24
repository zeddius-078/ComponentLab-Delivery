'use strict';
// Static interpretation of the observed D2C subset. Component boundaries are
// retained: expanded templates are evidence, NOT permission to flatten SDK UI.
const fs = require('node:fs');
const path = require('node:path');
const {createRequire} = require('node:module');
const {inspect} = require('./inspect_sfc.cjs');
const {parseTemplate} = require('./template_syntax.cjs');
const {resolve} = require('./static_expression.cjs');
const own = (obj, key) => Object.prototype.hasOwnProperty.call(obj, key);

function entities(text) {
  return text.replace(/&([^;\s]+);/g, (_, code) => {
    const named = {amp: '&', lt: '<', gt: '>', quot: '"', apos: "'", nbsp: '\u00a0'};
    if (own(named, code)) return named[code];
    if (/^#\d+$/.test(code)) return String.fromCodePoint(Number(code.slice(1)));
    if (/^#x[0-9a-f]+$/i.test(code)) return String.fromCodePoint(parseInt(code.slice(2), 16));
    throw new Error('UNSUPPORTED_CHARACTER_REFERENCE: ' + code);
  });
}

class VueGraph {
  constructor(root, tools) {
    this.root = fs.realpathSync(root);
    this.tools = tools;
    this.cache = new Map();
    this.count = 0;
    this.undeclared = new Set();
  }
  templateOptions() {
    return {undefinedIdentifiers: true, missing: this.undeclared};
  }
  load(filename) {
    const file = fs.realpathSync(path.resolve(this.root, filename));
    const rel = path.relative(this.root, file);
    if (rel.startsWith('..') || path.isAbsolute(rel)) throw new Error('IMPORT_OUTSIDE_INPUT');
    if (!file.endsWith('.vue')) throw new Error('VUE_MODULE_REQUIRED');
    if (this.cache.has(file)) return this.cache.get(file);
    const source = fs.readFileSync(file, 'utf8');
    const info = inspect(source, rel.split(path.sep).join('/'), this.tools);
    if (info.unresolved.length) throw new Error('UNRESOLVED_SCRIPT: ' + info.unresolved.join(';'));
    const sfc = this.tools.sfc.parseComponent(source);
    if (!sfc.template) throw new Error('MISSING_TEMPLATE');
    const imports = Object.create(null);
    for (const imp of info.imports) for (const binding of imp.bindings) {
      if (imp.from.endsWith('.vue')) {
        if (binding.kind !== 'ImportDefaultSpecifier') throw new Error('UNSUPPORTED_COMPONENT_IMPORT');
        imports[binding.local] = imp.from.startsWith('@/')
          ? imp.from.slice(2) : path.relative(this.root, path.resolve(path.dirname(file), imp.from));
      } else {
        // Runtime imports are recorded by inventory but never executed. A
        // reference to an imported runtime binding will fail static evaluation.
      }
    }
    const result = {file: info.file, props: info.props, imports, sha256: info.sha256,
                    template: parseTemplate(sfc.template.content)};
    this.cache.set(file, result);
    return result;
  }
  component(filename, supplied, slots, instancePath, stack = []) {
    if (stack.length > 64) throw new Error('COMPONENT_DEPTH_LIMIT');
    const module = this.load(filename);
    if (stack.includes(module.file)) throw new Error('RECURSIVE_COMPONENT_NOT_SUPPORTED');
    const values = Object.create(null);
    for (const [key, spec] of Object.entries(module.props)) {
      let value = own(supplied, key) ? supplied[key] : spec.hasDefault ? spec.default
        : spec.type === 'Boolean' ? false : undefined;
      if (spec.type === 'Boolean' && (value === '' || value === key)) value = true;
      if (value !== undefined && spec.type && typeof value !==
          ({String: 'string', Boolean: 'boolean', Number: 'number'}[spec.type]))
        throw new Error('PROP_TYPE_MISMATCH: ' + module.file + ':' + key);
      values[key] = value;
    }
    const scope = Object.assign(Object.create(null), values, {props: values});
    const context = {module, scope, slots, instancePath, stack: [...stack, module.file]};
    return {kind: 'component', source: {file: module.file, sha256: module.sha256, instancePath},
            props: values, children: this.nodes(module.template.children, context)};
  }
  nodes(nodes, context) {
    return nodes.flatMap((node, index) => this.node(node, context, index));
  }
  node(node, context, index) {
    if (++this.count > 100000) throw new Error('EXPANSION_LIMIT');
    if (node.kind === 'comment') return [];
    const textSource = {file: context.module.file, offset: node.start,
      instancePath: context.instancePath};
    if (node.kind === 'text') return [{kind: 'text', text: entities(node.text), source: textSource}];
    if (node.kind === 'expression') {
      const value = resolve(node.expression, context.scope, this.tools.babel, this.templateOptions());
      if (value !== null && value !== undefined && !['string', 'number', 'boolean'].includes(typeof value))
        throw new Error('NON_SCALAR_INTERPOLATION');
      return [{kind: 'text', text: value == null ? '' : String(value), source: textSource}];
    }
    const attrs = Object.create(null);
    for (const attr of node.attrs) {
      if (attr.name === 'v-if') {
        if (!resolve(entities(attr.value), context.scope, this.tools.babel, this.templateOptions())) return [];
      } else if (attr.name.startsWith('v-') || attr.name.startsWith('@'))
        throw new Error('UNSUPPORTED_DIRECTIVE: ' + attr.name);
      else if (attr.name.startsWith(':')) {
        const name = attr.name.slice(1);
        if (!/^[A-Za-z_][A-Za-z0-9_-]*$/.test(name)) throw new Error('UNSUPPORTED_BINDING');
        attrs[name] = resolve(entities(attr.value), context.scope, this.tools.babel, this.templateOptions());
      } else if (!attr.name.startsWith('#')) attrs[attr.name] =
        attr.value === null ? '' : entities(attr.value);
    }
    if (node.tag === 'slot') {
      const name = attrs.name || 'default';
      const override = context.slots[name];
      const contents = override ? this.nodes(override.nodes, {
        ...override.context,
        // Values belong to the authoring scope; placement belongs to this
        // outlet. Reusing a slot must not reuse its runtime instance identity.
        instancePath: context.instancePath + '/slot:' + name + '@' + node.start
      })
        : this.nodes(node.children, context);
      return contents.map(child => ({...child, slotTrace: [
        {name, file: context.module.file, offset: node.start,
          instancePath: context.instancePath, overridden: Boolean(override)},
        ...(child.slotTrace || [])
      ]}));
    }
    const source = {file: context.module.file, offset: node.start,
                    instancePath: context.instancePath, designNodeId: attrs.id || null};
    if (own(context.module.imports, node.tag)) {
      const slots = Object.create(null);
      const ordinary = [];
      for (const child of node.children) {
        const slot = child.kind === 'element' && child.tag === 'template'
          && child.attrs.find(attr => attr.name.startsWith('#'));
        if (slot) {
          const name = slot.name.slice(1);
          if (slot.value !== null || !/^[A-Za-z_][A-Za-z0-9_]*$/.test(name))
            throw new Error('SCOPED_OR_DYNAMIC_SLOT_NOT_SUPPORTED');
          if (own(slots, name)) throw new Error('DUPLICATE_SLOT');
          slots[name] = {nodes: child.children, context};
        } else ordinary.push(child);
      }
      if (ordinary.some(child => child.kind !== 'text' || child.text.trim()))
        slots.default = {nodes: ordinary, context};
      const instancePath = context.instancePath + '/' + (attrs.id || node.tag + '@' + node.start);
      const child = this.component(context.module.imports[node.tag], attrs, slots, instancePath, context.stack);
      child.source = {...child.source, usage: source, tag: node.tag,
                      import: context.module.imports[node.tag]};
      child.attrs = attrs;
      return [child];
    }
    if (node.tag === 'template') return this.nodes(node.children, context);
    if (!['div', 'p', 'span', 'img', 'svg', 'path', 'g', 'rect', 'circle',
      'line', 'polyline', 'polygon', 'defs', 'clipPath', 'use', 'br'].includes(node.tag))
      throw new Error('UNRESOLVED_ELEMENT: ' + node.tag);
    return [{kind: 'element', tag: node.tag, attrs, source,
             children: this.nodes(node.children, context)}];
  }
}
if (require.main === module) {
  const [root, entry, modules] = process.argv.slice(2);
  if (!root || !entry || !modules || !path.isAbsolute(root) || !path.isAbsolute(modules))
    throw new Error('EXPLICIT_INPUT_ENTRY_TOOLS_REQUIRED');
  const external = createRequire(path.join(modules, '__native_parser_loader__.cjs'));
  const graph = new VueGraph(root, {sfc: external('@vue/compiler-sfc'),
    babel: external('@babel/parser'), postcss: external('postcss')});
  console.log(JSON.stringify({schemaVersion: 1, mode: 'static-export-subset',
    browserMeasured: false, tree: graph.component(entry, {}, {}, 'root')}));
}
module.exports = {VueGraph, entities};
