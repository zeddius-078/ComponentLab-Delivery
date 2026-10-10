// 下载工具返回的页面与其资源。原始 HTML 保持原样，记录 URL、状态与字节摘要。
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const root = path.resolve(__dirname, '..');
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
async function download(url, target) {
  const address = new URL(url);
  // 当前采集使用本机 Pixso 服务；拒绝非预期来源，避免下载设计文本中的任意地址。
  if (!['localhost', '127.0.0.1'].includes(address.hostname) || address.port !== '3667')
    throw Error('UNEXPECTED_DOWNLOAD_ORIGIN: ' + url);
  const response = await fetch(url);
  // HTTP 失败立即终止，缺资源的页面不能用于后续几何测量。
  if (!response.ok) throw Error('DOWNLOAD_FAILED: ' + response.status + ' ' + url);
  const bytes = Buffer.from(await response.arrayBuffer());
  fs.mkdirSync(path.dirname(target), {recursive: true});
  fs.writeFileSync(target, bytes);
  return {url, file: path.relative(root, target).replace(/\\/g, '/'),
    status: response.status, contentType: response.headers.get('content-type'),
    bytes: bytes.length, sha256: hash(bytes)};
}
async function main() {
  const text = fs.readFileSync(path.join(root, 'inputs/html/content-00.txt'), 'utf8');
  const manifest = JSON.parse(text.slice(text.indexOf('{')));
  // 单画板实验只接收一个入口；新增返回类型须先明确保存规则。
  if (manifest.pageEntries?.length !== 1) throw Error('EXPECTED_ONE_HTML_ENTRY');
  const page = await download(manifest.pageEntries[0].url, path.join(root, 'inputs/html/page.html'));
  const html = fs.readFileSync(path.join(root, page.file), 'utf8');
  const urls = [...new Set(html.match(/https?:\/\/[^\s"'<>()[\]]+/g) || [])];
  const assets = [], names = new Set();
  // 每个 HTTP 资源独立下载并记录；文件名冲突时停止，防止资源被覆盖。
  for (const url of urls) {
    const name = decodeURIComponent(new URL(url).pathname.split('/').pop());
    if (!name || names.has(name)) throw Error('ASSET_NAME_COLLISION: ' + name);
    names.add(name);
    assets.push(await download(url, path.join(root, 'inputs/assets', name)));
  }
  fs.writeFileSync(path.join(root, 'inputs/downloads.json'),
    JSON.stringify({capturedAtUtc: new Date().toISOString(), page, assets}, null, 2));
  console.log(JSON.stringify({page: page.file, assets: assets.map(a => a.file)}));
}
main().catch(e => { console.error(e); process.exitCode = 1; });
