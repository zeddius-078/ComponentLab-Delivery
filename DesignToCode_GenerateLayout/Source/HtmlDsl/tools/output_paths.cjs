const fs = require('fs');
const path = require('path');
const project = path.resolve(__dirname, '../../..');
const inside = (root, value) => {
  const relative = path.relative(root, value);
  return relative === '' || (!relative.startsWith('..' + path.sep) && relative !== '..' && !path.isAbsolute(relative));
};
function outputPath(value) {
  const output = path.resolve(value);
  if (inside(project, output) && !inside(path.join(project, 'build'), output)) throw Error('OUTPUT_MUST_BE_IN_BUILD');
  for (let cursor = output;; cursor = path.dirname(cursor)) {
    if (fs.existsSync(cursor) && fs.lstatSync(cursor).isSymbolicLink()) throw Error('OUTPUT_PATH_IS_LINK');
    if (path.dirname(cursor) === cursor) break;
  }
  return output;
}
module.exports = {outputPath};
