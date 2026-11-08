$ErrorActionPreference = "Stop"
$SdeDir = Join-Path $env:TEMP "ssrjson-sde"
$Url = "https://github.com/Antares0982/ssrjson-nix-dev/releases/download/v0.0.0/sde-external-10.8.0-2026-03-15-win.tar.xz"
$Sha256 = "176f87c80eb42bb91b73e1428f4a0fd067df322f901f9b4359b20b86b92c2bae"
if (-not (Test-Path "$SdeDir\sde.exe")) {
    New-Item -ItemType Directory -Force $SdeDir | Out-Null
    Invoke-WebRequest -Uri $Url -OutFile "$SdeDir\sde.tar.xz"
    if ((Get-FileHash "$SdeDir\sde.tar.xz" -Algorithm SHA256).Hash -ne $Sha256) {
        throw "SDE checksum mismatch"
    }
    tar -xJf "$SdeDir\sde.tar.xz" -C $SdeDir
    if ($LASTEXITCODE -ne 0) { throw "SDE extraction failed" }
    $inner = Get-ChildItem $SdeDir -Directory | Where-Object { $_.Name -like "sde-external-*" } | Select-Object -First 1
    Get-ChildItem $inner.FullName | Move-Item -Destination $SdeDir -Force
    $inner.Delete()
    Remove-Item "$SdeDir\sde.tar.xz"
}
$env:SSRJSON_SDE = "$SdeDir\sde.exe"
if ($env:GITHUB_ENV) { "SSRJSON_SDE=$env:SSRJSON_SDE" >> $env:GITHUB_ENV }
