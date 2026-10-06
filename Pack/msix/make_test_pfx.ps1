param(
  [Parameter(Mandatory=$true)][string]$OutPfx,
  [Parameter(Mandatory=$true)][string]$Password
)
$secure = ConvertTo-SecureString $Password -AsPlainText -Force
$cert = New-SelfSignedCertificate -Type Custom `
  -Subject "CN=PinZhun-YouYouJieTu-Test" `
  -KeyUsage DigitalSignature `
  -FriendlyName "YouYouJieTu MSIX Test" `
  -CertStoreLocation "Cert:\CurrentUser\My" `
  -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
Export-PfxCertificate -Cert $cert -FilePath $OutPfx -Password $secure | Out-Null
Remove-Item -Path ("Cert:\CurrentUser\My\" + $cert.Thumbprint) -Force
Write-Host "Created $OutPfx"
