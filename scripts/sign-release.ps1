# Sign release artifacts (Authenticode)
param(
  [Parameter(Mandatory = $true)][string]$Path,
  [string]$Thumbprint = $env:RAILSHOT_CERT_THUMBPRINT,
  [string]$TimestampUrl = "http://timestamp.digicert.com"
)

if (-not $Thumbprint) {
  Write-Error "Set RAILSHOT_CERT_THUMBPRINT or pass -Thumbprint"
  exit 1
}

$signtool = Get-ChildItem "C:\Program Files*\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction SilentlyContinue |
  Sort-Object FullName -Descending | Select-Object -First 1

if (-not $signtool) {
  Write-Error "signtool.exe not found. Install Windows SDK."
  exit 1
}

& $signtool.FullName sign /sha1 $Thumbprint /fd SHA256 /tr $TimestampUrl /td SHA256 $Path
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Signed $Path"
