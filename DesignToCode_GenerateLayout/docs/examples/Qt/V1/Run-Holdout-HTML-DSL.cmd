@echo off
REM 工单复核的 B2 对照入口，读取 holdout-hybrid-input.json。
setlocal
set "BUNDLE=%~dp0"
if not exist "%BUNDLE%d2c_preview.exe" set "BUNDLE=%~dp0..\..\..\build\demo\page_examples\V1\"
REM 与原始 DSL 共用同一 EXE，只替换输入和临时报告目录。
cd /d "%BUNDLE%"
set QT_PLUGIN_PATH=
set QT_QPA_PLATFORM_PLUGIN_PATH=
set QT_QPA_PLATFORM=windows
start "" "%BUNDLE%d2c_preview.exe" --raw "%BUNDLE%data/holdout-hybrid-input.json" --report "%BUNDLE%..\..\reports\V1\holdout-hybrid"
