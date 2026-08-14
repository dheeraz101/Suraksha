<#
.SYNOPSIS
    Suraksha Local Code Signing Helper Script
.DESCRIPTION
    Helper script to sign Suraksha.exe and SurakshaSetup installer using signtool.exe.
    Can use an existing .pfx file or generate a local self-signed certificate for testing.
#>

param (
    [string]$FilePath = "x64\Release\Suraksha.exe",
    [string]$PfxPath = "",
    [string]$Password = ""
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Suraksha Code Signing Utility" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

if (-not (Test-Path $FilePath)) {
    Write-Error "Target file '$FilePath' does not exist. Please build the project first."
    exit 1
}

# Locate signtool.exe
$signtool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1).FullName

if (-not $signtool) {
    Write-Error "signtool.exe not found. Please install the Windows 10/11 SDK."
    exit 1
}

Write-Host "Found signtool: $signtool" -ForegroundColor Green

if ($PfxPath -and (Test-Path $PfxPath)) {
    Write-Host "Signing '$FilePath' with certificate '$PfxPath'..." -ForegroundColor Yellow
    if ($Password) {
        & $signtool sign /f "$PfxPath" /p "$Password" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "$FilePath"
    } else {
        & $signtool sign /f "$PfxPath" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "$FilePath"
    }
} else {
    Write-Host "No PFX provided. Generating local self-signed certificate for development testing..." -ForegroundColor Yellow
    $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=Suraksha Development Certificate" -CertStoreLocation "Cert:\CurrentUser\My"
    $tempPfx = "$env:TEMP\suraksha_dev_cert.pfx"
    $pwd = ConvertTo-SecureString -String "DevPass123" -Force -AsPlainText
    Export-PfxCertificate -Cert $cert -FilePath $tempPfx -Password $pwd | Out-Null
    
    & $signtool sign /f "$tempPfx" /p "DevPass123" /fd SHA256 "$FilePath"
    Remove-Item $tempPfx -Force -ErrorAction SilentlyContinue
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "File successfully signed: $FilePath" -ForegroundColor Green
    & $signtool verify /pa "$FilePath"
} else {
    Write-Error "Signing failed with exit code $LASTEXITCODE"
}
