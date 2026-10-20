@echo off
setlocal
if not exist "%~dp0python\python.exe" (
  echo PACKAGE_INCOMPLETE: private Python 3.9 missing. Obtain the complete offline package.
  exit /b 2
)
"%~dp0python\python.exe" -I -B -X utf8 "%~dp0scripts\offline.py" %*
exit /b %errorlevel%
