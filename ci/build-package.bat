@echo off
cd /d "%~dp0.."
copy /y licenses\* .
rmdir /s /q licenses
python -m pip install build
python -m build
if %errorlevel% neq 0 exit /b %errorlevel%
