param(
    [int]$Jobs = 4,
    [string]$QtDirectory = 'D:/Qt/6.11.1/msvc2022_64',
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $project
$port = "$PSScriptRoot/Build-WindowsPort.ps1"
& $port -WithV8 -BuildDirectory build/windows-port -Target livi_v8 -Jobs $Jobs
& $port -WithBase -BuildDirectory build/windows-base -Target livi_base -Jobs $Jobs
& $port -WithIcu -BuildDirectory build/windows-icu -Target livi_icu -Jobs $Jobs
& $port -WithGraphics -BuildDirectory build/windows-graphics -Target livi_skia,livi_freetype -Jobs $Jobs
& $port -WithUtil -WithSsl -BuildDirectory build/windows-util -Target livi_util,livi_ssl -Jobs $Jobs
& $port -WithMediaBase -BuildDirectory build/windows-media -Target livi_media_base -Jobs $Jobs
& $port -WithBlink -BuildDirectory build/b -Target livi_blink -Jobs $Jobs
& $port -WithBlink -BuildDirectory build/bg -Target livi_blink_generated -Jobs $Jobs
& $port -WithGl -WithEngine -BuildDirectory build/windows-gl -Target livi_angle,livi_gl,livi_uv,livicore -Jobs $Jobs
& "$PSScriptRoot/Prepare-Runtime.ps1"
. "$PSScriptRoot/Enter-BuildEnvironment.ps1"
$tests = if ($SkipTests) { 'OFF' } else { 'ON' }
$ErrorActionPreference = 'Continue'
& cmake -S $project -B build/release -G Ninja '-DCMAKE_BUILD_TYPE=Release' '-DCMAKE_CXX_COMPILER=cl' `
    "-DCMAKE_PREFIX_PATH=$QtDirectory" '-DQLITEVIEW_EMBED_RUNTIME=ON' "-DBUILD_TESTING=$tests"
if ($LASTEXITCODE) { throw 'Qt configuration failed.' }
& cmake --build build/release --parallel $Jobs
if ($LASTEXITCODE) { throw 'Qt build failed.' }
& python "$PSScriptRoot/check_qml_contract.py" build/release/src/QWebKit/meta_types/qt6qliteview_metatypes.json
if ($LASTEXITCODE) { throw 'QML compatibility check failed.' }
if (!$SkipTests) {
    $env:PATH = "$QtDirectory/bin;$env:PATH"
    $env:QWEBKIT_REQUIRE_EMBEDDED = '1'
    & ctest --test-dir build/release --output-on-failure
    if ($LASTEXITCODE) {
        Get-Content build/release/tests/smoke-result.txt
        throw 'Runtime validation failed.'
    }
}
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path "$project/dist" | Out-Null
Copy-Item -LiteralPath "$project/build/release/qml/QWebKit/qliteview.dll" -Destination "$project/dist/qliteview.dll" -Force
if (Test-Path -LiteralPath "$project/build/release/bin/qliteview_example.exe") {
    Copy-Item -LiteralPath "$project/build/release/bin/qliteview_example.exe" -Destination "$project/dist/qliteview_example.exe" -Force
    Copy-Item -LiteralPath "$PSScriptRoot/Run-Example.ps1" -Destination "$project/dist/Run-Example.ps1" -Force
}
Write-Output "Built $project/dist/qliteview.dll"
