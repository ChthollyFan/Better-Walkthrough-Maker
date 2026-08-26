# Local packaging script: deploy Qt runtime via windeployqt, assemble dist dir, create zip.
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\deploy.ps1            # version from latest git tag, fallback 0.1.0
#   powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -Version 0.2.0
# Output: dist\BetterWalkthroughMaker-<version>-win64.zip
param(
    [string]$Version = ""
)
$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$buildSrc = Join-Path $root "build\src"
$exe = Join-Path $buildSrc "bwm.exe"

if (-not (Test-Path $exe)) {
    Write-Error "bwm.exe not found at $exe. Run 'cmake --build build' first."
    exit 1
}

if (-not $Version) {
    try {
        $Version = & git -C $root describe --tags --abbrev=0 2>$null
    } catch {
        $Version = ""
    }
    if (-not $Version) { $Version = "0.1.0" }
}
Write-Host "Packaging version: $Version"

# 1. Deploy Qt runtime DLLs next to the exe (idempotent)
$qtBin = "C:\Users\ThinkPad\Qt\6.11.2\mingw_64\bin"
$windeployqt = Join-Path $qtBin "windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    Write-Error "windeployqt not found at $windeployqt. Check Qt install path."
    exit 1
}
Write-Host "Running windeployqt to deploy runtime..."
& $windeployqt --release --compiler-runtime $exe | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error "windeployqt failed"; exit 1 }

# 2. Assemble package directory (exe + DLLs + plugins + docs)
$distRoot = Join-Path $root "dist"
$pkgDir = Join-Path $distRoot "BetterWalkthroughMaker-$Version"
if (Test-Path $pkgDir) { Remove-Item -Recurse -Force $pkgDir }
New-Item -ItemType Directory -Force -Path $pkgDir | Out-Null

Write-Host "Copying build output..."
Copy-Item -Path (Join-Path $buildSrc "*") -Destination $pkgDir -Recurse -Force
Copy-Item (Join-Path $root "README.md") $pkgDir -Force
if (Test-Path (Join-Path $root "LICENSE")) {
    Copy-Item (Join-Path $root "LICENSE") $pkgDir -Force
}

# 3. Create zip
$zipPath = Join-Path $distRoot "BetterWalkthroughMaker-$Version-win64.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Write-Host "Creating zip..."
Compress-Archive -Path $pkgDir -DestinationPath $zipPath -CompressionLevel Optimal -Force

Write-Host ""
Write-Host "DONE: $zipPath"
Write-Host "(Portable build: extract and run BetterWalkthroughMaker.exe, no Qt install needed)"
