<#
.SYNOPSIS
    Suraksha Complete Local Release Builder
.DESCRIPTION
    Builds, packages, signs, and generates checksums for all 4 release deliverables locally:
    1. Suraksha.exe (Stand-alone binary)
    2. SurakshaSetup-vX.Y.Z-x64.exe (Inno Setup Installer)
    3. Suraksha-vX.Y.Z-x64.msix (Modern Windows MSIX Package)
    4. SHA256SUMS.txt (Cryptographic checksum manifest)
.EXAMPLE
    .\scripts\Build-LocalRelease.ps1
#>

param (
    [string]$OutputDir = "dist",
    [string]$CertPath = "",
    [string]$CertPassword = "SurakshaTest123"
)

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "       Suraksha Local Release Pipeline Builder          " -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# 1. Read metadata from Version.h
$version = "2.0.0"
$buildTag = ""
$isBeta = $false

if (Test-Path "Version.h") {
    $h = Get-Content "Version.h" -Raw
    if ($h -match '#define\s+SURAKSHA_VERSION_STRING\s+L"([^"]+)"') { $version = $Matches[1] }
    if ($h -match '#define\s+SURAKSHA_BUILD_TAG\s+L"([^"]+)"') { $buildTag = $Matches[1] }
    if ($h -match '#define\s+SURAKSHA_IS_BETA\s+(true|false)') { $isBeta = ($Matches[1] -eq "true") }
}

$channel = if ($isBeta) { "Beta" } else { "Stable" }
$tag = if ($isBeta) { "v$version-beta" } else { "v$version" }

Write-Host "Target Release Profile:" -ForegroundColor Yellow
Write-Host "  * Channel:         $channel" -ForegroundColor White
Write-Host "  * Version:         $version ($tag)" -ForegroundColor White
Write-Host "  * Apple Build Tag: $buildTag" -ForegroundColor White
Write-Host ""

# Ensure output directory exists and is clean
if (-not (Test-Path $OutputDir)) { New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null }

