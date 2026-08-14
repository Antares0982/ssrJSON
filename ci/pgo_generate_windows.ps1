$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

$SdeUrl = "https://github.com/Antares0982/ssrjson-nix-dev/releases/download/v0.0.0/sde-external-10.8.0-2026-03-15-win.tar.xz"
$SdeDir = Join-Path $env:TEMP "ssrjson-sde"
$PgoData = "test_data\pgo"

# Download and extract SDE if needed
if (-not (Test-Path $SdeDir)) {
    Write-Host "Downloading Intel SDE..."
    New-Item -ItemType Directory -Force $SdeDir | Out-Null
    $tar = Join-Path $SdeDir "sde.tar.xz"
    Invoke-WebRequest -Uri $SdeUrl -OutFile $tar
    Write-Host "Extracting..."
    tar -xJf $tar -C $SdeDir
    if ($LASTEXITCODE -ne 0) { throw "Failed to extract SDE" }
    $inner = Get-ChildItem $SdeDir -Directory | Where-Object { $_.Name -like "sde-external-*" } | Select-Object -First 1
    Get-ChildItem $inner.FullName | Move-Item -Destination $SdeDir -Force
    $inner.Delete()
    Remove-Item $tar
}

$Sde = (Resolve-Path "$SdeDir\sde.exe").Path

# Smoke test: verify SDE works
Write-Host "SDE smoke test..."
Write-Host "  SDE path: $Sde"
Write-Host "  intel64 contents:"
Get-ChildItem "$SdeDir\intel64" | ForEach-Object { Write-Host "    $_" }
& $Sde --version 2>&1 | Write-Host
if ($LASTEXITCODE -ne 0) { throw "SDE smoke test failed (exit $LASTEXITCODE)" }
Write-Host "SDE OK."

# Detect free-threading Python and set up env vars (mirrors setup.py)
$cmakeFtArgs = @()
$pyFt = python -c "import sys; print('t' if hasattr(sys, '_is_gil_enabled') and not sys._is_gil_enabled() else '')"
if ($pyFt -eq "t") {
    Write-Host "Free-threading Python detected, setting SEARCH_PYTHON3_USE_ENV..."
    $env:Python3_INCLUDE_DIR = python -c "import sysconfig, os; print(os.path.dirname(sysconfig.get_config_h_filename()))"
    $env:Python3_LIBRARY = python -c "import sysconfig, os, sys; minor = sys.version_info[1]; t = 't' if (hasattr(sys, '_is_gil_enabled') and not sys._is_gil_enabled()) else ''; print(os.path.join(sysconfig.get_config_var('LIBDIR'), f'python3{minor}{t}.lib'))"
    $cmakeFtArgs = @("-DSEARCH_PYTHON3_USE_ENV=ON", "-DBUILD_FREE_THREADING=ON")
}

# Build instrumented binary
Write-Host "Building instrumented ssrjson..."
cmake . -B build-pgo-instr -T ClangCL `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_CTESTS=OFF `
    -DBUILD_SHIPPING_SIMD=ON `
    -DBUILD_PGO_GENERATE=ON `
    @cmakeFtArgs
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
cmake --build build-pgo-instr --config Release
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

# PGO training: 4 jobs (native + 3 SDE CPU levels) in parallel via ForEach -Parallel
Write-Host "Running PGO training (3 jobs)..."
Remove-Item -Recurse -Force $PgoData -ErrorAction SilentlyContinue

# VCRUNTIME140.dll picks its memmove implementation from a Windows feature-detection
# API that SDE does not intercept, so it uses AVX2 even under -ivb and trips chip check.
# CPUID emulation (what our own dispatch reads) is unaffected; ISA validation is done by
# the SDE test jobs, not here.
$NoChipCheck = @("-chip_check_disable", "1")

$trainArgs = @(
    @{ Name = "avx512"; Exe = $Sde;    Args = @("-clx") + $NoChipCheck + @("--", "python", "ci\pgo_train.py", "--build-dir", "build-pgo-instr", "--bench-dir", "bench", "--profile-dir", "$PgoData\avx512") },
    @{ Name = "avx2";   Exe = $Sde;    Args = @("-rpl") + $NoChipCheck + @("--", "python", "ci\pgo_train.py", "--build-dir", "build-pgo-instr", "--bench-dir", "bench", "--profile-dir", "$PgoData\avx2") },
    @{ Name = "sse42";  Exe = $Sde;    Args = @("-ivb") + $NoChipCheck + @("--", "python", "ci\pgo_train.py", "--build-dir", "build-pgo-instr", "--bench-dir", "bench", "--profile-dir", "$PgoData\sse42") }
)

foreach ($job in $trainArgs) {
    Write-Host "[$($job.Name)] Starting training..."
    New-Item -ItemType Directory -Force (Join-Path $PgoData $job.Name) | Out-Null
    $proc = Start-Process -FilePath $job.Exe -ArgumentList $job.Args -NoNewWindow -Wait -PassThru
    if ($proc.ExitCode -ne 0) {
        throw "Job $($job.Name) failed with exit code $($proc.ExitCode)"
    }
    Write-Host "[$($job.Name)] Done."
}

# Merge profiles
Write-Host "Merging profiles into merged.profdata..."
$profrawFiles = Get-ChildItem -Path $PgoData -Recurse -Filter "*.profraw" | ForEach-Object { $_.FullName }
if ($profrawFiles.Count -eq 0) { throw "No .profraw files found" }
llvm-profdata merge -o "$PgoData\merged.profdata" -sparse @profrawFiles
if ($LASTEXITCODE -ne 0) { throw "llvm-profdata merge failed" }

# Cleanup: keep only merged.profdata
Get-ChildItem -Path $PgoData -Exclude "merged.profdata" | Remove-Item -Recurse -Force

Write-Host "PGO profile ready: $PgoData\merged.profdata"
exit 0
