'use strict';
// Declared-value cascade, not a browser layout/computed-style replacement.
// Selector and variable subsets fail explicitly; source locations stay attached.
const inherited = new Set(['color', 'font-family', 'font-size', 'font-style',
  'font-weight', 'line-height', 'letter-spacing', 'text-align', 'visibility',
  'white-space', 'word-break', 'direction']);

function selector(text) {
  // Attribute values in the supported export subset contain no whitespace.
  const tokens = text.trim().replace(/\s*>\s*/g, ' > ').split(/\s+/);
  const parts = [], specificity = [0, 0, 0];
  let relation = 'descendant';
  for (const token of tokens) {
    if (token === '>') {
      if (!parts.length || relation === 'child') throw new Error('INVALID_SELECTOR');
      relation = 'child'; continue;
    }
    const tests = [];
    let rest = token;
    while (rest) {
      let match;
      if ((match = /^([.#])([\w-]+)/.exec(rest))) {
        tests.push({kind: match[1] === '.' ? 'class' : 'id', value: match[2]});
        specificity[match[1] === '#' ? 0 : 1]++;
      } else if ((match = /^\[([\w-]+)="([^"]*)"\]/.exec(rest))) {
        tests.push({kind: 'attribute', key: match[1], value: match[2]}); specificity[1]++;
      } else if ((match = /^:root/.exec(rest))) {
        tests.push({kind: 'root'}); specificity[1]++;
      } else if ((match = /^\*/.exec(rest))) {
        tests.push({kind: 'any'});
      } else if ((match = /^[A-Za-z][\w-]*/.exec(rest))) {
        tests.push({kind: 'tag', value: match[0]}); specificity[2]++;
      } else throw new Error('UNSUPPORTED_SELECTOR: ' + text);
      rest = rest.slice(match[0].length);
    }
    parts.push({tests, relation}); relation = 'descendant';
  }
  if (relation === 'child') throw new Error('INVALID_SELECTOR');
  return {parts, specificity};
}

function matches(parsed, node, ancestors) {
  const simple = (part, element, index) => part.tests.every(test => {
    const attrs = element.attrs || {};
    if (test.kind === 'any') return true;
    if (test.kind === 'root') return index === 0 && element.tag === 'html';
    if (test.kind === 'tag') return element.tag === test.value;
    if (test.kind === 'id') return attrs.id === test.value;
    if (test.kind === 'class') return String(attrs.class || '').split(/\s+/).includes(test.value);
    return attrs[test.key] === test.value;
  });
  const chain = [...ancestors, node];
  function search(partIndex, index) {
    if (index < 0 || !simple(parsed.parts[partIndex], chain[index], index)) return false;
    if (partIndex === 0) return true;
    if (parsed.parts[partIndex].relation === 'child') return search(partIndex - 1, index - 1);
    for (let previous = index - 1; previous >= 0; previous--)
      if (search(partIndex - 1, previous)) return true;
    return false;
  }
  return search(parsed.parts.length - 1, chain.length - 1);
}

function substitute(value, variables, stack = []) {
  let at = 0, result = '';
  while (at < value.length) {
    const start = value.indexOf('var(', at);
    if (start < 0) return result + value.slice(at);
    result += value.slice(at, start);
    let end = start + 4, depth = 1, comma = -1;
    for (; end < value.length; end++) {
      if (value[end] === '(') depth++;
      if (value[end] === ')' && --depth === 0) break;
      // Restore depth for chars other than ')' (decrement happens only above).
      if (value[end] === ',' && depth === 1 && comma < 0) comma = end;
    }
    if (end >= value.length) throw new Error('UNCLOSED_CSS_VAR');
    const name = value.slice(start + 4, comma < 0 ? end : comma).trim();
    if (!/^--[\w-]+$/.test(name)) throw new Error('INVALID_CSS_VAR');
    if (stack.includes(name)) throw new Error('CYCLIC_CSS_VAR: ' + name);
    if (Object.prototype.hasOwnProperty.call(variables, name))
      result += substitute(variables[name], variables, [...stack, name]);
    else if (comma >= 0) result += substitute(value.slice(comma + 1, end).trim(), variables, stack);
    else throw new Error('UNRESOLVED_CSS_VAR: ' + name);
    at = end + 1;
  }
  return result;
}

function rankCompare(a, b) {
  for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return a[i] - b[i];
  return 0;
}

class Cascade {
  constructor(postcss) {
    this.postcss = postcss;
    this.rules = [];
    this.fonts = [];
  }
  add(source, file) {
    const root = this.postcss.parse(source, {from: file});
    for (const rule of root.nodes) {
      if (rule.type === 'comment') continue;
      if (rule.type === 'atrule' && rule.name === 'font-face') {
        this.fonts.push({file, declarations: rule.nodes.filter(n => n.type === 'decl')
          .map(n => ({property: n.prop, value: n.value}))});
        continue;
      }
      if (rule.type !== 'rule') throw new Error('UNSUPPORTED_CSS_RULE: ' + rule.type);
      for (const text of rule.selectors) {
        // Generated list pseudo-elements are not ordinary elements; retain a
        // selector guard and reject only if their originating element is used.
        const pseudo = /::(before|after|marker)$/.exec(text);
        const matchText = pseudo ? text.slice(0, pseudo.index) : text;
        this.rules.push({selector: selector(matchText), text, pseudo: pseudo && pseudo[1],
          declarations: rule.nodes.map(decl => {
            if (decl.type === 'comment') return null;
            if (decl.type !== 'decl') throw new Error('NESTED_CSS_UNSUPPORTED');
            return {property: decl.prop, value: decl.value, important: Boolean(decl.important),
              source: {file, line: decl.source.start.line, selector: text}};
          }).filter(Boolean)});
      }
    }
  }
  resolve(node, ancestors, parent = {values: {}, variables: {}}) {
    const winners = new Map();
    let order = 0;
    const put = (decl, specificity, inline = 0) => {
      const rank = [Number(decl.important), inline, ...specificity, order++];
      // Shorthands compete with longhands in the cascade, not afterwards.
      // Keep the raw value until custom properties have been resolved: a var()
      // can expand to several lengths, so splitting it here would lose sides.
      const properties = ['padding', 'margin'].includes(decl.property)
        ? ['top', 'right', 'bottom', 'left'].map(side => decl.property + '-' + side)
        : [decl.property];
      properties.forEach((property, side) => {
        const previous = winners.get(property);
        if (!previous || rankCompare(rank, previous.rank) >= 0)
          winners.set(property, {...decl, rank,
            boxSide: properties.length === 4 ? side : undefined});
      });
    };
    for (const rule of this.rules) {
      if (!matches(rule.selector, node, ancestors)) continue;
      if (rule.pseudo) throw new Error('GENERATED_PSEUDO_CONTENT_UNSUPPORTED: ' + rule.text);
      for (const decl of rule.declarations) put(decl, rule.selector.specificity);
    }
    if (node.attrs && node.attrs.style) {
      if (typeof node.attrs.style !== 'string') throw new Error('DYNAMIC_STYLE_OBJECT_UNSUPPORTED');
      const inline = this.postcss.parse('x{' + node.attrs.style + '}').first;
      for (const decl of inline.nodes) {
        if (decl.type !== 'decl') throw new Error('INVALID_INLINE_STYLE');
        put({property: decl.prop, value: decl.value, important: Boolean(decl.important),
          source: {file: node.source && node.source.file, inline: true}}, [0, 0, 0], 1);
      }
    }
    const variables = {...parent.variables};
    for (const [key, decl] of winners) if (key.startsWith('--')) variables[key] = decl.value;
    const resolvedVariables = {};
    for (const key of Object.keys(variables))
      resolvedVariables[key] = substitute(variables[key], variables, [key]);
    const values = {}, origins = {};
    for (const key of inherited) if (key in parent.values) values[key] = parent.values[key];
    for (const [key, decl] of winners) {
      if (key.startsWith('--')) continue;
      let value = substitute(decl.value, resolvedVariables).trim();
      if (decl.boxSide !== undefined) {
        const parts = this.postcss.list.space(value);
        if (parts.length < 1 || parts.length > 4)
          throw new Error('UNSUPPORTED_BOX_SHORTHAND: ' + decl.property);
        const sides = parts.length === 1 ? [0, 0, 0, 0]
          : parts.length === 2 ? [0, 1, 0, 1]
          : parts.length === 3 ? [0, 1, 2, 1] : [0, 1, 2, 3];
        value = parts[sides[decl.boxSide]];
      }
      if (value === 'inherit' || (value === 'unset' && inherited.has(key))) {
        if (key in parent.values) values[key] = parent.values[key];
        else delete values[key];
      } else if (value === 'initial' || value === 'unset') delete values[key];
      else if (value === 'revert' || value === 'revert-layer')
        throw new Error('CSS_REVERT_UNSUPPORTED');
      else values[key] = value;
      origins[key] = decl.source;
    }
    return {values, origins, variables: resolvedVariables, measured: false};
  }
}
module.exports = {Cascade, selector, matches, substitute};
