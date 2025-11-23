@echo off
set build_type=%1

pushd %~dp0
cd ..

if exist .\ssrjson rd /s /q .\ssrjson

if not exist build (
    mkdir build
)

cd build

cmake -T ClangCL ..

cmake --build . --config %build_type%

copy /Y .\%build_type%\ssrjson.pyd .\ssrjson.pyd

popd
