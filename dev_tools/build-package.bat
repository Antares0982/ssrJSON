@echo off
cd /d "%~dp0.."
xcopy /e /i /h /y pysrc ssrjson
copy /y licenses\* .
rmdir /s /q licenses
python -m pip install build
python -m build
rmdir /s /q ssrjson
