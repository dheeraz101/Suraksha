<#
.SYNOPSIS
    Suraksha MSIX Package Builder Script
.DESCRIPTION
    Packages the Release binary into a modern Windows MSIX package using makeappx.exe.
#>

param (
    [string]$Version = "2.0.0.0",
    [string]$OutputDir = "dist",
    [string]$CertPath = "",
    [string]$CertPassword = ""
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Building Suraksha MSIX Package" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$stageDir = "$env:TEMP\SurakshaMsixStage"
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path "$stageDir\Assets" | Out-Null
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

# Copy binary & assets
Copy-Item "x64\Release\Suraksha.exe" -Destination "$stageDir\Suraksha.exe" -Force
Copy-Item "packaging\AppxManifest.xml" -Destination "$stageDir\AppxManifest.xml" -Force
Copy-Item "packaging\Assets\*" -Destination "$stageDir\Assets\" -Force

# If certificate is present, sync Publisher in manifest to match certificate Subject exactly (non-interactively)
if ($CertPath -and (Test-Path $CertPath)) {
    try {
        $certObj = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($CertPath, $CertPassword)
        $publisher = $certObj.Subject
        Write-Host "Syncing AppxManifest Publisher to match certificate: $publisher" -ForegroundColor Yellow
        [xml]$manifest = Get-Content "$stageDir\AppxManifest.xml"
        $manifest.Package.Identity.Publisher = $publisher
        $manifest.Save("$stageDir\AppxManifest.xml")
    } catch {
        Write-Warning "Could not extract certificate subject from $CertPath"
    }
}

# Locate makeappx.exe (explicitly x64)
$makeappx = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin\*\x64" -Filter "makeappx.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -Last 1).FullName
if (-not $makeappx) {
    $makeappx = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "makeappx.exe" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -like "*\x64\makeappx.exe" } | Select-Object -Last 1).FullName
}
if (-not $makeappx) {
    $makeappx = (Get-Command makeappx.exe -ErrorAction SilentlyContinue).Source
}

if (-not $makeappx) {
    Write-Error "makeappx.exe not found. Please ensure Windows 10/11 SDK is installed."
    exit 1
}

$outputMsix = "$OutputDir\Suraksha-v2.0.0-x64.msix"
Write-Host "Creating MSIX: $outputMsix using $makeappx" -ForegroundColor Green
& "$makeappx" pack /d "$stageDir" /p "$outputMsix" /o

if ($LASTEXITCODE -ne 0) {
    Write-Error "makeappx.exe failed to create MSIX package."
    exit 1
}

# Sign MSIX package if certificate provided
if ($CertPath -and (Test-Path $CertPath)) {
    $signtool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin\*\x64" -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -Last 1).FullName
    if (-not $signtool) {
        $signtool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin" -Filter "signtool.exe" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -like "*\x64\signtool.exe" } | Select-Object -Last 1).FullName
    }
    if (-not $signtool) {
        $signtool = (Get-Command signtool.exe -ErrorAction SilentlyContinue).Source
    }
    if ($signtool) {
        Write-Host "Signing MSIX package: $outputMsix" -ForegroundColor Yellow
        if ($CertPassword) {
            & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "$outputMsix"
        } else {
            & $signtool sign /f "$CertPath" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "$outputMsix"
        }
    }
}

Remove-Item $stageDir -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "MSIX Package built successfully: $outputMsix" -ForegroundColor Green
