# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

# Build MPLC Studio (Qt6 desktop IDE)

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Configuration = "Release",

    [string]$BuildDir = "build",

    [ValidateSet("Ninja", "VisualStudio")]
    [string]$Generator = "Ninja",

    [string]$QtDir = "C:\Qt\6.8.3\msvc2022_64",

    [switch]$SkipDeployQt
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot
$buildPath = Join-Path $projectRoot $BuildDir

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Import-CmdEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BatchPath,
        [string]$Arguments = ""
    )

    $tempFile = [System.IO.Path]::GetTempFileName()
    try {
        $wrappedCommand = "call `"$BatchPath`" $Arguments && set > `"$tempFile`""
        & cmd.exe /c $wrappedCommand | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to execute command: $BatchPath $Arguments"
        }

        Get-Content $tempFile | ForEach-Object {
            if ($_ -match "^(.*?)=(.*)$") {
                [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2])
            }
        }
    }
    finally {
        Remove-Item $tempFile -ErrorAction SilentlyContinue
    }
}

function Get-VsDevCmdPath {
    $vswhereExe = Join-Path "${env:ProgramFiles(x86)}" "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhereExe)) {
        throw "Could not find vswhere.exe. Install Visual Studio Build Tools."
    }

    $installPath = & $vswhereExe -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if (-not $installPath) {
        throw "No Visual Studio installation with MSBuild was found."
    }

    $devCmd = Join-Path $installPath "Common7\Tools\VsDevCmd.bat"
    if (-not (Test-Path $devCmd)) {
        throw "VsDevCmd.bat not found at expected path: $devCmd"
    }

    return $devCmd
}

Write-Step "Configuring MPLC Studio ($Configuration)"
Import-CmdEnvironment (Get-VsDevCmdPath) "-arch=amd64"

if (-not (Test-Path $QtDir)) {
    throw "Qt directory not found: $QtDir"
}

$cmakeArgs = @(
    "-S", $projectRoot,
    "-B", $buildPath,
    "-G", $Generator,
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DCMAKE_PREFIX_PATH=$QtDir"
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

Write-Step "Building MPLC Studio"
& cmake --build $buildPath --config $Configuration --target mplc_studio
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

$exePath = Join-Path $buildPath "app\MPLCStudio.exe"
if (-not (Test-Path $exePath)) {
    $exePath = Join-Path $buildPath "app\Release\MPLCStudio.exe"
}

if (-not $SkipDeployQt) {
    Write-Step "Deploying Qt runtime"
    $windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
    if (-not (Test-Path $windeployqt)) {
        throw "windeployqt not found: $windeployqt"
    }
    & $windeployqt --no-translations $exePath
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed." }
}

Write-Host ""
Write-Host "Build complete: $exePath" -ForegroundColor Green
