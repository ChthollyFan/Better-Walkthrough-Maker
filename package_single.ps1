# @file package_single.ps1
# @brief 用 Enigma Virtual Box 把 release 目录打包成单个可执行文件。
#
# 用法（在项目根目录执行）：
#   pwsh -File package_single.ps1
#
# 前置条件：
#   1. 已执行 cmake --build build
#   2. 已执行 pwsh -File package.ps1（生成 release 目录）
#   3. 已安装 Enigma Virtual Box（默认路径 D:/software/Enigma Virtual Box）
#
# 产物：BWM.exe（单文件，双击即可运行，无需 Qt DLL）

param(
    # Enigma 打包工具目录：留空则优先用仓库内 tools/enigmavbconsole.exe，
    # 回退到本机安装目录（便于本地开发与 CI 保持一致）
    [string]$EnigmaDir = "",
    # 输出文件名（发布产物固定为 BWM.exe）
    [string]$OutputName = "BWM.exe"
)

$ErrorActionPreference = "Stop"

$root    = (Get-Location).Path
$release = Join-Path $root "release"
$mainExe = Join-Path $release "bwm.exe"
$output  = Join-Path $root $OutputName
$evbPath = Join-Path $root "build/bwm.evb"

# 定位 enigmavbconsole.exe：优先仓库内 tools/（CI 与本地一致），回退本机安装
$console = Join-Path $root "tools/enigmavbconsole.exe"
if (-not (Test-Path $console)) {
    if (-not $EnigmaDir) { $EnigmaDir = "D:/software/Enigma Virtual Box" }
    $console = Join-Path $EnigmaDir "enigmavbconsole.exe"
}

Write-Host "=== 1. 检查前置条件 ===" -ForegroundColor Cyan
if (-not (Test-Path $mainExe)) { throw "找不到 $mainExe -- 请先执行 pwsh -File package.ps1" }
if (-not (Test-Path $console)) { throw "找不到 enigmavbconsole.exe -- 请检查 tools/ 或 -EnigmaDir 参数" }
Write-Host "  release/bwm.exe OK"
Write-Host "  enigmavbconsole.exe OK ($console)"

# XML 特殊字符转义
function Esc([string]$s) {
    return $s.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;")
}

# 递归生成虚拟文件树（Type=2 文件，Type=3 目录）
function New-Tree([string]$dir, [int]$depth) {
    $pad = [string]::new([char]9, $depth)
    $sb = New-Object System.Text.StringBuilder
    foreach ($f in (Get-ChildItem -LiteralPath $dir -File | Sort-Object Name)) {
        if ($f.FullName -eq $mainExe) { continue }   # 主 exe 作为 InputFile，不放入虚拟文件
        [void]$sb.AppendLine("$pad<File><Type>2</Type><Name>$(Esc $f.Name)</Name><File>$(Esc $f.FullName)</File><ActiveX>false</ActiveX><ActiveXInstall>false</ActiveXInstall><Action>0</Action><OverwriteDateTime>false</OverwriteDateTime><OverwriteAttributes>false</OverwriteAttributes><PassCommandLine>false</PassCommandLine></File>")
    }
    foreach ($d in (Get-ChildItem -LiteralPath $dir -Directory | Sort-Object Name)) {
        [void]$sb.AppendLine("$pad<File><Type>3</Type><Name>$(Esc $d.Name)</Name><Action>0</Action><OverwriteDateTime>false</OverwriteDateTime><OverwriteAttributes>false</OverwriteAttributes><Files>")
        [void]$sb.Append((New-Tree $d.FullName ($depth + 1)))
        [void]$sb.AppendLine("$pad</Files></File>")
    }
    return $sb.ToString()
}

Write-Host "=== 2. 生成 .evb 项目文件 ===" -ForegroundColor Cyan
$body = New-Tree $release 3
$xml = @"
<?xml version="utf-16"?>
<>
	<InputFile>$(Esc $mainExe)</InputFile>
	<OutputFile>$(Esc $output)</OutputFile>
	<Files>
		<Enabled>true</Enabled>
		<Files>
			<File><Type>3</Type><Name>%DEFAULT FOLDER%</Name><Action>0</Action><OverwriteDateTime>false</OverwriteDateTime><OverwriteAttributes>false</OverwriteAttributes><Files>
$body			</Files></File>
		</Files>
	</Files>
	<Registries>
		<Enabled>false</Enabled>
		<Registries/>
	</Registries>
	<Packaging>
		<Enabled>false</Enabled>
	</Packaging>
	<Options>
		<ShareVirtualSystem>false</ShareVirtualSystem>
		<MapExecutableWithTemporaryFile>false</MapExecutableWithTemporaryFile>
		<AllowRunningOfVirtualExeFiles>false</AllowRunningOfVirtualExeFiles>
	</Options>
</>
"@
[System.IO.File]::WriteAllText($evbPath, $xml, [System.Text.Encoding]::Unicode)
Write-Host "  已生成 $evbPath"

Write-Host "=== 3. 打包（Enigma Virtual Box）===" -ForegroundColor Cyan
if (Test-Path $output) { Remove-Item $output -Force }
& $console $evbPath | Out-Null
if (-not (Test-Path $output)) { throw "打包失败：未生成 $output" }

$size = [math]::Round((Get-Item $output).Length / 1MB, 1)
Write-Host ""
Write-Host "=== 完成 ===" -ForegroundColor Green
Write-Host "  单文件 exe: $output ($size MB)"
Write-Host "  双击即可运行（无需 Qt DLL）"
Write-Host "  发布时将此文件上传到 GitHub Releases"
