# Generate Strings\<lang>\Resources.resw for MSIX Start Menu / tile display names.
param(
  [Parameter(Mandatory = $true)][string]$OutDir,
  [Parameter(Mandatory = $false)][string]$NamesJson = ""
)

$ErrorActionPreference = "Stop"
if (-not $NamesJson) {
  $NamesJson = Join-Path $PSScriptRoot "localized_names.json"
}
if (-not (Test-Path -LiteralPath $NamesJson)) {
  throw "Missing names file: $NamesJson"
}

$map = Get-Content -LiteralPath $NamesJson -Raw -Encoding UTF8 | ConvertFrom-Json
if (-not $map.languages) {
  throw "localized_names.json missing 'languages'"
}

function Escape-Xml([string]$s) {
  if ($null -eq $s) { return "" }
  return ($s -replace "&", "&amp;" -replace "<", "&lt;" -replace ">", "&gt;" -replace '"', "&quot;")
}

function Write-Resw([string]$path, [string]$displayName, [string]$description) {
  $dn = Escape-Xml $displayName
  $ds = Escape-Xml $description
  $xml = @"
<?xml version="1.0" encoding="utf-8"?>
<root>
  <data name="AppDisplayName" xml:space="preserve">
    <value>$dn</value>
  </data>
  <data name="AppDescription" xml:space="preserve">
    <value>$ds</value>
  </data>
  <data name="TileShortName" xml:space="preserve">
    <value>$dn</value>
  </data>
</root>
"@
  $dir = Split-Path -Parent $path
  if (-not (Test-Path -LiteralPath $dir)) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
  }
  [IO.File]::WriteAllText($path, $xml, (New-Object System.Text.UTF8Encoding $false))
}

if (Test-Path -LiteralPath $OutDir) {
  Remove-Item -LiteralPath $OutDir -Recurse -Force
}
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

$count = 0
foreach ($prop in $map.languages.PSObject.Properties) {
  $lang = $prop.Name
  $entry = $prop.Value
  $out = Join-Path $OutDir (Join-Path $lang "Resources.resw")
  Write-Resw $out $entry.displayName $entry.description
  $count++
}

Write-Host "Generated $count language Resources.resw under $OutDir"
