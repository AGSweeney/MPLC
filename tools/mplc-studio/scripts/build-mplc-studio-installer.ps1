# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

# Build MPLC Studio and create a Windows installer with Inno Setup.

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Configuration = "Release",

    [string]$BuildDir = "build",

    [string]$QtDir = "C:\Qt\6.8.3\msvc2022_64",

    [string]$AppVersion = "0.1.0",

    [string]$IsccPath = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",

    [switch]$SkipStudioBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot
$buildPath = Join-Path $projectRoot $BuildDir
$appPath = Join-Path $buildPath "app"
$stagePath = Join-Path $projectRoot "dist\staging"
$distPath = Join-Path $projectRoot "dist"
$issPath = Join-Path $projectRoot "installer\MPLCStudio.iss"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Copy-IfExists {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )
    if (Test-Path $Source) {
        Copy-Item -Path $Source -Destination $Destination -Recurse -Force
    }
}

if (-not $SkipStudioBuild) {
    Write-Step "Building MPLC Studio ($Configuration)"
    & (Join-Path $scriptRoot "build-mplc-studio.ps1") -Configuration $Configuration -BuildDir $BuildDir -QtDir $QtDir
}

$exePath = Join-Path $appPath "MPLCStudio.exe"
if (-not (Test-Path $exePath)) {
    $exePath = Join-Path $appPath "Release\MPLCStudio.exe"
}
if (-not (Test-Path $exePath)) {
    throw "MPLCStudio.exe not found under $appPath. Build the studio first."
}

Write-Step "Staging installer payload"
if (Test-Path $stagePath) {
    Remove-Item $stagePath -Recurse -Force
}
New-Item -ItemType Directory -Path $stagePath | Out-Null

Copy-Item -Path $exePath -Destination $stagePath -Force
Copy-Item -Path (Join-Path $appPath "*.dll") -Destination $stagePath -Force

foreach ($subdir in @("generic", "iconengines", "imageformats", "networkinformation", "platforms", "styles", "tls")) {
    Copy-IfExists -Source (Join-Path $appPath $subdir) -Destination (Join-Path $stagePath $subdir)
}

$vcRedist = Join-Path $appPath "vc_redist.x64.exe"
if (Test-Path $vcRedist) {
    Copy-Item -Path $vcRedist -Destination $stagePath -Force
}

if (-not (Test-Path $IsccPath)) {
    $altIscc = Join-Path ${env:ProgramFiles} "Inno Setup 6\ISCC.exe"
    if (Test-Path $altIscc) {
        $IsccPath = $altIscc
    } else {
        throw "Inno Setup compiler not found. Install Inno Setup 6 or pass -IsccPath."
    }
}

Write-Step "Compiling installer (Inno Setup)"
New-Item -ItemType Directory -Path $distPath -Force | Out-Null

& $IsccPath `
    "/DMyAppVersion=$AppVersion" `
    "/DSourceDir=$stagePath" `
    $issPath

if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compile failed."
}

$installerName = "MPLCStudio-$AppVersion-Setup.exe"
$installerPath = Join-Path $distPath $installerName
if (-not (Test-Path $installerPath)) {
    throw "Expected installer was not produced: $installerPath"
}

Write-Host ""
Write-Host "Installer complete: $installerPath" -ForegroundColor Green