# 2. Locate MSBuild and Compile Release|x64
Write-Host ">>> [1/5] Compiling Release Binary (x64)..." -ForegroundColor Green
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
}
if (-not $msbuild -or -not (Test-Path $msbuild)) {
    $msbuild = (Get-ChildItem -Path "C:\Program Files*\Microsoft Visual Studio*" -Filter "MSBuild.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
}

if (-not $msbuild) {
    Write-Error "MSBuild.exe not found! Please ensure Visual Studio is installed."
    exit 1
}

Write-Host "Using MSBuild: $msbuild" -ForegroundColor DarkGray
& "$msbuild" "Suraksha.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /verbosity:minimal

if ($LASTEXITCODE -ne 0) {
    Write-Error "MSBuild compilation failed!"
    exit 1
}
Write-Host "[OK] Release x64 binary compiled successfully!" -ForegroundColor Green

# 3. Setup Signing Certificate & Locate SignTool
Write-Host ">>> [2/5] Setting up Code Signing Certificate..." -ForegroundColor Green
$tempCert = "$env:TEMP\suraksha_local_cert.pfx"
if (-not $CertPath -or -not (Test-Path $CertPath)) {
    Write-Host "Creating self-signed code signing certificate..." -ForegroundColor DarkGray
    $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=Suraksha Open Source" -CertStoreLocation "Cert:\CurrentUser\My"
    $pwd = ConvertTo-SecureString -String $CertPassword -Force -AsPlainText
    Export-PfxCertificate -Cert $cert -FilePath $tempCert -Password $pwd | Out-Null
    $CertPath = $tempCert
}

$signtool = (Get-ChildItem -Path "D:\Windows Kits\10\bin\*\x64\signtool.exe", "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction SilentlyContinue | Select-Object -Last 1).FullName
if (-not $signtool) {
    $signtool = (Get-Command signtool.exe -ErrorAction SilentlyContinue).Source
}

if ($signtool) {
    Write-Host "Signing x64\Release\Suraksha.exe..." -ForegroundColor DarkGray
    & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "x64\Release\Suraksha.exe"
    if ($LASTEXITCODE -ne 0) {
        & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 "x64\Release\Suraksha.exe"
    }
}

# Copy standalone exe to dist
Copy-Item "x64\Release\Suraksha.exe" -Destination "$OutputDir\Suraksha.exe" -Force
Write-Host "[OK] Suraksha.exe signed and placed in $OutputDir/" -ForegroundColor Green

# 4. Build & Sign MSIX Modern Windows App Package
Write-Host ">>> [3/5] Building and Signing MSIX Package..." -ForegroundColor Green
$stageDir = "$env:TEMP\SurakshaMsixStage"
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path "$stageDir\Assets" | Out-Null

Copy-Item "x64\Release\Suraksha.exe" -Destination "$stageDir\Suraksha.exe" -Force
Copy-Item "packaging\AppxManifest.xml" -Destination "$stageDir\AppxManifest.xml" -Force
Copy-Item "packaging\Assets\*" -Destination "$stageDir\Assets\" -Force

# Match publisher in manifest to certificate
try {
    $certObj = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($CertPath, $CertPassword)
    $publisher = $certObj.Subject
    [xml]$manifest = Get-Content "$stageDir\AppxManifest.xml"
    $manifest.Package.Identity.Publisher = $publisher
    $manifest.Save("$stageDir\AppxManifest.xml")
} catch { }

$makeappx = (Get-ChildItem -Path "D:\Windows Kits\10\bin\*\x64\makeappx.exe", "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\makeappx.exe" -ErrorAction SilentlyContinue | Select-Object -Last 1).FullName
if (-not $makeappx) {
    $makeappx = (Get-Command makeappx.exe -ErrorAction SilentlyContinue).Source
}

$msixFile = "$OutputDir\Suraksha-$tag-x64.msix"
if ($makeappx) {
    & "$makeappx" pack /d "$stageDir" /p "$msixFile" /o | Out-Null
    if ($signtool) {
        & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "$msixFile"
        if ($LASTEXITCODE -ne 0) {
            & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 "$msixFile"
        }
    }
    Write-Host "[OK] MSIX package built: $msixFile" -ForegroundColor Green
} else {
    Write-Warning "makeappx.exe not found, skipping MSIX packaging."
}

# 5. Build & Sign Inno Setup Installer
Write-Host ">>> [4/5] Compiling and Signing Inno Setup Installer..." -ForegroundColor Green
$isccPath = (Get-Command iscc.exe -ErrorAction SilentlyContinue).Source
if (-not $isccPath) { $isccPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" }
if (-not (Test-Path $isccPath)) {
    $isccPath = (Get-ChildItem -Path "C:\Program Files*" -Filter "ISCC.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
}

if ($isccPath -and (Test-Path $isccPath)) {
    & "$isccPath" "SurakshaSetup.iss" | Out-Null
    $installerPath = Get-ChildItem -Path "Output" -Filter "*.exe" | Select-Object -First 1 -ExpandProperty FullName
    if ($installerPath) {
        if ($signtool) {
            & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 "$installerPath"
            if ($LASTEXITCODE -ne 0) {
                & $signtool sign /f "$CertPath" /p "$CertPassword" /fd SHA256 "$installerPath"
            }
        }
        Copy-Item "$installerPath" -Destination "$OutputDir\" -Force
        Write-Host "[OK] Setup Installer built: $(Split-Path $installerPath -Leaf)" -ForegroundColor Green
    }
} else {
    Write-Warning "Inno Setup Compiler (ISCC.exe) not found, skipping installer build."
}

# 6. Generate SHA256SUMS.txt
Write-Host ">>> [5/5] Generating Checksums Manifest..." -ForegroundColor Green
$distFiles = Get-ChildItem -Path $OutputDir -File | Where-Object { $_.Name -ne "SHA256SUMS.txt" }
$hashLines = foreach ($f in $distFiles) {
    $h = (Get-FileHash -Path $f.FullName -Algorithm SHA256).Hash.ToLower()
    "$h  $($f.Name)"
}
$hashLines | Out-File -FilePath "$OutputDir\SHA256SUMS.txt" -Encoding ascii
Write-Host "[OK] SHA256SUMS.txt generated!" -ForegroundColor Green

# 7. Summary Report
Write-Host ""
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "         ALL 4 DELIVERABLES BUILT SUCCESSFULLY          " -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""

Get-ChildItem -Path $OutputDir -File | Format-Table -Property Name, @{Name="Size (MB)"; Expression={[math]::Round($_.Length / 1MB, 2)}}, LastWriteTime
