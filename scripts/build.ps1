#Requires -Version 5.1
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [ValidateSet("x64")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$DistDir = Join-Path $Root "dist"
$PackDir = Join-Path $Root "pack"
$ScriptsDir = Join-Path $Root "scripts"
$Solution = Join-Path $Root "MecchaCheatV.slnx"
$DllProject = Join-Path $Root "client\MecchaCheatV.vcxproj"
$InjectorProject = Join-Path $Root "injector\MecchaCheatV Injector.vcxproj"
$DllPath = Join-Path $BuildDir "xv_meca.dll"
$EmbeddedCpp = Join-Path $Root "injector\embedded_payload.cpp"
$EmbeddedHpp = Join-Path $Root "injector\embedded_payload.hpp"
$FinalExe = Join-Path $DistDir "xv_meca.exe"

function Find-MSBuild {
    $Candidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            return $Candidate
        }
    }

    $Cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($Cmd) {
        return $Cmd.Source
    }

    throw "MSBuild not found. Install Visual Studio 2022 with C++ workload."
}

function Find-Python {
    $Cmd = Get-Command python -ErrorAction SilentlyContinue
    if ($Cmd) {
        return $Cmd.Source
    }

    $Cmd = Get-Command py -ErrorAction SilentlyContinue
    if ($Cmd) {
        return "py -3"
    }

    throw "Python not found."
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

$MsBuild = Find-MSBuild
$Python = Find-Python

Write-Host "==> Building xv_meca.dll ($Configuration|$Platform)" -ForegroundColor Cyan
& $MsBuild $DllProject /m /p:Configuration=$Configuration /p:Platform=$Platform /t:Rebuild
if ($LASTEXITCODE -ne 0) { throw "DLL build failed" }

if (-not (Test-Path $DllPath)) {
    throw "Expected DLL at $DllPath"
}

Write-Host "==> Embedding payload into injector" -ForegroundColor Cyan
$EmbedArgs = @(
    (Join-Path $ScriptsDir "embed_payload.py"),
    "--dll", $DllPath,
    "--out-cpp", $EmbeddedCpp,
    "--out-hpp", $EmbeddedHpp
)

if ($Python -eq "py -3") {
    & py -3 @EmbedArgs
} else {
    & $Python @EmbedArgs
}
if ($LASTEXITCODE -ne 0) { throw "Embedding failed" }

Write-Host "==> Building single EXE launcher" -ForegroundColor Cyan
& $MsBuild $InjectorProject /m /p:Configuration=$Configuration /p:Platform=$Platform /t:Rebuild
if ($LASTEXITCODE -ne 0) { throw "Injector build failed" }

if (-not (Test-Path $FinalExe)) {
    throw "Expected EXE at $FinalExe"
}

$Info = Get-Item $FinalExe
Write-Host ""
Write-Host "Done." -ForegroundColor Green
Write-Host "Output: $($Info.FullName)" -ForegroundColor Green
Write-Host "Size:   $([math]::Round($Info.Length / 1MB, 2)) MB" -ForegroundColor Green
Write-Host ""
Write-Host "Usage:" -ForegroundColor Yellow
Write-Host "  1. Start Meccha Chameleon"
Write-Host "  2. Run dist\xv_meca.exe"
Write-Host "  3. Click LAUNCH + INJECT"
Write-Host "  4. Open http://127.0.0.1:17777 in browser"
