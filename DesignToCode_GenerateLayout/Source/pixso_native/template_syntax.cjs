'use strict';

// Bounded template syntax reader for the native export adapter. Preserves
// PascalCase and source offsets; does not use an HTML DOM (which folds names).
// Semantic directives are interpreted separately. No source is executed.
class TemplateSyntaxError extends Error {}
function parseTemplate(source) {
  let at = 0;
  const root = {kind: 'fragment', children: [], start: 0, end: source.length};
  const stack = [root];
  const fail = message => { throw new TemplateSyntaxError(`${message} at ${at}`); };
  const whitespace = () => { while (/\s/.test(source[at] || '') && at < source.length) at++; };
  const append = node => stack[stack.length - 1].children.push(node);
  const name = () => {
    const start = at;
    while (at < source.length && !/[\s=/>]/.test(source[at])) at++;
    if (at === start) fail('NAME_REQUIRED');
    return source.slice(start, at);
  };
  while (at < source.length) {
    const start = at;
    if (source.startsWith('<!--', at)) {
      const end = source.indexOf('-->', at + 4);
      if (end < 0) fail('UNCLOSED_COMMENT');
      at = end + 3;
      append({kind: 'comment', text: source.slice(start + 4, end), start, end: at});
    } else if (source.startsWith('{{', at)) {
      at += 2;
      const expressionStart = at;
      // Interpolation ends only outside quoted strings and balanced brackets.
      let quote = null, escaped = false;
      const brackets = [];
      while (at < source.length) {
        const char = source[at];
        if (quote) {
          if (escaped) escaped = false;
          else if (char === '\\') escaped = true;
          else if (char === quote) quote = null;
          at++;
          continue;
        }
        if (brackets.length === 0 && source.startsWith('}}', at)) break;
        if (char === '`') fail('TEMPLATE_LITERAL_NOT_SUPPORTED');
        if (char === '"' || char === "'") quote = char;
        else if ('([{'.includes(char)) brackets.push(char);
        else if (')]}'.includes(char)) {
          const expected = {')': '(', ']': '[', '}': '{'}[char];
          if (brackets.pop() !== expected) fail('UNBALANCED_EXPRESSION');
        }
        at++;
      }
      if (at >= source.length) fail('UNCLOSED_INTERPOLATION');
      const expression = source.slice(expressionStart, at).trim();
      if (!expression) fail('EMPTY_INTERPOLATION');
      at += 2;
      append({kind: 'expression', expression, start, end: at});
    } else if (source.startsWith('</', at)) {
      at += 2;
      const tag = name();
      whitespace();
      if (source[at++] !== '>') fail('INVALID_CLOSING_TAG');
      if (stack.length === 1 || stack[stack.length - 1].tag !== tag)
        fail('MISMATCHED_CLOSING_TAG');
      stack.pop().end = at;
    } else if (source[at] === '<') {
      at++;
      if (!/[A-Za-z]/.test(source[at] || '')) fail('UNSUPPORTED_MARKUP');
      const tag = name();
      if (!/^[A-Za-z][A-Za-z0-9_.:-]*$/.test(tag)) fail('INVALID_TAG');
      const node = {kind: 'element', tag, attrs: [], children: [], start, end: null};
      const names = new Set();
      let selfClosing = false;
      while (true) {
        whitespace();
        if (source.startsWith('/>', at)) { at += 2; selfClosing = true; break; }
        if (source[at] === '>') { at++; break; }
        if (at >= source.length) fail('UNCLOSED_START_TAG');
        const attrStart = at;
        const key = name();
        if (names.has(key)) fail('DUPLICATE_ATTRIBUTE');
        names.add(key);
        whitespace();
        let value = null;
        if (source[at] === '=') {
          at++;
          whitespace();
          const quote = source[at++];
          if (quote !== '"' && quote !== "'") fail('QUOTED_ATTRIBUTE_REQUIRED');
          const valueStart = at;
          while (at < source.length && source[at] !== quote) at++;
          if (at >= source.length) fail('UNCLOSED_ATTRIBUTE');
          value = source.slice(valueStart, at++);
        }
        node.attrs.push({name: key, value, start: attrStart, end: at});
      }
      append(node);
      if (selfClosing || ['img', 'input', 'br', 'hr', 'meta', 'link', 'area',
        'base', 'col', 'embed', 'param', 'source', 'track', 'wbr'].includes(tag)) {
        node.end = at;
      } else stack.push(node);
    } else {
      while (at < source.length && source[at] !== '<' && !source.startsWith('{{', at)) at++;
      append({kind: 'text', text: source.slice(start, at), start, end: at});
    }
  }
  if (stack.length !== 1) fail('UNCLOSED_ELEMENT');
  return root;
}
module.exports = {parseTemplate, TemplateSyntaxError};
