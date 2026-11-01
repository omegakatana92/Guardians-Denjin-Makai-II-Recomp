param(
    [Parameter(Mandatory = $true)][string]$Rom,
    [string]$BuildDir = "build\cmake"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    python tools\verify_rom.py $Rom
    if ($LASTEXITCODE -ne 0) { throw "ROM verification failed" }

    cmake -S . -B $BuildDir -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_C_COMPILER=gcc `
        "-DGRDIANS_ROM=$Rom"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    cmake --build $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    $headless = Join-Path $BuildDir "bin\guardians_headless.exe"
    & $headless --rom $Rom --frames 600 --hashes build\boot.hashes
    if ($LASTEXITCODE -ne 0) { throw "Headless smoke test failed" }
    Write-Host "Build and headless smoke test passed."
}
finally {
    Pop-Location
}
