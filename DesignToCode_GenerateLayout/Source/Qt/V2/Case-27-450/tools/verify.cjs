// 只核对本案例保存的输入与输出，不连接 Pixso，不修改设计数据和组件规则。
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const assert = require('assert/strict');
const root = path.resolve(__dirname, '..');
const read = name => fs.readFileSync(path.join(root, name));
const json = name => JSON.parse(read(name).toString('utf8').replace(/^\uFEFF/, ''));
const hash = data => crypto.createHash('sha256').update(data).digest('hex');
const sha = name => hash(read(name));

function rpcResult(name, id) {
  const text = read(name).toString('utf8');
  // 服务可能返回 JSON 或 SSE，解包仅用于比较，原始响应字节继续保留。
  const messages = text.trimStart().startsWith('{') ? [JSON.parse(text)] :
    text.replace(/\r\n/g, '\n').split('\n\n').map(event =>
      event.split('\n').filter(line => line.startsWith('data:'))
        .map(line => line.slice(5).trimStart()).join('\n'))
      .filter(Boolean).map(value => JSON.parse(value));
  const matches = messages.filter(message => message.id === id);
  assert.equal(matches.length, 1, `RPC_RESPONSE_ID: ${name}`);
  assert.ok(!matches[0].error && !matches[0].result.isError, `RPC_ERROR: ${name}`);
  return matches[0].result;
}

