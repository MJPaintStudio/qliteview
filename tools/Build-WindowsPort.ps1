param(
    [int]$Jobs = 4,
    [string]$LlvmDirectory = '',
    [switch]$WithV8,
    [switch]$WithV8Smoke,
    [switch]$WithBase,
    [switch]$WithIcu,
    [switch]$WithGraphics,
    [switch]$WithBlink,
    [switch]$WithUtil,
    [switch]$WithSsl,
    [switch]$WithGl,
    [switch]$WithMediaBase,
    [switch]$WithEngine,
    [string]$BuildDirectory = '',
    [string[]]$Target = @('all'),
    [int]$KeepGoing = 20
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
. "$PSScriptRoot/Enter-BuildEnvironment.ps1" -LlvmDirectory $LlvmDirectory
$v8 = if ($WithV8 -or $WithV8Smoke) { 'ON' } else { 'OFF' }
$v8Smoke = if ($WithV8Smoke) { 'ON' } else { 'OFF' }
$base = if ($WithBase) { 'ON' } else { 'OFF' }
$icu = if ($WithIcu) { 'ON' } else { 'OFF' }
$graphics = if ($WithGraphics) { 'ON' } else { 'OFF' }
$blink = if ($WithBlink) { 'ON' } else { 'OFF' }
$util = if ($WithUtil) { 'ON' } else { 'OFF' }
$ssl = if ($WithSsl) { 'ON' } else { 'OFF' }
$gl = if ($WithGl) { 'ON' } else { 'OFF' }
$mediaBase = if ($WithMediaBase) { 'ON' } else { 'OFF' }
$engine = if ($WithEngine) { 'ON' } else { 'OFF' }
if (!$BuildDirectory) { $BuildDirectory = "$project/build/windows-port" }
# Windows PowerShell 5 treats redirected native stderr (including CMake
# warnings) as an ErrorRecord. Native exit codes are the authority here.
$ErrorActionPreference = 'Continue'
& cmake -S "$project/ports/windows" -B $BuildDirectory -G Ninja '-DCMAKE_BUILD_TYPE=Release' '-DCMAKE_C_COMPILER=clang-cl' '-DCMAKE_CXX_COMPILER=clang-cl' "-DLITEVIEW_BUILD_V8_PORT=$v8" "-DLITEVIEW_BUILD_V8_SMOKE=$v8Smoke" "-DLITEVIEW_BUILD_BASE_PORT=$base" "-DLITEVIEW_BUILD_ICU_PORT=$icu" "-DLITEVIEW_BUILD_GRAPHICS_PORT=$graphics" "-DLITEVIEW_BUILD_BLINK_PORT=$blink" "-DLITEVIEW_BUILD_UTIL_PORT=$util" "-DLITEVIEW_BUILD_SSL_PORT=$ssl" "-DLITEVIEW_BUILD_GL_PORT=$gl" "-DLITEVIEW_BUILD_MEDIA_BASE_PORT=$mediaBase" "-DLITEVIEW_LINK_ENGINE=$engine"
if ($LASTEXITCODE -ne 0) { $ErrorActionPreference = 'Stop'; throw 'Windows port configuration failed.' }
& cmake --build $BuildDirectory --target $Target --parallel $Jobs -- -k $KeepGoing
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw 'Windows port compilation failed; see compiler diagnostics.' }
