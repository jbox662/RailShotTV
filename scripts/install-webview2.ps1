# Downloads Microsoft.Web.WebView2 NuGet into native/third_party/webview2 for the browser helper.
param(
    [string]$Version = "1.0.2903.40",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $OutDir = Join-Path $root "native\third_party\webview2"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$zip = Join-Path $OutDir "webview2.zip"
$pkg = Join-Path $OutDir "pkg"
$uri = "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$Version"

Write-Host "Downloading WebView2 $Version..."
Invoke-WebRequest -Uri $uri -OutFile $zip -UseBasicParsing
if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
Expand-Archive -Path $zip -DestinationPath $pkg -Force

$include = Join-Path $pkg "build\native\include\WebView2.h"
$loader = Join-Path $pkg "build\native\x64\WebView2LoaderStatic.lib"
if (-not (Test-Path $include) -or -not (Test-Path $loader)) {
    throw "WebView2 package layout unexpected — missing headers or x64 loader."
}

Write-Host "WebView2 SDK ready at $pkg"
Write-Host "Set RAILSHOT_WEBVIEW2_ROOT=$pkg\build\native (CMake auto-detects this path)."
Write-Host "Runtime: install Evergreen WebView2 Runtime (usually already present with Edge)."
