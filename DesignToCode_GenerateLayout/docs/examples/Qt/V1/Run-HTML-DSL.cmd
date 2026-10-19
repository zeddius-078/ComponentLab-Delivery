@echo off
REM V1 B2 演示：HTML 布局与 DSL 组件语义已合并到 hybrid-input.json。
setlocal
set "BUNDLE=%~dp0"
if not exist "%BUNDLE%d2c_preview.exe" set "BUNDLE=%~dp0..\..\..\build\demo\page_examples\V1\"
REM 依赖按入口目录定位，运行结果写到用户临时目录。
cd /d "%BUNDLE%"
set QT_PLUGIN_PATH=
set QT_QPA_PLATFORM_PLUGIN_PATH=
set QT_QPA_PLATFORM=windows
start "" "%BUNDLE%d2c_preview.exe" --raw "%BUNDLE%data/hybrid-input.json" --report "%BUNDLE%..\..\reports\V1\hybrid"
