# V1 离线交付：构建、测试并生成 HTML + DSL 合并输入。
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../Source/NativeTools.ps1"
Initialize-NativeTools -NeedNode
$out = Join-Path $D2CProjectRoot 'build/demo/page_examples/V1'
[void](New-Item -ItemType Directory -Force $out)
Copy-Item -Path "$PSScriptRoot/Qt/V1/Run-*.cmd" -Destination $out
# 两个入口分别展示常规页与工单页，均消费同一条 HTML + DSL 生成链。
foreach ($launcher in @('Run-HTML-DSL.cmd', 'Run-Holdout-HTML-DSL.cmd')) {
    # 逐项要求维护的启动入口存在，避免打包时默默丢掉某一演示路径。
    if (-not (Test-Path -LiteralPath (Join-Path $out $launcher) -PathType Leaf)) {
        throw "Missing maintained V1 launcher: $out/$launcher"
    }
}
& "$D2CSourceRoot/Build.ps1" -Test -Preview -BuildDirectory "$D2CProjectRoot/build/cache/V1"
$scratch=[IO.Path]::GetFullPath("$D2CProjectRoot/build/cache/DesignToCodeV1-transient-"+[guid]::NewGuid().ToString('N'))
# HTML 派生数据在唯一临时目录生成，主线输入与原始设计响应保持原样。
$node=$NativeNode
try {
    & $node "$D2CSourceRoot/HtmlDsl/tools/html_v1.cjs" --raw-dir "$D2CProjectRoot/docs/fixtures/layout/html-dsl/node-12-382" --output-dir "$scratch/pilot" --dsl "$D2CProjectRoot/docs/fixtures/layout/design-inputs/node-12-382-full/canonical.raw.json" --report-dir "$scratch/pilot-report"
    # 样例转换失败时停止部署，防止旧混合输入被复制进新包。
    if($LASTEXITCODE -ne 0){throw 'V1 pilot HTML conversion failed'}
    & $node "$D2CSourceRoot/HtmlDsl/tools/html_v1.cjs" --raw-dir "$D2CProjectRoot/docs/fixtures/layout/html-dsl/holdout-19-45" --output-dir "$scratch/holdout" --dsl "$D2CProjectRoot/docs/fixtures/layout/design-inputs/holdout-19-45-full/canonical.raw.json" --report-dir "$scratch/holdout-report" --width 600 --height 560
    # 独立页面也必须完成 HTML 转换，确保四个启动入口对应的数据齐全。
    if($LASTEXITCODE -ne 0){throw 'V1 holdout HTML conversion failed'}
# 部署需整目录分发：data 是设计输入，mapping 是规则，fonts 是显式字体依赖。
New-Item -ItemType Directory -Force $out, "$out/data", "$out/mapping", "$out/fonts" | Out-Null
Copy-Item -LiteralPath "$D2CProjectRoot/build/cache/V1/d2c_preview.exe" -Destination $out
Copy-Item -LiteralPath "$scratch/pilot/hybrid-input.json" -Destination "$out/data"
Copy-Item -LiteralPath "$scratch/holdout/hybrid-input.json" -Destination "$out/data/holdout-hybrid-input.json"
Copy-Item -LiteralPath "$D2CProjectRoot/docs/fixtures/layout/mapping/component-mapping.json", "$D2CProjectRoot/docs/fixtures/layout/mapping/layout-mapping.json" -Destination "$out/mapping"
Copy-Item -LiteralPath "$D2CProjectRoot/docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf" -Destination "$out/fonts"
# Qt 插件由部署工具复制，MSVC 运行库和许可随后单独加入。
& "$env:QT_ROOT/bin/windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime "$out/d2c_preview.exe"
# 依赖部署失败时停止发布目录输出，避免把缺少 Qt 插件的包交给使用者。
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed' }
Copy-Item "$env:VCToolsRedistDir/x64/Microsoft.VC*.CRT/*.dll" -Destination $out
Copy-Item -LiteralPath "$PSScriptRoot/Qt/V1/qt.conf" -Destination $out
Copy-GeneratedComponentRuntime -Destination $out
Write-Output $out
} finally {
    # 清理前核对解析后的父目录、名称前缀和链接属性，只删除本次临时目录。
    if(Test-Path -LiteralPath $scratch) {
        $resolved=(Resolve-Path -LiteralPath $scratch).Path
        # 核对解析后的父目录和专用名称前缀，保证递归删除只作用于本次临时输出。
        if([IO.Path]::GetDirectoryName($resolved) -ne [IO.Path]::GetFullPath("$D2CProjectRoot/build/cache") -or [IO.Path]::GetFileName($resolved) -notlike 'DesignToCodeV1-transient-*'){throw 'Unsafe temporary path'}
        # 临时路径含重解析属性时拒绝清理，避免沿链接删除其他位置的内容。
        if((Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Temporary path is a link'}
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
