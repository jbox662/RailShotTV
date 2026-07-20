# Registers railshot_vcam.dll for Win11 Frame Server virtual camera use.
# Prefer enabling Virtual Camera from the RailShotTV UI (auto-registers).
param(
    [string]$DllPath = ""
)

$ErrorActionPreference = "Stop"
if (-not $DllPath) {
    $candidates = @(
        (Join-Path $PSScriptRoot "..\native\build\vs2026\bin\RelWithDebInfo\railshot_vcam.dll"),
        (Join-Path $PSScriptRoot "..\native\build\vs2026\bin\Release\railshot_vcam.dll")
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $DllPath = (Resolve-Path $c).Path; break }
    }
}
if (-not $DllPath -or -not (Test-Path $DllPath)) {
    throw "railshot_vcam.dll not found. Build the native project first or pass -DllPath."
}

$stageDir = Join-Path $env:ProgramData "RailShotTV"
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
$staged = Join-Path $stageDir "railshot_vcam.dll"
Copy-Item -Force $DllPath $staged
Write-Host "Staged $staged"

$reg = Start-Process -FilePath "regsvr32.exe" -ArgumentList @("/s", $staged) -Wait -PassThru
if ($reg.ExitCode -ne 0) {
    Write-Host "HKLM regsvr32 failed (exit $($reg.ExitCode)); trying per-user via rundll32..."
    rundll32.exe "$staged",DllRegisterServer
}

Write-Host "Registered. Enable Virtual Camera in RailShotTV, then select 'RailShotTV Virtual Camera' in other apps (Windows 11)."
