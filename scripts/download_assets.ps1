[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
$destDir = Join-Path $PSScriptRoot "..\assets\fonts"
if (!(Test-Path $destDir)) { New-Item -ItemType Directory -Force -Path $destDir | Out-Null }

$fonts = @(
    @{ Name = "Inter.ttf"; Url = "https://raw.githubusercontent.com/google/fonts/main/ofl/inter/Inter%5Bopsz%2Cwght%5D.ttf" },
    @{ Name = "remixicon.ttf"; Url = "https://cdn.jsdelivr.net/npm/remixicon@4.6.0/fonts/remixicon.ttf" },
    @{ Name = "lucide.ttf"; Url = "https://cdn.jsdelivr.net/npm/lucide-static@latest/font/lucide.ttf" }
)

foreach ($f in $fonts) {
    $outPath = Join-Path $destDir $f.Name
    Write-Host "Downloading $($f.Name)..."
    try {
        Invoke-WebRequest -Uri $f.Url -OutFile $outPath -UseBasicParsing -TimeoutSec 15
        $size = (Get-Item $outPath).Length
        Write-Host " [OK] $($f.Name) downloaded ($size bytes)"
    } catch {
        Write-Host " [FAIL] $($f.Name): $($_.Exception.Message)"
    }
}
