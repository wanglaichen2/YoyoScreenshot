param(
  [Parameter(Mandatory=$true)][string]$OutPfx,
  [Parameter(Mandatory=$true)][string]$Password
)
$ErrorActionPreference = "Stop"
# Subject 必须与 AppxManifest Identity.Publisher 一致
$secure = ConvertTo-SecureString -String $Password -AsPlainText -Force
$cert = New-SelfSignedCertificate -Type Custom `
  -Subject "CN=0220A900-026B-463F-AEFE-0F5505669E86" `
  -KeyUsage DigitalSignature `
  -FriendlyName "YouYouJieTu MSIX Store Identity" `
  -CertStoreLocation "Cert:\CurrentUser\My" `
  -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
try {
  Export-PfxCertificate -Cert $cert -FilePath $OutPfx -Password $secure | Out-Null
  Write-Host "Created $OutPfx"
} finally {
  Remove-Item -Path ("Cert:\CurrentUser\My\" + $cert.Thumbprint) -Force -ErrorAction SilentlyContinue
}
