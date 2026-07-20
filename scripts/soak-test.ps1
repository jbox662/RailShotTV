# Soak / reliability gate for RailShotTV
param(
  [int]$Hours = 1,
  [int]$SampleSeconds = 30,
  [string]$Exe = "",
  [double]$MaxWorkingSetGrowthMb = 512
)

$ErrorActionPreference = "Stop"
if (-not $Exe) {
  $candidates = @(
    "$PSScriptRoot\..\native\build\vs2026\bin\RelWithDebInfo\RailShotTV.exe",
    "$PSScriptRoot\..\native\build\release\bin\RailShotTV.exe",
    "$PSScriptRoot\..\native\build\release\RelWithDebInfo\RailShotTV.exe"
  )
  $Exe = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $Exe -or -not (Test-Path $Exe)) {
  Write-Error "Build RailShotTV first (Exe not found)"
  exit 1
}

$logDir = Join-Path $env:LOCALAPPDATA "RailShotTV\soak"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$csv = Join-Path $logDir ("soak-{0:yyyyMMdd-HHmmss}.csv" -f (Get-Date))
"timestamp,workingSetMb,privateMb,cpuSec" | Out-File $csv -Encoding utf8

$start = Get-Date
$end = $start.AddHours($Hours)
Write-Host "Soak start $start → end $end"
Write-Host "Exe: $Exe"
Write-Host "Metrics: $csv"

$proc = Start-Process -FilePath $Exe -PassThru
Start-Sleep -Seconds 3
$proc.Refresh()
$baselineMb = [math]::Round($proc.WorkingSet64 / 1MB, 1)
Write-Host "Baseline WorkingSet: ${baselineMb} MB"

while ((Get-Date) -lt $end) {
  if ($proc.HasExited) {
    Write-Error "RailShotTV exited early with code $($proc.ExitCode)"
    exit 2
  }
  $proc.Refresh()
  $ws = [math]::Round($proc.WorkingSet64 / 1MB, 1)
  $priv = [math]::Round($proc.PrivateMemorySize64 / 1MB, 1)
  $cpu = [math]::Round($proc.TotalProcessorTime.TotalSeconds, 1)
  "{0:o},{1},{2},{3}" -f (Get-Date), $ws, $priv, $cpu | Out-File $csv -Append -Encoding utf8

  $growth = $ws - $baselineMb
  if ($growth -gt $MaxWorkingSetGrowthMb) {
    Write-Error "Working set grew ${growth} MB (limit $MaxWorkingSetGrowthMb). Failing soak."
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    exit 3
  }
  Start-Sleep -Seconds $SampleSeconds
}

Write-Host "Soak completed — stopping process"
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Write-Host "OK — metrics at $csv"
