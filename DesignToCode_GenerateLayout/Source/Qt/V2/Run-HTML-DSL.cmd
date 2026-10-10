@echo off
REM HTML + DSL 路线入口：HTML 提供画面测量，DSL 提供组件身份和 Token 绑定。
setlocal
set QT_PLUGIN_PATH=
set QT_QPA_PLATFORM_PLUGIN_PATH=
set QT_QPA_PLATFORM=windows
cd /d "%~dp0"
REM 同时支持在源码目录和离线包目录启动，数据始终与选定路线对应。
if exist "%~dp0d2c_v2.exe" (
  start "" "%~dp0d2c_v2.exe" --data "%~dp0data/html-dsl" %*
) else (
  start "" "%~dp0..\..\..\build\demo\page_examples\V2\d2c_v2.exe" --data "%~dp0..\..\..\build\demo\page_examples\V2\data/html-dsl" %*
)
