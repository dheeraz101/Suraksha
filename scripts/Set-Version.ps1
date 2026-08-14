<#
.SYNOPSIS
    Suraksha Version and Codename Management Script
.DESCRIPTION
    Updates application versions and codenames across all project files:
    - Version.h (C++ header)
    - SurakshaSetup.iss (Inno Setup Installer)
    - packaging/AppxManifest.xml (MSIX Package Manifest)
    - README.md & CHANGELOG.md
.EXAMPLE
    .\scripts\Set-Version.ps1 -Type Release -Bump Minor -CodeName "Vajra"
.EXAMPLE
    .\scripts\Set-Version.ps1 -Type Beta -Bump Patch -CodeName "Kavach"
.EXAMPLE
    .\scripts\Set-Version.ps1 -Version "2.1.0" -Type Release -CodeName "Raksha"
#>

param (
    [ValidateSet("Release", "Beta")]
    [string]$Type = "Release",

    [string]$Version = "",

    [ValidateSet("Major", "Minor", "Patch", "None")]
    [string]$Bump = "None",

    [string]$CodeName = ""
)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Suraksha Version & Codename Manager" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Read current version from Version.h
$versionHPath = "Version.h"
if (-not (Test-Path $versionHPath)) {
    Write-Error "Version.h not found in root directory!"
    exit 1
}

$versionHContent = Get-Content $versionHPath -Raw

$major = 2
$minor = 0
$patch = 0
$currentCodeName = "Kavach"

if ($versionHContent -match '#define\s+SURAKSHA_VERSION_MAJOR\s+(\d+)') { $major = [int]$Matches[1] }
if ($versionHContent -match '#define\s+SURAKSHA_VERSION_MINOR\s+(\d+)') { $minor = [int]$Matches[1] }
if ($versionHContent -match '#define\s+SURAKSHA_VERSION_PATCH\s+(\d+)') { $patch = [int]$Matches[1] }
if ($versionHContent -match '#define\s+SURAKSHA_CODENAME\s+L"([^"]+)"') { $currentCodeName = $Matches[1] }

Write-Host "Current Version: $major.$minor.$patch ($currentCodeName)" -ForegroundColor DarkGray

# 2. Determine new version
if ($Version) {
    if ($Version -match '^(\d+)\.(\d+)\.(\d+)') {
        $newMajor = [int]$Matches[1]
        $newMinor = [int]$Matches[2]
        $newPatch = [int]$Matches[3]
    } else {
        Write-Error "Invalid Version format '$Version'. Expected Semantic Version like '2.1.0'."
        exit 1
    }
} else {
    $newMajor = $major
    $newMinor = $minor
    $newPatch = $patch

    switch ($Bump) {
        "Major" { $newMajor++; $newMinor = 0; $newPatch = 0 }
        "Minor" { $newMinor++; $newPatch = 0 }
        "Patch" { $newPatch++ }
        "None"  { }
    }
}

$newCodeName = if ($CodeName) { $CodeName } else { $currentCodeName }
$isBeta = ($Type -eq "Beta")
$semVer = "$newMajor.$newMinor.$newPatch"
$displayVersion = if ($isBeta) { "$semVer-beta ($newCodeName)" } else { "$semVer ($newCodeName)" }
$rawDisplayVer = if ($isBeta) { "$semVer-beta" } else { "$semVer" }
$msixVersion = "$newMajor.$newMinor.$newPatch.0"

Write-Host ""
Write-Host "Applying New Version Profile:" -ForegroundColor Yellow
Write-Host "  * Type:            $Type" -ForegroundColor White
Write-Host "  * SemVer:          $semVer" -ForegroundColor White
Write-Host "  * Codename:        $newCodeName" -ForegroundColor White
Write-Host "  * Display Version: $displayVersion" -ForegroundColor White
Write-Host "  * MSIX Version:    $msixVersion" -ForegroundColor White
Write-Host ""

# 3. Update Version.h
$isBetaLiteral = if ($isBeta) { "true" } else { "false" }
$lines = @(
    "#pragma once",
    "",
    "// ═══════════════════════════════════════════════════════════════",
    "// Suraksha Version & Codename Definition",
    "// Automatically maintained by scripts/Set-Version.ps1",
    "// ═══════════════════════════════════════════════════════════════",
    "",
    "#define SURAKSHA_VERSION_MAJOR      $newMajor",
    "#define SURAKSHA_VERSION_MINOR      $newMinor",
    "#define SURAKSHA_VERSION_PATCH      $newPatch",
    "#define SURAKSHA_VERSION_BUILD      0",
    "",
    "#define SURAKSHA_VERSION_STRING     L`"$semVer`"",
    "#define SURAKSHA_CODENAME           L`"$newCodeName`"",
    "#define SURAKSHA_IS_BETA            $isBetaLiteral",
    "",
    "#if SURAKSHA_IS_BETA",
    "    #define SURAKSHA_DISPLAY_VERSION L`"$semVer-beta ($newCodeName)`"",
    "#else",
    "    #define SURAKSHA_DISPLAY_VERSION L`"$semVer ($newCodeName)`"",
    "#endif"
)
$lines | Out-File -FilePath "Version.h" -Encoding utf8
Write-Host "[OK] Updated Version.h" -ForegroundColor Green

# 4. Update SurakshaSetup.iss
if (Test-Path "SurakshaSetup.iss") {
    $issContent = Get-Content "SurakshaSetup.iss" -Raw
    $issContent = $issContent -replace '#define MyAppVersion "[^"]+"', "#define MyAppVersion `"$rawDisplayVer`""
    $issContent | Out-File -FilePath "SurakshaSetup.iss" -Encoding utf8
    Write-Host "[OK] Updated SurakshaSetup.iss" -ForegroundColor Green
}

# 5. Update packaging/AppxManifest.xml
if (Test-Path "packaging\AppxManifest.xml") {
    [xml]$manifest = Get-Content "packaging\AppxManifest.xml"
    $manifest.Package.Identity.Version = $msixVersion
    $manifest.Save((Resolve-Path "packaging\AppxManifest.xml"))
    Write-Host "[OK] Updated packaging/AppxManifest.xml" -ForegroundColor Green
}

# 6. Update README.md
if (Test-Path "README.md") {
    $readme = Get-Content "README.md" -Raw
    $readme = [regex]::Replace($readme, '# Suraksha.*?\n', "# Suraksha - Privacy & Security (v$rawDisplayVer)`n")
    $readme | Out-File -FilePath "README.md" -Encoding utf8
    Write-Host "[OK] Updated README.md" -ForegroundColor Green
}

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Version successfully updated to $displayVersion!" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
