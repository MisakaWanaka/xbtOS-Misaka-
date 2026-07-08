param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Builder = Join-Path $Root "scripts\build_iso_zh.ps1"

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "      XBTOS-CL 中文安装/打包程序" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "1. 编译内核并生成可引导 ISO"
Write-Host "2. 仅整理 ISO 目录并跳过编译"
Write-Host "3. 退出"
Write-Host ""

if ($SkipBuild) {
    & $Builder -SkipBuild
    exit $LASTEXITCODE
}

$Choice = Read-Host "请选择"

if ($Choice -eq "1") {
    & $Builder
} elseif ($Choice -eq "2") {
    & $Builder -SkipBuild
} else {
    Write-Host "已退出。"
}
