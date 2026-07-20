# Harvest staged deploy folder into WiX RuntimeDlls fragment.
# Run after scripts/stage-deploy.ps1
param(
    [string]$DeployDir = "",
    [string]$OutWxs = ""
)

$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
if (-not $DeployDir) { $DeployDir = Join-Path $repo "native\build\deploy" }
if (-not (Test-Path (Join-Path $DeployDir "RailShotTV.exe"))) {
    throw "Deploy folder missing RailShotTV.exe. Run scripts/stage-deploy.ps1 first."
}
if (-not $OutWxs) { $OutWxs = Join-Path $repo "native\packaging\windows\RuntimeDlls.wxs" }

$heat = Get-Command heat.exe -ErrorAction SilentlyContinue
if (-not $heat) {
    $candidates = @(
        "${env:WIX}\bin\heat.exe",
        "C:\Program Files (x86)\WiX Toolset v3.14\bin\heat.exe",
        "C:\Program Files\WiX Toolset v5\bin\heat.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $heat = $c; break }
    }
}
if (-not $heat) {
    Write-Warning "heat.exe not found — writing a minimal RuntimeDlls.wxs with known companions."
    $helper = Join-Path $DeployDir "railshot_browser_helper.exe"
    $vcam = Join-Path $DeployDir "railshot_vcam.dll"
    $xml = @"
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Fragment>
    <ComponentGroup Id="RuntimeDlls" Directory="INSTALLFOLDER">
      <Component Id="BrowserHelper" Guid="*">
        <File Id="BrowserHelperExe" Source="$DeployDir\railshot_browser_helper.exe" KeyPath="yes" />
      </Component>
      <Component Id="VcamDll" Guid="*">
        <File Id="VcamDllFile" Source="$DeployDir\railshot_vcam.dll" KeyPath="yes" />
      </Component>
    </ComponentGroup>
  </Fragment>
</Wix>
"@
    Set-Content -Path $OutWxs -Value $xml -Encoding UTF8
    Write-Host "Wrote minimal $OutWxs (install WiX heat for full Qt/FFmpeg harvest)."
    return
}

& $heat dir $DeployDir -cg RuntimeDlls -gg -sfrag -srd -dr INSTALLFOLDER -out $OutWxs
Write-Host "Harvested $OutWxs from $DeployDir"
Write-Host "Link with RailShotTV.wxs using -dDeployDir and -dRepoRoot"
