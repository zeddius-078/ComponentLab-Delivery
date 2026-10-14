# 单一路线入口：HTML 提供可测量画面，DSL 提供组件身份、状态和 Token 绑定。
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$resultManifest = "$root/HtmlDsl/inputs/result-page/capture.json"
$reviewManifest = "$root/HtmlDsl/inputs/holdout-19-45/capture.json"
# 预览回归也通过明确清单选择输入，避免入口隐式读取固定历史样例。
& "$root/HtmlDsl/Generate.ps1" -Preview -CaptureManifest $resultManifest -ReviewCaptureManifest $reviewManifest
