@echo off
setlocal
set "QT_PLUGIN_PATH="
set "QT_QPA_PLATFORM_PLUGIN_PATH="
cd /d "%~dp0"
start "" "%~dp0component_lab.exe" %*
endlocal
