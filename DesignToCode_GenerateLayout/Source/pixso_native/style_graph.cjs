'use strict';
const fs = require('node:fs');
const path = require('node:path');
const {createRequire} = require('node:module');
const {VueGraph} = require('./vue_graph.cjs');
const {Cascade} = require('./css_cascade.cjs');

function modeAssignments(source, parser) {
  const ast = parser.parse(source, {sourceType: 'module'});
  const assignments = [], bindings = new Map();
  const isMember = (node, object, member) => node && node.type === 'MemberExpression'
    && !node.computed && node.object.name === object && node.property.name === member;
  for (const stmt of ast.program.body) {
    const call = stmt.type === 'ExpressionStatement' && stmt.expression;
    if (!call || call.type !== 'CallExpression'
      || !isMember(call.callee, 'document', 'addEventListener')
      || call.arguments[0].value !== 'DOMContentLoaded'
      || !['FunctionExpression', 'ArrowFunctionExpression'].includes(call.arguments[1].type))
      throw new Error('UNSUPPORTED_MODE_SCRIPT');
    for (const line of call.arguments[1].body.body) {
      if (line.type === 'VariableDeclaration') {
        for (const decl of line.declarations) {
          if (decl.id.type !== 'Identifier' || !decl.init || decl.init.type !== 'CallExpression'
              || !isMember(decl.init.callee, 'document', 'getElementById')
              || decl.init.arguments.length !== 1 || decl.init.arguments[0].type !== 'StringLiteral')
            throw new Error('UNSUPPORTED_MODE_BINDING');
          if (bindings.has(decl.id.name)) throw new Error('DUPLICATE_MODE_BINDING');
          bindings.set(decl.id.name, decl.init.arguments[0].value);
        }
      } else if (line.type === 'IfStatement' && line.test.type === 'Identifier'
          && bindings.has(line.test.name) && !line.alternate) {
        if (line.consequent.type !== 'BlockStatement') throw new Error('MODE_BLOCK_REQUIRED');
        for (const effect of line.consequent.body) {
          const set = effect.type === 'ExpressionStatement' && effect.expression;
          if (!set || set.type !== 'CallExpression'
              || !isMember(set.callee, line.test.name, 'setAttribute')
              || set.arguments.length !== 2 || set.arguments.some(a => a.type !== 'StringLiteral')
              || !/^data-[\w-]+-mode$/.test(set.arguments[0].value))
            throw new Error('UNSUPPORTED_MODE_EFFECT');
          assignments.push({id: bindings.get(line.test.name),
            attribute: set.arguments[0].value, value: set.arguments[1].value});
        }
      } else throw new Error('UNSUPPORTED_MODE_STATEMENT');
    }
  }
  return assignments;
}

function materialize(component) {
  function visit(node) {
    if (node.kind === 'component') {
      const roots = node.children.flatMap(visit);
      if (node.attrs) {
        const elements = roots.filter(n => n.kind === 'element');
        const inherited = Object.fromEntries(Object.entries(node.attrs)
          .filter(([key]) => !Object.prototype.hasOwnProperty.call(node.props, key)));
        if (elements.length !== 1 && Object.keys(inherited).length)
          throw new Error('FRAGMENT_ATTRIBUTE_FALLTHROUGH_UNRESOLVED');
        if (elements.length === 1) {
          const target = elements[0];
          target.attrs = {...target.attrs, ...inherited,
            class: [target.attrs.class, inherited.class].filter(Boolean).join(' '),
            style: [target.attrs.style, inherited.style].filter(Boolean).join(';')};
          target.componentBoundaries = [...(target.componentBoundaries || []), node.source];
          target.slotTrace = [...(node.slotTrace || []), ...(target.slotTrace || [])];
        }
      }
      return roots;
    }
    return [{...node, attrs: {...node.attrs},
      children: (node.children || []).flatMap(visit)}];
  }
  return visit(component);
}

