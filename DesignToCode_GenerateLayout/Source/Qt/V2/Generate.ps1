# 单一路线入口：HTML 提供可测量画面，DSL 提供组件身份、状态和 Token 绑定。
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$root/HtmlDsl/Generate.ps1" -Preview
