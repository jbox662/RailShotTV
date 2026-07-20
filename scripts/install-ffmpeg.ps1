# Install LGPL shared FFmpeg (BtbN) for RailShotTV
$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
$destRoot = Join-Path $repo "native\third_party\ffmpeg"
New-Item -ItemType Directory -Force -Path $destRoot | Out-Null

$api = "https://api.github.com/repos/BtbN/FFmpeg-Builds/releases/latest"
$rel = Invoke-RestMethod -Uri $api -Headers @{ "User-Agent" = "RailShotTV" }
$asset = $rel.assets | Where-Object { $_.name -eq "ffmpeg-master-latest-win64-lgpl-shared.zip" } | Select-Object -First 1
if (-not $asset) { throw "Could not find win64-lgpl-shared asset" }

$zip = Join-Path $env:TEMP $asset.name
Write-Host "Downloading $($asset.name)..."
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zip -UseBasicParsing

Write-Host "Extracting to $destRoot"
Get-ChildItem $destRoot -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force
Expand-Archive -Path $zip -DestinationPath $destRoot -Force

$inner = Get-ChildItem $destRoot -Directory | Select-Object -First 1
Write-Host "Installed LGPL FFmpeg at:"
Write-Host "  $($inner.FullName)"
Write-Host "Reconfigure with:"
Write-Host "  `$env:RAILSHOT_FFMPEG_ROOT = '$($inner.FullName)'"
Write-Host "  cmake -B native/build/vs2026 -DRAILSHOT_FFMPEG_ROOT=`"$($inner.FullName)`" ..."
