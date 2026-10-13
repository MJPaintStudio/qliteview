param(
    [string]$EngineDirectory = '',
    [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
. "$PSScriptRoot/Enter-BuildEnvironment.ps1"
if (!$EngineDirectory) { $EngineDirectory = "$project/build/windows-gl/bin" }
if (!$OutputDirectory) { $OutputDirectory = "$project/build/runtime" }
$engine = [IO.Path]::GetFullPath($EngineDirectory)
$runtime = [IO.Path]::GetFullPath($OutputDirectory)
$source = Join-Path $engine 'livicore.dll'
if (!(Test-Path -LiteralPath $source)) { throw "Missing native engine: $source" }
New-Item -ItemType Directory -Force -Path $runtime | Out-Null
$queue = [Collections.Generic.Queue[string]]::new()
$queue.Enqueue($source)
$copied = @{}
while ($queue.Count) {
    $source = $queue.Dequeue()
    $name = [IO.Path]::GetFileName($source)
    if ($copied.ContainsKey($name)) { continue }
    $copied[$name] = $true
    Copy-Item -LiteralPath $source -Destination (Join-Path $runtime $name) -Force
    $dependencies = & dumpbin.exe /nologo /dependents $source
    if ($LASTEXITCODE) { throw "Cannot inspect PE dependencies: $source" }
    foreach ($line in $dependencies) {
        if ($line -notmatch '^\s+([\w.+-]+\.dll)\s*$') { continue }
        $dependency = $Matches[1]
        $candidate = Join-Path $engine $dependency
        if (Test-Path -LiteralPath $candidate) { $queue.Enqueue($candidate); continue }
        if ($dependency -match '^(api-ms-|ext-ms-)' -or (Test-Path -LiteralPath (Join-Path "$env:WINDIR/System32" $dependency))) { continue }
        throw "Unresolved runtime dependency $dependency (required by $name)"
    }
}
$files = @($copied.Keys | Sort-Object | ForEach-Object {
    [ordered]@{
        path = $_
        sha256 = (Get-FileHash -LiteralPath (Join-Path $runtime $_) -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})
$manifest = [ordered]@{ engine = 'LiteView Windows x64'; files = $files }
[IO.File]::WriteAllText((Join-Path $runtime 'manifest.json'), ($manifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
Write-Output "Prepared $($files.Count) runtime files in $runtime"
