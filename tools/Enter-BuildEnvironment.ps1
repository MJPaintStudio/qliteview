param([string]$LlvmDirectory = '')
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ x64 tools were not found.' }
# Build.ps1 invokes this for each native library in the same process.
# Re-entering VsDevShell repeatedly expands PATH until cmd.exe rejects it.
if ($env:VSCMD_ARG_TGT_ARCH -notin @('x64', 'amd64') -or !(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    & (Join-Path $vs 'Common7/Tools/Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
}
if (!$LlvmDirectory) {
    $candidates = @("$PSScriptRoot/../../qwebkit/tools/llvm/LLVM/bin", 'C:/Program Files/LLVM/bin')
    $LlvmDirectory = $candidates | Where-Object { Test-Path (Join-Path $_ 'clang-cl.exe') } | Select-Object -First 1
}
if (!$LlvmDirectory -or !(Test-Path (Join-Path $LlvmDirectory 'clang-cl.exe'))) {
    throw 'Pass -LlvmDirectory pointing to the directory containing clang-cl.exe and lld-link.exe.'
}
$toolPaths = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$env:PATH = (("$LlvmDirectory;D:/Qt/Tools/Ninja;$env:PATH" -split ';') |
    Where-Object { $_ -and $toolPaths.Add($_) }) -join ';'
$env:VSLANG = '1033'
$env:PYTHONUTF8 = '1'