function verify() {
  const sourceHashes = {};
  // 分别核对三次采集会话的 HTTP 正文与文本提取文件，避免只验证本地派生 JSON。
  for (const directory of ['html', 'dsl', 'context']) {
    const prefix = `inputs/${directory}`;
    for (const label of ['01-initialize', '02-initialized', '03-tools', '04-call']) {
      const name = `${prefix}/${label}.response.body`;
      const metadata = json(`${prefix}/${label}.response-meta.json`);
      assert.equal(sha(name), metadata.sha256, `CAPTURE_HASH: ${name}`);
      assert.equal(read(name).length, metadata.bytes, `CAPTURE_SIZE: ${name}`);
      sourceHashes[name] = sha(name);
    }
    const response = rpcResult(`${prefix}/04-call.response.body`, 4);
    // 文本块按响应索引逐个比较，证明提取时没有重写设计字段。
    response.content.forEach((block, index) => {
      if (block.type !== 'text') return;
      const stem = directory === 'dsl' ? 'canonical.raw' : `content-${String(index).padStart(2, '0')}`;
      const file = ['.json', '.txt'].map(ext => `${prefix}/${stem}${ext}`)
        .find(name => fs.existsSync(path.join(root, name)));
      assert.ok(file, `CONTENT_MISSING: ${stem}`);
      assert.deepEqual(read(file), Buffer.from(block.text, 'utf8'), `CONTENT_CHANGED: ${file}`);
      sourceHashes[file] = sha(file);
    });
  }
  const contract = json('experiment.json');
  const context = json('inputs/context/content-00.json');
  assert.equal(context.fileKey, contract.requestedFileKey);
  assert.equal(context.node.id, contract.rootGuid);
  const dsl = json('inputs/dsl/canonical.raw.json');
  assert.equal(dsl.pixTreeNodes[0].guid, contract.rootGuid);
  const htmlRequest = json('inputs/html/04-call.request.json');
  assert.equal(htmlRequest.params.name, 'design_to_code');
  assert.deepEqual(htmlRequest.params.arguments.guids, [contract.rootGuid]);
  assert.equal(htmlRequest.params.arguments.clientFrameworks, 'html');
  const dslRequest = json('inputs/dsl/04-call.request.json');
  assert.equal(dslRequest.params.name, 'get_node_dsl');
  assert.equal(dslRequest.params.arguments.guid, contract.rootGuid);
  assert.equal(dslRequest.params.arguments.simplify, false);

  const downloads = json('inputs/downloads.json');
  // 下载清单绑定网页及其字体、图标的实际字节，不以文件名作为一致性依据。
  for (const item of [downloads.page, ...downloads.assets]) {
    assert.equal(sha(item.file), item.sha256, `DOWNLOAD_HASH: ${item.file}`);
    sourceHashes[item.file] = sha(item.file);
  }
  const html = json('../../../../build/reports/Case-27-450/html-result.json');
  const qt = json('../../../../build/reports/Case-27-450/qt-result.json');
  const generation = json('../../../../build/reports/Case-27-450/generation.json');
  assert.equal(html.status, 'PASS');
  assert.equal(qt.status, 'PASS');
  assert.equal(generation.status, 'PASS');
  assert.equal(html.sourceHtmlSha256, sha('inputs/html/page.html'));
  assert.equal(html.sourceDslSha256, sha('inputs/dsl/canonical.raw.json'));
  assert.equal(html.bindings.length, 14);
  assert.equal(qt.comparisons.length, html.bindings.length);
  assert.deepEqual(qt.comparisons.map(x => x.nodeId).sort(), html.bindings.map(x => x.guid).sort());
  assert.equal(qt.maxGeometryDelta, 0);
  assert.equal(qt.interactions.length, 10);
  assert.ok(qt.interactions.every(x => x.passed));
  assert.equal(qt.platform, 'windows');
  assert.equal(qt.largerViewportContained, true);
  const generatedHashes = {};
  // 同时核对生成目录与离线 data，防止交付包仍包含上次生成的页面。
  for (const [ext, expectedHash] of Object.entries(generation.hashes)) {
    const file = `generated/page${ext}`;
    assert.equal(sha(file), expectedHash.toLowerCase());
    assert.equal(sha(file), sha(`../../../../build/demo/page_examples/Case-27-450/data/page${ext}`));
    generatedHashes[file] = sha(file);
  }
  const dependencies = {};
  const pipeline = path.resolve(root, '../../../HtmlDsl/pipeline');
  function recordDirectory(directory) {
    // 记录实际只读依赖的源码和规则，供以后判断同一输入能否在同一规则下重放。
    for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
      const full = path.join(directory, entry.name);
      if (entry.isDirectory()) recordDirectory(full);
      else dependencies[path.relative(root, full).replace(/\\/g, '/')] = hash(fs.readFileSync(full));
    }
  }
  for (const folder of ['mapping', 'engines', 'compiler', 'runtime']) {
    recordDirectory(path.join(pipeline, folder));
  }
  const report = {
    status: 'PASS', integration: 'ADAPTED', verifiedAtUtc: new Date().toISOString(),
    source: {fileKey: context.fileKey, nodeId: context.node.id, nodeName: context.node.name,
      sourceUrl: contract.sourceUrl, endpointUsed: 'http://127.0.0.1:3667/mcp',
      identityEvidence: 'read-only eval_script response; local hashes are not a server signature'},
    screenshot: {file: 'inputs/pixso.png', sha256: sha('inputs/pixso.png'),
      method: 'MCP get_screenshot({guid:"27:450"}); image content base64 decoded without pixel changes',
      transportBodySaved: false},
    checks: {capturedSessions: 3, measuredNodes: html.bindings.length,
      maxHtmlDslGeometryDelta: Math.max(...html.bindings.map(x => x.maxDelta)),
      maxQtHtmlGeometryDelta: qt.maxGeometryDelta,
      buttons: qt.interactions.filter(x => x.kind === 'button').length,
      disabledOptions: qt.interactions.filter(x => x.kind === 'disabled-option').length,
      interactionsPassed: qt.interactions.filter(x => x.passed).length,
      identicalGenerationRuns: generation.repeatRuns, nativePlatform: qt.platform,
      largerViewportContained: qt.largerViewportContained},
    limitations: ['Business navigation and submission services are not defined by this design.',
      'Geometry equality does not establish pixel-perfect rendering or general responsive support.',
      'Pixso export itself is an upstream tool; this case proves replay of saved inputs, not determinism of future exports.'],
    sourceHashes, generatedHashes, dependencies,
    executable: {file: '../../../../build/demo/page_examples/Case-27-450/case_27_450.exe', sha256: sha('../../../../build/demo/page_examples/Case-27-450/case_27_450.exe')}
  };
  fs.writeFileSync(path.join(root, '../../../../build/reports/Case-27-450/verification.json'), JSON.stringify(report, null, 2));
  console.log(JSON.stringify({status: report.status, checks: report.checks}));
}

// 失败也覆盖汇总文件，防止旧的 PASS 在下一轮失败后继续被展示。
try { verify(); }
catch (error) {
  fs.writeFileSync(path.join(root, '../../../../build/reports/Case-27-450/verification.json'), JSON.stringify({status: 'FAIL', error: String(error)}, null, 2));
  console.error(error);
  process.exitCode = 1;
}
