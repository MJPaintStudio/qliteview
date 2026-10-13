param([string]$QtDirectory = 'D:/Qt/6.11.1/msvc2022_64', [switch]$Headless, [string]$Url)
$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'qliteview_example.exe'
if (!(Test-Path -LiteralPath $exe)) { $exe = Join-Path $PSScriptRoot '../dist/qliteview_example.exe' }
if (!(Test-Path -LiteralPath $exe)) { throw 'Run tools/Build.ps1 first.' }
$localQt = Test-Path -LiteralPath (Join-Path (Split-Path $exe -Parent) 'Qt6Core.dll')
if (!$localQt -and !(Test-Path -LiteralPath (Join-Path $QtDirectory 'bin/Qt6Core.dll'))) { throw 'Deploy Qt beside the example or set QtDirectory to the Qt 6.11.1 MSVC x64 kit.' }
$savedPath = $env:PATH
$savedPlatform = $env:QT_QPA_PLATFORM
$savedBackend = $env:QT_QUICK_BACKEND
try {
    if (!$localQt) { $env:PATH = (Join-Path $QtDirectory 'bin') + ';' + $env:PATH }
    $exampleArguments = @()
    if ($Url) { $exampleArguments += @('--url', $Url) }
    if ($Headless) {
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:QT_QUICK_BACKEND = 'software'
        $exampleArguments += '--smoke-test'
    }
    & $exe @exampleArguments | Out-Null
    if ($LASTEXITCODE) { throw "Example failed: $LASTEXITCODE" }
} finally {
    $env:PATH = $savedPath
    $env:QT_QPA_PLATFORM = $savedPlatform
    $env:QT_QUICK_BACKEND = $savedBackend
}
