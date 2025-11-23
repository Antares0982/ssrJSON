@echo off
setlocal

pushd %~dp0
cd ..


set "UPGRADE_FLAG=false"
for %%a in (%*) do (
    if "%%a"=="--upgrade" set "UPGRADE_FLAG=true"
)

if "%UPGRADE_FLAG%"=="true" (
    set "NEED_INSTALL=true"
) else (
    if not exist ".\build-benchmark\Lib\site-packages\ssrjson_benchmark\" (
        set "NEED_INSTALL=true"
    ) else (
        set "NEED_INSTALL=false"
    )
)

if "%NEED_INSTALL%"=="true" (
    if not exist ".\build-benchmark\benchmark-src\.git" (
        echo Cloning ssrJSON-benchmark from GitHub...
        git clone --branch main https://github.com/Nambers/ssrJSON-benchmark.git .\build-benchmark\benchmark-src
        if errorlevel 1 (
            echo Failed to clone repository.
            popd
            exit /b 1
        )
    ) else (
        echo Updating existing repository...
        pushd .\build-benchmark\benchmark-src
        git pull origin main
        if errorlevel 1 (
            echo Failed to pull latest changes.
            popd
            popd
            exit /b 1
        )
        popd
    )

    python -m pip install --prefix build-benchmark .\build-benchmark\benchmark-src
    if errorlevel 1 (
        echo Failed to install ssrjson-benchmark from source.
        popd
        exit /b 1
    )
)

set "PYTHONPATH_OLD=%PYTHONPATH%"
set "PYTHONPATH=.\build;.\build-benchmark\Lib\site-packages;%PYTHONPATH_OLD%"

python -m ssrjson_benchmark --process-gigabytes 0.1

popd
endlocal
