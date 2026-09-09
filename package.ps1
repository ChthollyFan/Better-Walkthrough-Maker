# @file package.ps1
# @brief 打包 release 目录 + 压缩 zip，一键完成。
#
# 用法（在项目根目录执行）：
#   pwsh -File package.ps1
#
# 默认 Qt 路径取环境变量 QT6_DIR；也可命令行传入：
#   pwsh -File package.ps1 -QtDir "C:/Users/ThinkPad/Qt/6.11.2/mingw_64"
#
# 产物：
#   release/                          部署目录（exe + Qt 运行时 + 插件）
#   Better-Walkthrough-Maker.zip      压缩分发包
#
# 前置条件：build/src/bwm.exe 已存在（请先 cmake --build build）

param(
    # Qt 路径：留空则取环境变量 QT6_DIR，再回退到本机 MinGW 版路径
    [string]$QtDir = "",
    # 设为 1 则保留 opengl32sw.dll（兼容性优先，体积回到约 60 MB）
    [string]$KeepSwrast = "0"
)

if (-not $QtDir) {
    # 优先本机 MinGW 版 Qt：环境变量 QT6_DIR 可能指向 MSVC 版（不匹配本工程的 MinGW 构建）
    $mingwQt = "C:/Users/ThinkPad/Qt/6.11.2/mingw_64"
    if (Test-Path (Join-Path $mingwQt "bin/windeployqt.exe")) {
        $QtDir = $mingwQt
    } elseif ($env:QT6_DIR) {
        $QtDir = $env:QT6_DIR
    }
}

$ErrorActionPreference = "Stop"

$root    = (Get-Location).Path
$build   = Join-Path $root "build/src/bwm.exe"
$release = Join-Path $root "release"
$zipPath = Join-Path $root "Better-Walkthrough-Maker.zip"
$winDeploy = Join-Path $QtDir "bin/windeployqt.exe"

Write-Host "=== 1. 检查编译产物 ===" -ForegroundColor Cyan
if (-not (Test-Path $build)) { throw "找不到 $build —— 请先执行 cmake --build build" }
Write-Host "  bwm.exe OK"

Write-Host "=== 2. 清理旧 release ===" -ForegroundColor Cyan
if (Test-Path $release) { Remove-Item $release -Recurse -Force }
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
New-Item -ItemType Directory -Path $release | Out-Null

Write-Host "=== 3. 复制 exe 并部署 Qt 运行时 ===" -ForegroundColor Cyan
Copy-Item $build $release
if (-not (Test-Path $winDeploy)) { throw "找不到 windeployqt.exe ($winDeploy) —— 请检查 -QtDir" }
& $winDeploy --release --no-translations "$release\bwm.exe" | Out-Null

Write-Host "=== 4. 精简（删除软件光栅器） ===" -ForegroundColor Cyan
$swrast = Join-Path $release "opengl32sw.dll"
if ((Test-Path $swrast) -and $KeepSwrast -ne "1") { Remove-Item $swrast; Write-Host "  已删除 opengl32sw.dll（需目标机器有显卡驱动）" }
elseif (Test-Path $swrast) { Write-Host "  保留 opengl32sw.dll（-KeepSwrast 1，兼容性优先）" }

Write-Host "=== 5. 压缩 zip ===" -ForegroundColor Cyan
Compress-Archive -Path $release -DestinationPath $zipPath -Force

$releaseSize = (Get-ChildItem $release -Recurse -File | Measure-Object -Property Length -Sum).Sum
$zipSize     = (Get-Item $zipPath).Length
$fileCount   = (Get-ChildItem $release -Recurse -File).Count

Write-Host "" -ForegroundColor Cyan
Write-Host "=== 完成 ===" -ForegroundColor Green
Write-Host "  release 目录: $(([math]::Round($releaseSize/1048576,1))) MB  ($fileCount 个文件)"
Write-Host "  zip 压缩包:   $(([math]::Round($zipSize/1048576,1))) MB  -> $zipPath"
Write-Host "  双击 release\bwm.exe 即可运行"
