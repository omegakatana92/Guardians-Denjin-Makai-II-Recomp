param(
    [string]$BuildDir = "build\cmake",
    [string]$OutDir = "dist"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "$BuildDir\bin\Guardians-Windows-x86_64.exe"
$dll = Join-Path $root "$BuildDir\bin\SDL2.dll"
if (-not (Test-Path $exe)) { throw "missing $exe" }
if (-not (Test-Path $dll)) { throw "missing $dll (copy SDL2.dll beside the exe first)" }

New-Item -ItemType Directory -Force (Join-Path $root $OutDir) | Out-Null
$stage = Join-Path $root "build\package-windows"
Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $stage | Out-Null

Copy-Item $exe $stage
Copy-Item $dll $stage
Copy-Item (Join-Path $root "README.md") $stage
Copy-Item (Join-Path $root "LICENSE") $stage
Copy-Item (Join-Path $root "CHANGELOG.md") $stage
Copy-Item (Join-Path $root "LICENSES") $stage -Recurse
Copy-Item (Join-Path $root "docs") $stage -Recurse

$zip = Join-Path $root "$OutDir\Guardians-Windows-x86_64.zip"
Remove-Item $zip -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Host "wrote $zip (no ROM included)"
