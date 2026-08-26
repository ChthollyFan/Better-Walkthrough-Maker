# 本地打包脚本：windeployqt 部署运行时 → 组装发布目录 → 打 zip 绿色版
# 用法：
#   powershell -ExecutionPolicy Bypass -File .\deploy.ps1            # 版本号取最近 git tag，缺省 0.1.0
#   powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -Version 0.2.0
# 输出：dist\BetterWalkthroughMaker-<版本>-win64.zip
param(
    [string]$Version = ""
)
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildSrc = Join-Path $root "build\src"
$exe = Join-Path $buildSrc "bwm.exe"

if (-not (Test-Path $exe)) {
    Write-Error "未找到 $exe，请先执行 cmake --build build"
    exit 1
}

if (-not $Version) {
    $Version = git -C $root describe --tags --abbrev=0 2>$null
    if (-not $Version) { $Version = "0.1.0" }
}
Write-Host "打包版本：$Version"

# 1. 部署 Qt 运行时 DLL 到 exe 目录（幂等，重复执行安全）
$qtBin = "C:\Users\ThinkPad\Qt\6.11.2\mingw_64\bin"
$windeployqt = Join-Path $qtBin "windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    Write-Error "未找到 $windeployqt，请检查 Qt 安装路径"
    exit 1
}
Write-Host "运行 windeployqt 部署运行时…"
& $windeployqt --release --compiler-runtime $exe | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error "windeployqt 失败"; exit 1 }

# 2. 组装发布目录（exe + DLL + 插件 + 文档）
$distRoot = Join-Path $root "dist"
$pkgDir = Join-Path $distRoot "BetterWalkthroughMaker-$Version"
if (Test-Path $pkgDir) { Remove-Item -Recurse -Force $pkgDir }
New-Item -ItemType Directory -Force -Path $pkgDir | Out-Null

Write-Host "复制构建产物…"
Copy-Item -Path (Join-Path $buildSrc "*") -Destination $pkgDir -Recurse -Force
Copy-Item (Join-Path $root "README.md") $pkgDir -Force
if (Test-Path (Join-Path $root "LICENSE")) {
    Copy-Item (Join-Path $root "LICENSE") $pkgDir -Force
}

# 3. 打 zip
$zipPath = Join-Path $distRoot "BetterWalkthroughMaker-$Version-win64.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Write-Host "压缩发布包…"
Compress-Archive -Path $pkgDir -DestinationPath $zipPath -CompressionLevel Optimal -Force

Write-Host ""
Write-Host "✅ 发布包已生成：$zipPath"
Write-Host "（绿色版：用户解压后直接运行 BetterWalkthroughMaker.exe，无需安装 Qt）"
