@echo off
REM V2 启动入口：支持放在源码根目录或部署目录，其他参数原样传给 EXE。
setlocal
REM 清空外部 Qt 插件环境，使用随包部署的 Windows 平台插件。
set QT_PLUGIN_PATH=
set QT_QPA_PLATFORM_PLUGIN_PATH=
set QT_QPA_PLATFORM=windows
cd /d "%~dp0"
REM 优先启动同目录 EXE，源码目录下则转到 Build。
if exist "%~dp0d2c_v2.exe" (
  start "" "%~dp0d2c_v2.exe" %*
) else (
  start "" "%~dp0..\..\..\build\demo\page_examples\V2\d2c_v2.exe" %*
)
