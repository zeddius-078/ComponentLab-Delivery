'use strict';
const fs = require('node:fs');
const path = require('node:path');
const {createRequire} = require('node:module');
const {VueGraph} = require('./vue_graph.cjs');
const {collections} = require('./collection_evidence.cjs');

function evidence(styled, mapping) {
  const instances = new Map(), result = [];
  function index(node) {
    if (node.kind === 'component') {
      if (instances.has(node.source.instancePath)) throw new Error('DUPLICATE_INSTANCE_SCOPE');
      instances.set(node.source.instancePath, node);
    }
    (node.children || []).forEach(index);
  }
  index(styled.componentTree);
  function collect(node, slots = [], output = {text: [], images: []}) {
    if (node.kind === 'element' && node.visible === false) return output;
    const trace = [...slots, ...(node.slotTrace || [])];
    if (node.kind === 'text' && node.text.trim())
      output.text.push({text: node.text, source: node.source, slots: trace});
    const css = node.style && node.style.values || {};
    if (node.tag === 'img' && node.attrs.src)
      output.images.push({reference: node.attrs.src, kind: 'image',
        source: node.source, slots: trace});
    if (css['background-image'] && css['background-image'] !== 'none')
      output.images.push({reference: css['background-image'], kind: 'css-background',
        source: node.style.origins['background-image'], slots: trace});
    (node.children || []).forEach(child => collect(child, trace, output));
    return output;
  }
  function walk(node) {
    for (const boundary of node.componentBoundaries || []) {
      const instance = instances.get(boundary.instancePath);
      if (!instance) throw new Error('MISSING_INSTANCE_SCOPE');
      const rule = mapping.components.find(rule =>
        rule.source.import === boundary.import && rule.source.tag === boundary.tag);
      const content = collect(node), conflicts = [];
      const contract = rule && rule.contentExpectations;
      if (contract && node.visible) {
        const declared = instance.props[contract.property];
        const expected = contract.values[declared];
        if (!expected) conflicts.push({code: 'UNSUPPORTED_CONTENT_VALUE', declared});
        else if (expected.text !== Boolean(content.text.length) || expected.images !== content.images.length)
          conflicts.push({code: 'DECLARED_CONTENT_CONFLICT', property: contract.property, declared,
            expected, observed: {text: Boolean(content.text.length), images: content.images.length}});
      }
      result.push({source: boundary, declaredProps: instance.props,
        visible: node.visible, content, conflicts,
        identityStatus: 'module binding only; live file/family evidence must be joined separately'});
    }
    (node.children || []).forEach(walk);
  }
  walk(styled.styledTree);
  for (const item of result) {
    const candidates = mapping.components.filter(rule =>
      rule.source.import === item.source.import && rule.source.tag === item.source.tag);
    if (candidates.length > 1) throw new Error('AMBIGUOUS_COMPONENT_MODULE');
    if (candidates.length === 1 && candidates[0].collections) {
      try {
        item.collections = collections(item, result, candidates[0].collections);
      } catch (error) {
        item.conflicts.push({code: 'COLLECTION_CONTRACT_FAILED', detail: error.message});
      }
    }
  }
  return result;
}

if (require.main === module) {
  const {styleGraph} = require('./style_graph.cjs');
  const [root, entry, modules, mainFile, modeFile, mappingFile] = process.argv.slice(2);
  if (!root || !path.isAbsolute(root) || !modules || !path.isAbsolute(modules) || !mappingFile)
    throw new Error('EXPLICIT_INPUTS_REQUIRED');
  const external = createRequire(path.join(modules, '__native_content_loader__.cjs'));
  const graph = new VueGraph(root, {sfc: external('@vue/compiler-sfc'), babel: external('@babel/parser'),
    postcss: external('postcss')});
  const styled = styleGraph(graph, entry, mainFile, modeFile);
  console.log(JSON.stringify({schemaVersion: 1, browserMeasured: false,
    components: evidence(styled, JSON.parse(fs.readFileSync(mappingFile, 'utf8')))}));
}
module.exports = {evidence};
