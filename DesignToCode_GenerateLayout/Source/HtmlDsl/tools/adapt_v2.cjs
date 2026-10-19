// V2 B2 路径：递归测量同源 HTML，再合并 DSL 组件语义，输出临时 hybrid-result.json。
// 第一个参数指定临时目录；持久报告写到 Validation/v2/html，原始输入保持原样。
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {pathToFileURL} = require('url');
const {chromium} = require('playwright');
const {outputPath} = require('./output_paths.cjs');
const root = path.resolve(__dirname, '..');
// 要求调用方提供临时目录，确保本工具产生的预览与混合输入有明确的清理归属。
if(!process.argv[2])throw Error('OUTPUT_DIRECTORY_REQUIRED: caller owns temporary output cleanup');
const output = outputPath(process.argv[2]);
// 独立验证可指定报告目录，避免覆盖旧演示已保存的验收证据。
const report = outputPath(process.argv[3] || path.resolve(root, '../../build/reports/v2/html-dsl-adaptation'));
const hash = data => crypto.createHash('sha256').update(data).digest('hex');
const write = (name, data) => fs.writeFileSync(path.join(report, name), JSON.stringify(data, null, 2));
// 清单内的相对路径以 capture.json 所在目录为基准，保证从任意工作目录调用都得到同一输入。
const capturePath = process.argv[4] ? path.resolve(process.argv[4]) : null;
const captureDirectory = capturePath ? path.dirname(capturePath) : null;
const resolveCapturePath = value => {
  if(!value) throw Error('CAPTURE_PATH_REQUIRED');
  return path.isAbsolute(value) ? value : path.resolve(captureDirectory || process.cwd(), value);
};
async function main() {
  fs.mkdirSync(output, {recursive:true});fs.mkdirSync(report, {recursive:true});
  if(!capturePath) throw Error('CAPTURE_MANIFEST_REQUIRED');
  // 清单模式由 Generate.ps1 统一校验；此处只负责读取已经确定的输入路径。
  const capture = JSON.parse(fs.readFileSync(capturePath, 'utf8'));
  const source = fs.readFileSync(resolveCapturePath(capture.sourceHtml), 'utf8');
  const rawBytes = fs.readFileSync(resolveCapturePath(capture.sourceDsl));
  const raw = JSON.parse(rawBytes);
  if(raw.pixTreeNodes?.length!==1)throw Error('EXPECTED_SINGLE_DESIGN_ROOT');
  if(capture && raw.pixTreeNodes[0].guid!==capture.expectedRootId)throw Error('CAPTURE_ROOT_MISMATCH');
  // 计算样式可能包含浏览器默认值；新采集必须提供独立设计属性证据，不能把默认零当成设计决策。
  const referenceBytes = capture && capture.layoutReference ? fs.readFileSync(resolveCapturePath(capture.layoutReference)) : null;
  const reference = referenceBytes ? JSON.parse(referenceBytes) : null;
  const referencePage = reference?.pages.find(p=>p.id===capture.expectedRootId);
  if(reference && (!referencePage || reference.fileKey!==capture.fileKey))throw Error('LAYOUT_REFERENCE_MISMATCH');
  const assets = capture.resources.map(r=>[r.url,resolveCapturePath(r.local)]);
  let html=source;
  const resources=assets.map(([url,local])=>{
    // 替换前核对 URL 仍在原文中，防止资源表过期却继续生成看似完整的离线页面。
    if(!source.includes(url))throw Error('RESOURCE_NOT_IN_SOURCE: '+url);
    html=html.split(url).join(pathToFileURL(local).href);
    return {url,local,sha256:hash(fs.readFileSync(local))};
  });
  const missingScript='<script src="js/animation.js"></script>';
  const removedMissingScript=html.includes(missingScript);html=html.replace(missingScript,'');
  const browser=await chromium.launch({channel:'msedge',headless:true});
  try {
    const viewport={width:Math.ceil(raw.pixTreeNodes[0].width),height:Math.ceil(raw.pixTreeNodes[0].height)};
    const page=await browser.newPage({viewport,deviceScaleFactor:1});
    // 所有临时资源必须已落地；预览测量不能悄悄从网络补取文件。
    await page.route(/^https?:\/\//, route=>route.abort());
    const beforeErrors=[];page.on('pageerror',e=>beforeErrors.push(e.message));
    fs.writeFileSync(path.join(output,'preview.html'),html);
    await page.goto(pathToFileURL(path.join(output,'preview.html')).href);
    await page.evaluate(()=>document.fonts.ready);
    // 当前导出脚本在同一回调重复声明 element_* 常量，先记录原错误再修改预览副本。
    // 替换范围限于这类生成标识符，将 const 改为可重复声明的 var。
    let replacements=0;
    html=html.replace(/const (element_\d+_\d+) = document\.getElementById/g,(_,name)=>{replacements++;return `var ${name} = document.getElementById`;});
    fs.writeFileSync(path.join(output,'preview.html'),html);
    const errors=[],failures=[];
    page.removeAllListeners('pageerror');page.on('pageerror',e=>errors.push(e.message));
    page.on('requestfailed',r=>failures.push({url:r.url(),error:r.failure()?.errorText}));
    await page.reload();await page.evaluate(()=>document.fonts.ready);
    // 仅把节点 ID、类型和层级送入浏览器，以设计树确定需要测量的页面节点。
    const shape=n=>({guid:n.guid,type:n.type,children:(n.childNode||[]).map(shape)});
    const measured=await page.evaluate(({tree,fontFamily})=>{
      function walk(n,parentRect) {
        // 每个设计 ID 必须唯一命中 DOM；坐标转换为父节点局部坐标，拒绝绝对定位。
        const matches=document.querySelectorAll(`[id="${n.guid.replace(/:/g,'_')}"]`);
        // 设计 GUID 必须唯一命中 DOM，缺失或重复都会破坏节点与测量值的对应关系。
        if(matches.length!==1)throw Error('HTML_ID_NOT_UNIQUE: '+n.guid);
        const el=matches[0], r=el.getBoundingClientRect(), s=getComputedStyle(el);
        // 当前转换只处理布局流中的节点，绝对定位无法按相同布局约束复现时立即报错。
        if(s.position==='absolute')throw Error('ABSOLUTE_LAYOUT_NODE: '+n.guid);
        const result={guid:n.guid,rect:[r.x-(parentRect?.x||0),r.y-(parentRect?.y||0),r.width,r.height],visible:s.display!=='none'&&s.visibility!=='hidden'};
        // 容器节点需要读取内部布局并递归测量；组件叶节点保留整体尺寸，避免误把内部装饰当页面子节点。
        if(n.type==='FRAME') {
          const content=[...el.children].find(c=>c.classList.contains('frame-content-'+n.guid.replace(/:/g,'_')))||el;
          const css=getComputedStyle(content);
          const ids=[...content.children].map(c=>c.id.replace(/_/g,':'));
          // 核对数量、唯一性和成员身份，防止 DOM 少节点、多节点或混入另一组件的子项。
          if(ids.length!==n.children.length||new Set(ids).size!==ids.length||ids.some(id=>!n.children.some(c=>c.guid===id)))throw Error('HTML_CHILD_SET_MISMATCH');
          result.layout={display:css.display,direction:css.flexDirection,gap:css.gap,justify:css.justifyContent,align:css.alignItems,
            padding:[css.paddingLeft,css.paddingTop,css.paddingRight,css.paddingBottom],order:ids};
          result.children=ids.map(id=>walk(n.children.find(c=>c.guid===id),r));
        }
        return result;
      }
      const instances=[...document.querySelectorAll('[class*="Pixso-instance-"]')];
      return {tree:walk(tree),instances:instances.length,missingIdentity:instances.filter(el=>!el.hasAttribute('data-component-key')).length,
        cssVariables:[...getComputedStyle(document.documentElement)].filter(k=>k.startsWith('--')).length,
        fontLoaded:document.fonts.check(`14px "${fontFamily}"`)};
    },{tree:shape(raw.pixTreeNodes[0]),fontFamily:capture.fontFamily});
    await page.screenshot({path:path.join(report,'html.png')});
    // 脚本、网络或字体失败会改变测量结果，因此先确认页面可用，再生成混合输入。
    if(errors.length||failures.length||!measured.fontLoaded)throw Error('HTML_RENDER_FAILED: '+JSON.stringify({errors,failures}));
    const bindings=[];
    // 仅接收明确的像素长度，拒绝 auto、百分比等需要额外求值规则的输入。
    const px=value=>{if(!/^\d+(\.\d+)?px$/.test(value))throw Error('UNSUPPORTED_CSS_LENGTH: '+value);return parseFloat(value);};
    const alignment=value=>{
      // 把当前横纵 flex 场景的默认起始对齐统一为 flex-start，供布局引擎读取。
      if(value==='normal'||value==='start')return 'flex-start';
      // 其余对齐值必须属于已有映射范围；遇到新枚举直接报错，避免静默改成默认布局。
      if(!['flex-start','flex-end','center'].includes(value))throw Error('UNSUPPORTED_ALIGNMENT: '+value);return value;
    };
    function adapt(n,m) {
      // 保存 HTML 与 DSL 几何差值，再只覆盖已声明的布局、尺寸、可见性和顺序。
      const original=[n.left||0,n.top||0,n.width,n.height];
      bindings.push({guid:n.guid,htmlRect:m.rect,dslRect:original,maxDelta:Math.max(...original.map((v,i)=>Math.abs(v-m.rect[i])))});
      n.width=m.rect[2];n.height=m.rect[3];n.visible=m.visible;
      // 只给容器覆盖布局字段并按测量树递归；叶节点的组件身份与属性继续沿用原 DSL。
      if(n.type==='FRAME') {
        const s=m.layout;
        if(referencePage) {
          const evidence=referencePage.nodes.find(x=>x.id===n.guid)?.metrics;
          const actual={gap:px(s.gap),left:px(s.padding[0]),top:px(s.padding[1]),right:px(s.padding[2]),bottom:px(s.padding[3])};
          for(const [key,value] of Object.entries(actual)) {
            if(!evidence || !Number.isFinite(evidence[key]))throw Error('MISSING_DESIGN_LAYOUT_EVIDENCE: '+n.guid+'/'+key);
            if(value!==evidence[key])throw Error('DESIGN_HTML_LAYOUT_MISMATCH: '+n.guid+'/'+key);
          }
        }
        // 当前适配规则只覆盖 row/column flex，先拒绝其他模型，再写入确定的横纵布局参数。
        if(s.display!=='flex'||!['row','column'].includes(s.direction))throw Error('UNSUPPORTED_FLEX');
        n.autoLayout={...n.autoLayout,stackMode:s.direction==='row'?'HORIZONTAL':'VERTICAL',autoLayoutItemSpacing:px(s.gap),
          autoLayoutPrimaryAlign:alignment(s.justify),autoLayoutCounterAlign:alignment(s.align),
          autoLayoutPaddingLeft:px(s.padding[0]),autoLayoutPaddingTop:px(s.padding[1]),autoLayoutPaddingRight:px(s.padding[2]),autoLayoutPaddingBottom:px(s.padding[3])};
        n.childNode=m.children.map(c=>adapt(n.childNode.find(x=>x.guid===c.guid),c)).reverse();
      }
      return n;
    }
    raw.pixTreeNodes=[adapt(raw.pixTreeNodes[0],measured.tree)];
    // 混合文件显式携带双来源哈希，生成器可将其继续写入中立设计树。
    raw._provenance={sourceDslSha256:hash(rawBytes),sourceHtmlSha256:hash(source),layout:'HTML computed layout',identity:'Pixso full DSL',
      ...(referenceBytes ? {layoutReferenceSha256:hash(referenceBytes),layoutVerifiedAgainst:'Pixso Plugin API explicit metrics'} : {})};
    const derived=JSON.stringify(raw);fs.writeFileSync(path.join(output,'hybrid-result.json'),derived);
    write('html-result.json',{status:'PASS',route:'B2: recursive HTML layout + explicit DSL identity',
      componentIdentitySource:'Pixso full DSL',
      htmlInstancesWithoutIdentity:measured.missingIdentity,
      sourceHtmlSha256:hash(source),sourceDslSha256:hash(rawBytes),outputSha256:hash(derived),
      viewport,layoutReferenceSha256:referenceBytes ? hash(referenceBytes) : null,
      transformations:{resources,removedMissingScript,duplicateConstDeclarationsLowered:replacements},originalScriptErrors:beforeErrors,
      errors,failures,inspection:measured,bindings,
      fieldSources:{layout:'HTML computed styles / DOM order / dimensions',sizingModes:'original DSL',componentIdentity:'original DSL',propertiesAndSlots:'original DSL',visuals:'existing Qt Token/QSS'}});
    console.log(JSON.stringify({status:'PASS',nodes:bindings.length,maxGeometryDelta:Math.max(...bindings.map(b=>b.maxDelta)),originalScriptErrors:beforeErrors}));
  } finally {await browser.close();}
}
// 失败时覆盖为 FAIL，避免上次成功的报告被当成本次结果。
main().catch(e=>{fs.mkdirSync(report,{recursive:true});write('html-result.json',{status:'FAIL',error:String(e)});console.error(e);process.exitCode=1;});
