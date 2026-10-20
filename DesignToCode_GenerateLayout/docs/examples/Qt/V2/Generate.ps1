# 单一路线入口：HTML 提供可测量画面，DSL 提供组件身份、状态和 Token 绑定。
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath("$PSScriptRoot/../../../../Source")
$resultManifest = "$root/../docs/fixtures/layout/html-dsl/result-page/capture.json"
$reviewManifest = "$root/../docs/fixtures/layout/html-dsl/holdout-19-45/capture.json"
# 预览回归也通过明确清单选择输入，避免入口隐式读取固定历史样例。
& "$root/HtmlDsl/Generate.ps1" -Preview -CaptureManifest $resultManifest -ReviewCaptureManifest $reviewManifest -MappingDirectory "$root/../docs/fixtures/layout/mapping" -Font "$root/../docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"
Copy-Item "$root/../docs/examples/templates/instrument_host/instrument_host.*" -Destination "$root/../output/work/preview/html-dsl"
