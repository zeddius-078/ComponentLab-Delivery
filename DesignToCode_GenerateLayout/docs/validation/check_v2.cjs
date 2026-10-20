// 验证唯一的 HTML + DSL 路线：双来源指纹、静态 Qt 文件、运行副本和离线启动。
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const childProcess = require('child_process');

const projectRoot = path.resolve(__dirname, '../..');
const sourceRoot = path.join(projectRoot, 'Source');
const routeRoot = path.join(sourceRoot, 'HtmlDsl');
const generated = path.join(projectRoot, 'output/work/preview/html-dsl');
const fixtures = path.join(projectRoot, 'docs/fixtures/layout');
const deployed = path.join(projectRoot, 'output/work/demo/page_examples/V2/data/html-dsl');
const reports = path.join(projectRoot, 'output/logs/v2');
const resultPath = path.join(reports, 'result.json');
const load = file => JSON.parse(fs.readFileSync(file, 'utf8'));
const hash = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const assert = (condition, message) => { if (!condition) throw new Error(message); };
const generatedFile = (page, extension) => path.join(
  generated,
  (page === 'result' ? 'result-html' : page) + extension
);

const result = {
  verifiedAtUtc: new Date().toISOString(),
  route: 'HTML + DSL',
  structureSource: 'HTML measured layout + Pixso DSL identity -> generated .ui -> QUiLoader'
};

try {
  childProcess.execFileSync(process.env.PYTHON || 'python', [
    '-B', path.join(projectRoot, 'docs/validation/test_independence.py')
  ], {stdio: 'pipe'});
  result.privateComponentBoundary = 'PASS';

  const xml = fs.readFileSync(path.join(reports, 'html-dsl/qt-results.xml'), 'utf8');
  assert(!/<Incident type="(fail|xpass)"/.test(xml), 'QT_TEST_FAILED');
  const functions = [...xml.matchAll(/<TestFunction name="([^"]+)">([\s\S]*?)<\/TestFunction>/g)];
  assert(functions.length > 2 && functions.every(item => item[2].includes('<Incident type="pass"')),
    'INCOMPLETE_QT_TESTS');
  result.qtTestCases = functions.filter(item => !['initTestCase', 'cleanupTestCase'].includes(item[1])).length;

  const htmlReport = load(path.join(reports, 'html-dsl-adaptation/result/html-result.json'));
  assert(htmlReport.status === 'PASS', 'HTML_DSL_ADAPTATION_FAILED');
  assert(htmlReport.sourceDslSha256 === hash(path.join(fixtures, 'design-inputs/result-page/canonical.raw.json')),
    'DSL_FINGERPRINT_MISMATCH');
  assert(htmlReport.sourceHtmlSha256 === hash(path.join(fixtures, 'html-dsl/result-page/page.html')),
    'HTML_FINGERPRINT_MISMATCH');
  assert(htmlReport.outputSha256 === load(generatedFile('result', '.components.json')).sourceSha256,
    'DERIVED_FINGERPRINT_MISMATCH');
  result.sourceFingerprints = {
    html: htmlReport.sourceHtmlSha256,
    dsl: htmlReport.sourceDslSha256,
    merged: htmlReport.outputSha256
  };

  for (const page of ['review', 'result', 'instrument_host']) {
    const extensions = page === 'instrument_host'
      ? ['.ui', '.qss', '.components.json']
      : ['.ui', '.qss', '.components.json', '.design-tree.json'];
    for (const extension of extensions) {
      assert(hash(generatedFile(page, extension)) === hash(path.join(deployed, page + extension)),
        `DEPLOY_FILE_MISMATCH: ${page}${extension}`);
    }
  }

  for (const page of ['review', 'result']) {
    const treePath = generatedFile(page, '.design-tree.json');
    const tree = load(treePath);
    const manifest = load(generatedFile(page, '.components.json'));
    assert(manifest.engineeringNamesSha256 === hash(path.join(fixtures, 'mapping/engineering-names.json')),
      'ENGINEERING_NAMES_FINGERPRINT_MISMATCH');
    assert(hash(treePath) === manifest.designTreeSha256, 'DESIGN_TREE_HASH_MISMATCH');
    assert(tree.sourceSha256 === manifest.sourceSha256, 'DESIGN_SOURCE_MISMATCH');
    const nodes = new Map(tree.nodes.map(node => [node.nodeId, node]));
    assert(nodes.size === tree.nodes.length && nodes.size === Object.keys(manifest.nodes).length,
      'DESIGN_NODE_COUNT_MISMATCH');
    const names = new Set(Object.values(manifest.nodes));
    assert(names.size === nodes.size, 'DUPLICATE_ENGINEERING_NAME');
    for (const node of tree.nodes) {
      const name = manifest.nodes[node.nodeId];
      assert(typeof name === 'string' && name.length > 0 && !name.startsWith('node_'),
        `ENGINEERING_NAME_MISSING: ${node.nodeId}`);
      for (const child of node.children)
        assert(nodes.get(child)?.parentId === node.nodeId, `DESIGN_PARENT_MISMATCH: ${child}`);
    }
    if (page === 'result') {
      assert(manifest.nodes['21:473'] === 'layout_resultActions' &&
        manifest.layoutNodeIds.includes('21:473'), 'STRUCTURAL_LAYOUT_BINDING_MISSING');
      const ui = fs.readFileSync(generatedFile(page, '.ui'), 'utf8');
      assert(ui.includes('<layout class="QHBoxLayout" name="layout_resultActions">') &&
        !ui.includes('<widget class="QWidget" name="resultActions">'),
        'STRUCTURAL_FRAME_STILL_WRAPPED');
    }
  }

  const offline = load(path.join(reports, 'offline-html-dsl/result.json'));
  assert(offline.status === 'PASS' && offline.platform === 'windows', 'OFFLINE_HTML_DSL_FAILED');
  assert(offline.structureSource === 'ui-xml', 'WRONG_RUNTIME_STRUCTURE_SOURCE');
  for (const [name, digest] of Object.entries(offline.fingerprints)) {
    const file = name === 'executable'
      ? path.join(projectRoot, 'output/work/demo/page_examples/V2/d2c_v2.exe')
      : path.join(deployed, name);
    assert(digest === hash(file), `OFFLINE_FINGERPRINT_MISMATCH: ${name}`);
  }

  for (const name of fs.readdirSync(path.join(projectRoot, 'docs/examples/Qt/V2/src')).filter(file => /\.(cpp|h)$/.test(file))) {
    const code = fs.readFileSync(path.join(projectRoot, 'docs/examples/Qt/V2/src', name), 'utf8');
    assert(!/SceneBuilder|LayoutConverter|ComponentConverter|pixTreeNodes|pixComponentTreeDslNodes/.test(code),
      `RUNTIME_DESIGN_PARSER: ${name}`);
  }
  assert(!fs.existsSync(path.join(sourceRoot, '01_DSL')), 'OBSOLETE_DSL_ROUTE_RETAINED');
  assert(!fs.existsSync(path.join(sourceRoot, '02_HTML')), 'AMBIGUOUS_HTML_ROUTE_RETAINED');
  result.status = 'PASS';
  result.geometryTolerancePx = 0;
  result.runtime = 'Static .ui + SDK components; no design parser in the runtime application';
} catch (error) {
  result.status = 'FAIL';
  result.error = String(error);
  process.exitCode = 1;
}

fs.mkdirSync(reports, {recursive: true});
fs.writeFileSync(resultPath, JSON.stringify(result, null, 2));
console.log(JSON.stringify(result));
