const fs = require('fs');
const path = require('path');
const project = path.resolve(__dirname, '../../..');
const inside = (root, value) => {
  const relative = path.relative(root, value);
  return relative === '' || (!relative.startsWith('..' + path.sep) && relative !== '..' && !path.isAbsolute(relative));
};
function runtimeRoots() {
  const tool = process.env.D2C_RUNTIME_ROOT;
  const work = process.env.D2C_WORK_ROOT;
  if (!tool && !work) return null;
  if (!tool || !work) throw Error('OFFLINE_ROOTS_REQUIRED');
  for (const root of [tool, work]) {
    if (!path.isAbsolute(root) || !fs.existsSync(root) || !fs.statSync(root).isDirectory()) throw Error('OFFLINE_ROOT_INVALID');
    rejectLinks(path.resolve(root));
  }
  if (inside(path.resolve(tool), path.resolve(work))) throw Error('WORK_ROOT_INSIDE_RUNTIME');
  return {tool: path.resolve(tool), work: path.resolve(work)};
}
function rejectLinks(value) {
  for (let cursor = value;; cursor = path.dirname(cursor)) {
    try {
      if (fs.lstatSync(cursor).isSymbolicLink()) throw Error('OUTPUT_PATH_IS_LINK');
    } catch (error) {
      if (error.code !== 'ENOENT' && error.code !== 'ENOTDIR') throw error;
    }
    if (path.dirname(cursor) === cursor) break;
  }
}
function outputPath(value) {
  const output = path.resolve(value);
  const roots = runtimeRoots();
  if (roots) {
    const allowed = ['work', 'logs', 'delivery'];
    if (inside(roots.tool, output) || !allowed.some(name => inside(path.join(roots.work, 'output', name), output)))
      throw Error('OUTPUT_MUST_BE_IN_WORK_BUILD');
  } else {
    if (!['work', 'logs', 'delivery'].some(name => inside(path.join(project, 'output', name), output))) throw Error('OUTPUT_MUST_BE_IN_WORK_BUILD');
  }
  rejectLinks(output);
  return output;
}
function browserLaunchOptions() {
  const roots = runtimeRoots();
  const executable = process.env.D2C_BROWSER_EXE;
  if (executable) {
    if (!path.isAbsolute(executable) || !fs.existsSync(executable) || !fs.statSync(executable).isFile())
      throw Error('BROWSER_EXECUTABLE_MISSING');
    return {executablePath: path.resolve(executable), headless: true};
  }
  throw Error('OFFLINE_BROWSER_EXECUTABLE_REQUIRED');
}
module.exports = {outputPath, runtimeRoots, browserLaunchOptions};
