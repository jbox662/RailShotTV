# Stage a deploy folder for WiX / zip distribution.
param(
  [string]$Config = "RelWithDebInfo",
  [string]$BuildRoot = ""
)

$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
if (-not $BuildRoot) {
  foreach ($c in @(
      "$repo\native\build\vs2026\bin\$Config",
      "$repo\native\build\release\bin\$Config",
      "$repo\native\build\release\bin"
    )) {
    if (Test-Path (Join-Path $c "RailShotTV.exe")) { $BuildRoot = $c; break }
  }
}
if (-not $BuildRoot -or -not (Test-Path (Join-Path $BuildRoot "RailShotTV.exe"))) {
  throw "RailShotTV.exe not found. Build RelWithDebInfo first."
}

$ffRoot = $env:RAILSHOT_FFMPEG_ROOT
if (-not $ffRoot) {
  $ffDir = Get-ChildItem "$repo\native\third_party\ffmpeg" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($ffDir) { $ffRoot = $ffDir.FullName }
}

$deploy = Join-Path $repo "native\build\deploy"
if (Test-Path $deploy) { Remove-Item $deploy -Recurse -Force }
New-Item -ItemType Directory -Force -Path $deploy | Out-Null
Copy-Item "$BuildRoot\*" $deploy -Recurse -Force

$qt = $env:QT6_DIR
if (-not $qt) { $qt = "C:\Qt\6.8.3\msvc2022_64" }
$windeploy = Join-Path $qt "bin\windeployqt.exe"
if (Test-Path $windeploy) {
  & $windeploy --release --no-translations (Join-Path $deploy "RailShotTV.exe")
}

if ($ffRoot -and (Test-Path "$ffRoot\bin")) {
  Copy-Item "$ffRoot\bin\*.dll" $deploy -Force
}

Copy-Item "$repo\docs\licensing\THIRD_PARTY_NOTICES.md" $deploy -Force
Copy-Item "$repo\docs\licensing\FFMPEG_BUILD.md" $deploy -Force

Write-Host "Staged deploy folder: $deploy"
Write-Host "Set WiX -dDeployDir=`"$deploy`" -dRepoRoot=`"$repo`""