function styleGraph(graph, entry, mainFile, modeFile) {
  const tree = graph.component(entry, {}, {}, 'root');
  const cascade = new Cascade(graph.tools.postcss);
  const within = file => {
    const resolved = fs.realpathSync(path.resolve(graph.root, file));
    const rel = path.relative(graph.root, resolved);
    if (rel.startsWith('..') || path.isAbsolute(rel)) throw new Error('STYLE_OUTSIDE_INPUT');
    return resolved;
  };
  const main = graph.tools.babel.parse(fs.readFileSync(within(mainFile), 'utf8'),
    {sourceType: 'module', plugins: ['typescript']});
  const seen = new Set();
  for (const node of main.program.body) {
    if (node.type === 'ImportDeclaration' && node.source.value.endsWith('.css')) {
      const file = within(path.join(path.dirname(mainFile), node.source.value));
      if (!seen.has(file)) {
        seen.add(file);
        cascade.add(fs.readFileSync(file, 'utf8'), path.relative(graph.root, file));
      }
    }
  }
  const loaded = new Set();
  function addModule(file) {
    const module = graph.load(file);
    if (loaded.has(module.file)) return;
    loaded.add(module.file);
    for (const dependency of Object.values(module.imports)) addModule(dependency);
    const source = fs.readFileSync(within(module.file), 'utf8');
    const styles = graph.tools.sfc.parseComponent(source).styles;
    for (const block of styles) {
      if (block.scoped || block.module) throw new Error('SCOPED_CSS_NOT_IMPLEMENTED');
      cascade.add(block.content, module.file);
    }
  }
  addModule(entry);
  const dom = {kind: 'element', tag: 'html', attrs: {}, children: [
    {kind: 'element', tag: 'body', attrs: {}, children: [
      {kind: 'element', tag: 'div', attrs: {id: 'app'}, children: materialize(tree)}
    ]}
  ]};
  const ids = new Map();
  function inventory(node) {
    if (node.attrs && node.attrs.id) {
      if (!ids.has(node.attrs.id)) ids.set(node.attrs.id, []);
      ids.get(node.attrs.id).push(node);
    }
    (node.children || []).forEach(inventory);
  }
  inventory(dom);
  const mode = modeAssignments(fs.readFileSync(within(modeFile), 'utf8'), graph.tools.babel);
  const ambiguous = [], absent = [];
  for (const assignment of mode) {
    const targets = ids.get(assignment.id) || [];
    if (!targets.length) { absent.push(assignment.id); continue; }
    if (targets.length > 1) ambiguous.push({id: assignment.id, matches: targets.length});
    // Match getElementById's first-node behavior; never apply a guessed mode
    // to all instances. The ambiguity remains an explicit acceptance issue.
    targets[0].attrs[assignment.attribute] = assignment.value;
  }
  function apply(node, ancestors, parent, ancestorDisplayNone) {
    if (node.kind !== 'element') return;
    node.style = cascade.resolve(node, ancestors, parent);
    const displayNone = ancestorDisplayNone || node.style.values.display === 'none';
    node.inLayout = !displayNone;
    node.visible = !displayNone && !['hidden', 'collapse'].includes(node.style.values.visibility);
    for (const child of node.children || [])
      apply(child, [...ancestors, node], node.style, displayNone);
  }
  apply(dom, [], undefined, false);
  return {schemaVersion: 1, mode: 'static-declared-css', browserMeasured: false,
    componentTree: tree, styledTree: dom, fonts: cascade.fonts,
    undeclaredTemplateIdentifiers: [...graph.undeclared].sort(),
    modeEvidence: {ambiguous, absent, timing: 'assignments statically applied; DOMContentLoaded timing not verified'},
    stylesheetOrder: 'main CSS imports followed by component dependencies in postorder; bundler execution not verified'};
}

if (require.main === module) {
  const [root, entry, modules, mainFile, modeFile, mappingFile] = process.argv.slice(2);
  if (!root || !path.isAbsolute(root) || !modules || !path.isAbsolute(modules)
      || !entry || !mainFile || !modeFile) throw new Error('EXPLICIT_INPUTS_REQUIRED');
  const external = createRequire(path.join(modules, '__native_style_loader__.cjs'));
  const graph = new VueGraph(root, {sfc: external('@vue/compiler-sfc'), babel: external('@babel/parser'),
    postcss: external('postcss')});
  const result = styleGraph(graph, entry, mainFile, modeFile);
  if (mappingFile) {
    const {evidence} = require('./content_evidence.cjs');
    result.componentEvidence = evidence(result, JSON.parse(fs.readFileSync(mappingFile, 'utf8')));
  }
  console.log(JSON.stringify(result));
}
module.exports = {styleGraph, modeAssignments, materialize};
