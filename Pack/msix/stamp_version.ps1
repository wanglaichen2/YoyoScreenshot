param(
  [Parameter(Mandatory = $true)][string]$ManifestPath,
  [Parameter(Mandatory = $true)][string]$Version
)
if ($Version -notmatch '^\d+\.\d+\.\d+\.\d+$') {
  throw "Version must be like 1.0.4.0, got: $Version"
}
$c = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8
# Only stamp Identity Version=..., never MinVersion / MaxVersionTested
$pattern = '(<Identity\b[^>]*\bVersion=")(\d+\.\d+\.\d+\.\d+)(")'
if ($c -notmatch $pattern) {
  throw "Identity Version not found in $ManifestPath"
}
$c = [regex]::Replace($c, $pattern, ('${1}' + $Version + '${3}'), 1)
[IO.File]::WriteAllText($ManifestPath, $c, (New-Object System.Text.UTF8Encoding $false))
Write-Host "Stamped Identity Version=$Version"
