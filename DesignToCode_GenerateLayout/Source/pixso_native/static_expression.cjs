'use strict';

// Evaluates only a bounded AST subset. Never eval(), Function(), require() from
// an input expression, or execute Vue/application scripts.
class StaticExpressionError extends Error {}
const own = (object, key) => Object.prototype.hasOwnProperty.call(object, key);
const forbidden = new Set(['__proto__', 'prototype', 'constructor']);

function evaluate(ast, scope, options = {}) {
  switch (ast.type) {
    case 'StringLiteral':
    case 'NumericLiteral':
    case 'BooleanLiteral':
      return ast.value;
    case 'NullLiteral':
      return null;
    case 'Identifier':
      if (ast.name === 'undefined') return undefined;
      if (forbidden.has(ast.name))
        throw new StaticExpressionError(`UNRESOLVED_IDENTIFIER: ${ast.name}`);
      if (!own(scope, ast.name)) {
        // Template expressions in a compiled Vue render proxy yield undefined
        // for undeclared identifiers. Only template callers opt in; all other
        // expression evaluation stays fail-closed.
        if (!options.undefinedIdentifiers)
          throw new StaticExpressionError(`UNRESOLVED_IDENTIFIER: ${ast.name}`);
        if (options.missing) options.missing.add(ast.name);
        return undefined;
      }
      return scope[ast.name];
    case 'ParenthesizedExpression':
    case 'TSAsExpression':
    case 'TSTypeAssertion':
    case 'TSNonNullExpression':
      return evaluate(ast.expression, scope, options);
    case 'UnaryExpression': {
      if (!['!', '+', '-'].includes(ast.operator))
        throw new StaticExpressionError(`UNSUPPORTED_UNARY: ${ast.operator}`);
      const value = evaluate(ast.argument, scope, options);
      if (ast.operator === '!') return !value;
      if (typeof value !== 'number' || !Number.isFinite(value))
        throw new StaticExpressionError('NUMERIC_OPERAND_REQUIRED');
      return ast.operator === '-' ? -value : value;
    }
    case 'LogicalExpression': {
      const left = evaluate(ast.left, scope, options);
      if (ast.operator === '&&') return left ? evaluate(ast.right, scope, options) : left;
      if (ast.operator === '||') return left ? left : evaluate(ast.right, scope, options);
      if (ast.operator === '??') return left == null ? evaluate(ast.right, scope, options) : left;
      throw new StaticExpressionError(`UNSUPPORTED_LOGICAL: ${ast.operator}`);
    }
    case 'BinaryExpression': {
      const left = evaluate(ast.left, scope, options);
      const right = evaluate(ast.right, scope, options);
      if (ast.operator === '===') return left === right;
      if (ast.operator === '!==') return left !== right;
      // Coercive equality and arbitrary arithmetic are not assumed equivalent.
      throw new StaticExpressionError(`UNSUPPORTED_BINARY: ${ast.operator}`);
    }
    case 'ConditionalExpression':
      return evaluate(ast.test, scope, options)
        ? evaluate(ast.consequent, scope, options) : evaluate(ast.alternate, scope, options);
    case 'MemberExpression': {
      const object = evaluate(ast.object, scope, options);
      const key = ast.computed ? evaluate(ast.property, scope, options) : ast.property.name;
      if (typeof key !== 'string' || forbidden.has(key)
          || object === null || typeof object !== 'object'
          || !own(object, key))
        throw new StaticExpressionError('UNRESOLVED_MEMBER');
      return object[key];
    }
    default:
      throw new StaticExpressionError(`UNSUPPORTED_EXPRESSION: ${ast.type}`);
  }
}

function resolve(expression, scope, parser, options = {}) {
  // Parser is explicitly injected from a selected tool directory. No global
  // resolution, automatic package installation, or first-route dependency.
  const ast = parser.parseExpression(expression, {
    plugins: ['typescript'], createParenthesizedExpressions: true
  });
  return evaluate(ast, scope, options);
}

module.exports = { evaluate, resolve, StaticExpressionError };
