@echo off
REM 根据启动脚本所在位置定位离线程序，隔离系统中其他 Qt 插件设置。
setlocal
set QT_PLUGIN_PATH=
set QT_QPA_PLATFORM_PLUGIN_PATH=
set QT_QPA_PLATFORM=windows
cd /d "%~dp0"
if exist "%~dp0case_27_450.exe" (
  start "" "%~dp0case_27_450.exe" %*
) else (
  start "" "%~dp0..\..\..\..\build\demo\page_examples\Case-27-450\case_27_450.exe" %*
)
