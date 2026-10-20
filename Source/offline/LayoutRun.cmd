@echo off
setlocal
set "PYTHONDONTWRITEBYTECODE=1"
set "PYTHONNOUSERSITE=1"
if not exist "%~dp0..\..\runtime\python\python.exe" (
  echo Missing controlled Python runtime. Ask the publisher; do not install dependencies.
  exit /b 2
)
"%~dp0..\..\runtime\python\python.exe" -B "%~dp0scripts\RuntimeRun.py" %*
exit /b %errorlevel%
