param(
    [string]$BuildDir = "build",
    [string]$IsoName = "XBTOS-CL.iso",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildPath = Join-Path $Root $BuildDir
$IsoRoot = Join-Path $BuildPath "iso-root"
$EfiBootDir = Join-Path $IsoRoot "EFI\BOOT"
$BootDir = Join-Path $IsoRoot "boot"
$IsoPath = Join-Path $BuildPath $IsoName

function Write-Step($Text) {
    Write-Host "[XBTOS 安装器] $Text" -ForegroundColor Cyan
}

function Find-Tool($Names) {
    foreach ($Name in $Names) {
        $Tool = Get-Command $Name -ErrorAction SilentlyContinue
        if ($Tool) { return $Tool.Source }
    }
    return $null
}

function Require-Tool($Names, $Hint) {
    $Tool = Find-Tool $Names
    if (-not $Tool) { throw "缺少工具：$($Names -join ', ')。$Hint" }
    return $Tool
}

Write-Step "准备目录"
New-Item -ItemType Directory -Force -Path @($BuildPath, $IsoRoot, $EfiBootDir, $BootDir) | Out-Null

if (-not $SkipBuild) {
    Write-Step "检查 CMake 与编译工具"
    $CMake = Require-Tool @("cmake.exe", "cmake") "请安装 CMake，并确保它在 PATH 中。"
    Write-Step "配置内核"
    & $CMake -S $Root -B $BuildPath
    Write-Step "编译内核"
    & $CMake --build $BuildPath --target XBTOS.elf
}

$Kernel = Join-Path $BuildPath "XBTOS.elf"
if (Test-Path $Kernel) {
    Copy-Item $Kernel (Join-Path $BootDir "XBTOS.elf") -Force
} else {
    Write-Warning "未找到 XBTOS.elf，ISO 仍会生成 EFI 目录，但 BIOS/GRUB 启动不可用。"
}

Write-Step "生成 UEFI 引导程序"
$Clang = Find-Tool @("clang.exe", "clang")
$LldLink = Find-Tool @("lld-link.exe", "lld-link")
$UefiSource = Join-Path $Root "boot\uefi\bootx64.c"
$UefiOutput = Join-Path $EfiBootDir "BOOTX64.EFI"

if ($Clang -and $LldLink) {
    & $Clang -target x86_64-pc-win32 -ffreestanding -fshort-wchar -mno-red-zone -nostdlib `
        -Wl,/subsystem:efi_application -Wl,/entry:efi_main `
        -fuse-ld=lld-link $UefiSource -o $UefiOutput
} else {
    Write-Warning "未找到 clang/lld-link，跳过 BOOTX64.EFI 编译。UEFI 启动文件需要安装 LLVM。"
}

Write-Step "写入 GRUB 配置"
$GrubDir = Join-Path $BootDir "grub"
New-Item -ItemType Directory -Force $GrubDir | Out-Null
@"
set timeout=3
set default=0

menuentry 'XBTOS-CL' {
    multiboot /boot/XBTOS.elf
    boot
}
"@ | Set-Content -Encoding ASCII (Join-Path $GrubDir "grub.cfg")

Write-Step "打包 ISO"
$GrubMkrescue = Find-Tool @("grub-mkrescue.exe", "grub-mkrescue")
$Xorriso = Find-Tool @("xorriso.exe", "xorriso")
$Oscdimg = Find-Tool @("oscdimg.exe", "oscdimg")

if ($GrubMkrescue) {
    & $GrubMkrescue -o $IsoPath $IsoRoot
} elseif ($Oscdimg) {
    & $Oscdimg -m -o -u2 $IsoRoot $IsoPath
    Write-Warning "oscdimg 只能做普通 ISO；若要 BIOS 可启动，请安装 grub-mkrescue/xorriso。"
} elseif ($Xorriso) {
    & $Xorriso -as mkisofs -R -J -o $IsoPath $IsoRoot
    Write-Warning "当前 xorriso 调用未注入 GRUB 启动扇区；若要 BIOS 可启动，请使用 grub-mkrescue。"
} else {
    throw "缺少 ISO 打包工具。请安装 grub-mkrescue 或 oscdimg。"
}

Write-Step "完成：$IsoPath"
