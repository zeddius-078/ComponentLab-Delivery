// V1 HTML + DSL 适配工具：HTML 提供浏览器计算布局，DSL 提供组件身份、状态和槽位。
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {pathToFileURL} = require('url');
const {chromium} = require('playwright');
const {outputPath} = require('./output_paths.cjs');

async function main() {
  // 调用方指定并清理临时输出目录；报告可通过 --report-dir 单独保存。
  const root = path.resolve(__dirname, '..');
  const args=process.argv.slice(2);
  const option=name=>args.includes(name)?args[args.indexOf(name)+1]:null;
  if(!option('--raw-dir'))throw Error('RAW_DIRECTORY_REQUIRED');
  const raw = path.resolve(option('--raw-dir'));
  const dslOption = option('--dsl');
  if(!dslOption)throw Error('DSL_INPUT_REQUIRED');
  // 临时文件位置由调用方提供，以便构建结束后统一清理，而不会散落在来源目录。
  if(!option('--output-dir'))throw Error('OUTPUT_DIRECTORY_REQUIRED: caller owns temporary output cleanup');
  const out = outputPath(option('--output-dir'));
  const reportDir = outputPath(option('--report-dir')||path.join(out, 'validation'));
  fs.mkdirSync(out, {recursive:true}); fs.mkdirSync(reportDir, {recursive:true});
  const original = fs.readFileSync(path.join(raw, 'page-0-0.html'), 'utf8');
  const assets = JSON.parse(fs.readFileSync(path.join(raw,'resource-map.json'),'utf8'));
  let html = original;
  const localizedAssets=[];
  // 根据资源表将临时 URL 换成本地文件 URL。字体直接复用，图片复制到临时目录。
  for(const [url,destination,sourceRelative] of assets) {
    const source=path.resolve(raw,sourceRelative||path.join('assets',path.basename(destination)));
    const font=/\.(ttf|otf|woff2?)$/i.test(source);
    const target=font?source:outputPath(path.join(out,destination));
    if(!font && (path.relative(out,target).startsWith('..') || path.isAbsolute(path.relative(out,target))))throw Error('RESOURCE_OUTPUT_ESCAPE');
    // 图片按导出的相对路径复制到预览目录；字体直接引用已保存文件，减少大文件重复复制。
    if(!font) {
      fs.mkdirSync(path.dirname(target),{recursive:true});
      fs.copyFileSync(source,target);
    }
    const local=pathToFileURL(target).href;
    html=html.split(url).join(local);
    localizedAssets.push({url,path:local,source,
      sha256:crypto.createHash('sha256').update(fs.readFileSync(source)).digest('hex')});
  }
  // 导出清单未提供 animation.js，仅在预览副本移除这条缺失引用，原始 HTML 保留。
  const missingScript = '<script src="js/animation.js"></script>';
  const removedMissingScript = html.includes(missingScript);
  html=html.replace(missingScript,'');
  fs.writeFileSync(path.join(out,'preview.html'),html);
  const browser = await chromium.launch({headless:true,channel:'msedge'});
  try {
    const width=Number(option('--width')||720),height=Number(option('--height')||600);
    // 视口必须为正整数像素，避免无效尺寸进入浏览器并导致测量结果不可比较。
    if(!Number.isInteger(width)||!Number.isInteger(height)||width<1||height<1)throw Error('INVALID_VIEWPORT');
    const page=await browser.newPage({viewport:{width,height},deviceScaleFactor:1});
    const failures=[];
    page.on('requestfailed',r=>failures.push({url:r.url(),error:r.failure()?.errorText}));
    await page.goto(pathToFileURL(path.join(out,'preview.html')).href);
    await page.evaluate(()=>document.fonts.ready);
    // 在浏览器中检查实际 DOM 属性和字体；组件身份由同源 DSL 提供。
    const inspection=await page.evaluate(()=>{
      const instances=[...document.querySelectorAll('[class*="Pixso-instance-"]')];
      const native=[...document.querySelectorAll('button,input,select,textarea,[role="button"],[role="checkbox"]')];
      return {
        instances:instances.map(el=>({
          tag:el.tagName,attributes:Object.fromEntries([...el.attributes].map(a=>[a.name,a.value])),
          text:el.textContent.trim(),
          layout:{display:getComputedStyle(el).display,direction:getComputedStyle(el).flexDirection,
            gap:getComputedStyle(el).gap,padding:getComputedStyle(el).padding},
          componentIdentity:el.getAttribute('data-component-key')||el.getAttribute('data-component-name'),
          variant:el.getAttribute('data-variant'),
          role:el.getAttribute('role')
        })),
        nativeControlCount:native.length,
        cssVariables:[...getComputedStyle(document.documentElement)].filter(k=>k.startsWith('--')),
        text:document.body.innerText,
        fontLoaded:document.fonts.check('14px "HarmonyOS Sans SC"')
      };
    });
    await page.screenshot({path:path.join(reportDir,'html.png')});
    const missingHtmlIdentity=inspection.instances.filter(n=>!n.componentIdentity);
    const report={
      route:'HTML + DSL',
      rawSha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(raw,'page-0-0.html'))).digest('hex'),
      sourceDslSha256:crypto.createHash('sha256').update(fs.readFileSync(dslOption)).digest('hex'),
      outputSha256:crypto.createHash('sha256').update(html).digest('hex'),
      visualRender:failures.length===0&&inspection.fontLoaded?'PASS':'FAIL',
      componentIdentitySource:'canonical DSL',
      htmlInstancesWithoutIdentity:missingHtmlIdentity.length,
      transformations:{localizedAssets,removedMissingScript},
      failures,inspection,
      decision:'Use HTML only for measured layout and canonical DSL for component identity, variants, states and slots.'
    };
    fs.writeFileSync(path.join(reportDir,'html-result.json'),JSON.stringify(report,null,2));
    console.log(JSON.stringify({visualRender:report.visualRender,componentIdentitySource:report.componentIdentitySource,
      instances:inspection.instances.length,nativeControls:inspection.nativeControlCount,variables:inspection.cssVariables.length}));
    // 同源 DSL 补齐 HTML 未保留的组件身份和变体。
    {
      // V1 限定单根单层页面；DOM ID 只用于关联同源节点，控件类型从 DSL 取得。
      const source=fs.readFileSync(dslOption);
      const dsl=JSON.parse(source);
      const roots=dsl.pixTreeNodes;
      // V1 按单根页面计算布局，根数量不符时停止，避免任意挑选节点造成错误关联。
      if(!Array.isArray(roots)||roots.length!==1)throw Error('HYBRID_REQUIRES_ONE_ROOT');
      const originalRoot=roots[0];
      const htmlLayout=await page.evaluate(guid=>{
        const element=document.getElementById(guid.replace(/:/g,'_'));
        // 先确认设计根 GUID 在页面中存在，再读取尺寸；缺失时无法建立同源布局对应。
        if(!element)throw Error('SOURCE_ROOT_NOT_IN_HTML');
        const container=[...element.children].find(n=>[...n.classList].some(c=>c.startsWith('frame-content-'))) || element;
        const style=getComputedStyle(container);
        const rect=element.getBoundingClientRect();
        const size=n=>{
          const r=n.getBoundingClientRect(),s=getComputedStyle(n);
          return {guid:n.id.replace(/_/g,':'),width:r.width,height:r.height,
            visible:s.display!=='none'&&s.visibility!=='hidden',position:s.position};
        };
        return {width:rect.width,height:rect.height,display:style.display,direction:style.flexDirection,
          gap:style.gap,justify:style.justifyContent,align:style.alignItems,
          padding:[style.paddingLeft,style.paddingTop,style.paddingRight,style.paddingBottom],
          children:[...container.children].map(size)};
      },originalRoot.guid);
      // 本路线布局规则仅支持横向和纵向 flex；其他 CSS 布局需要新增明确适配规则后才能处理。
      if(htmlLayout.display!=='flex'||!['row','column'].includes(htmlLayout.direction))throw Error('UNSUPPORTED_HTML_LAYOUT');
      const nodes=new Map(originalRoot.childNode.map(n=>[n.guid,n]));
      const bindings=[];
      const children=htmlLayout.children.map(n=>{
        const node=nodes.get(n.guid);
        // 每个 DOM 子项必须命中受支持的 DSL 叶节点，并参与正常布局流，否则停止转换。
        if(!node||!['INSTANCE','PARAGRAPH'].includes(node.type)||n.position==='absolute')throw Error('UNMAPPED_HTML_CHILD');
        bindings.push({htmlId:n.guid.replace(/:/g,'_'),dslGuid:node.guid,
          componentSetRef:node.mainStateGroup||null,identitySource:'DSL',layoutSource:'HTML computed style'});
        return {...node,width:n.width,height:n.height,visible:n.visible};
      });
      // 核对全部子项已一一覆盖，防止重复 DOM ID 或遗漏节点造成不完整页面。
      if(children.length!==nodes.size||new Set(bindings.map(n=>n.dslGuid)).size!==nodes.size)throw Error('HTML_DSL_CHILD_MISMATCH');
      // 只接受已支持的像素长度与对齐枚举，百分比/自动值等不会隐式猜测。
      const px=value=>{
        // 只解析明确的非负像素值，让输出间距和内边距具有可重现的单位含义。
        if(!/^\d+(\.\d+)?px$/.test(value))throw Error('UNSUPPORTED_CSS_LENGTH: '+value);
        return Number.parseFloat(value);
      };
      const alignment=value=>{
        // 把本次 flex 布局中的默认起始对齐归一为 flex-start，统一后续映射输入。
        if(value==='normal'||value==='start')return 'flex-start';
        // 对齐方式必须有对应的 Qt 规则，超出枚举时显式失败，避免外观被悄悄改写。
        if(!['flex-start','center','flex-end'].includes(value))throw Error('UNSUPPORTED_HTML_ALIGNMENT: '+value);
        return value;
      };
      // 浏览器尺寸覆盖输入副本；反转 DOM 顺序以符合本路线布局引擎的 childNode 约定。
      dsl.pixTreeNodes=[{...originalRoot,width:htmlLayout.width,height:htmlLayout.height,
        autoLayout:{stackMode:htmlLayout.direction==='row'?'HORIZONTAL':'VERTICAL',
          autoLayoutItemSpacing:px(htmlLayout.gap),
          autoLayoutPrimaryAlign:alignment(htmlLayout.justify),autoLayoutCounterAlign:alignment(htmlLayout.align),
          autoLayoutPaddingLeft:px(htmlLayout.padding[0]),autoLayoutPaddingTop:px(htmlLayout.padding[1]),
          autoLayoutPaddingRight:px(htmlLayout.padding[2]),autoLayoutPaddingBottom:px(htmlLayout.padding[3])},
        childNode:children.reverse()}];
      const derived=JSON.stringify(dsl);
      fs.writeFileSync(path.join(out,'hybrid-input.json'),derived);
      // 保留每个字段组的来源及输入/适配器哈希；ADAPTED 还需原生编译运行验证。
      fs.writeFileSync(path.join(reportDir,'hybrid-adaptation.json'),JSON.stringify({
        route:'B2: HTML layout + explicit DSL component metadata',
        sourceDslSha256:crypto.createHash('sha256').update(source).digest('hex'),
        sourceHtmlSha256:report.rawSha256,bindings,htmlLayout,
        outputSha256:crypto.createHash('sha256').update(derived).digest('hex'),
        adapterSha256:crypto.createHash('sha256').update(fs.readFileSync(__filename)).digest('hex'),
        fieldSources:{
          rootLayout:'HTML computed flex style',rootDimensions:'HTML bounding box',
          directChildOrder:'HTML DOM order',directChildDimensionsAndVisibility:'HTML computed layout',
          componentIdentityVariantsSlotsAndResources:'canonical DSL',childSizingModesAndOverrides:'canonical DSL',
          paragraphStyleAndRootFill:'canonical DSL'
        },
        status:'ADAPTED; native compile/run verification required',
        limitation:'Child fixed dimensions from HTML; full responsive CSS and nested layouts are not implemented'
      },null,2));
    }
    // 报告保存后同步设置失败退出码，使外层构建能够阻止使用渲染失败的输入。
    if(report.visualRender!=='PASS')process.exitCode=1;
  } finally {await browser.close();}
}
main().catch(e=>{console.error(e);process.exitCode=1;});
