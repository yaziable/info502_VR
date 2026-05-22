$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$SourceDir = Join-Path $Root "scene_shadow_creatures3"
$BuildDir = Join-Path $SourceDir "build"
$Exe = Join-Path $BuildDir "ShadowCreatures\ShadowCreatures.exe"

cmake -S $SourceDir -B $BuildDir -G "MinGW Makefiles"
cmake --build $BuildDir

Write-Host ""
Write-Host "Built: $Exe"
Write-Host "Run with:"
Write-Host "  .\scene_shadow_creatures3\build\ShadowCreatures\ShadowCreatures.exe"
