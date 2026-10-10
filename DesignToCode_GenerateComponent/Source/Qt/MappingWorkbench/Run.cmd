@echo off
setlocal
cd /d "%~dp0"
set QT_PLUGIN_PATH=
set QT_QPA_PLATFORM_PLUGIN_PATH=
start "" "%~dp0design_to_code.exe" %*
