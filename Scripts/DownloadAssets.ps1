<#
.SYNOPSIS
    Downloads sample assets required by InnocenceEngine test scenes.

.DESCRIPTION
    Downloads models from Stanford 3D Scanning Repository and Intel Graphics
    Research Samples into OriginalAssets/Models/. Assets that already exist
    on disk are skipped.

    Intel Sponza assets require manual download (browser sign-in wall).
    The script prints instructions for those.

.PARAMETER Force
    Re-download assets even if the target directory already exists.
#>
param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$modelsDir = Join-Path (Join-Path $repoRoot "OriginalAssets") "Models"
$tempDir = Join-Path (Join-Path $repoRoot "Build") "asset_download_tmp"

if (-not (Test-Path $modelsDir)) {
    New-Item -ItemType Directory -Path $modelsDir -Force | Out-Null
}
if (-not (Test-Path $tempDir)) {
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
}

function Download-Asset {
    param(
        [string]$Name,
        [string]$Url,
        [string]$TargetDir,
        [scriptblock]$PostProcess
    )

    if ((Test-Path $TargetDir) -and -not $Force) {
        Write-Host "[SKIP] $Name already exists at $TargetDir" -ForegroundColor Yellow
        return
    }

    $fileName = Split-Path -Leaf $Url
    $downloadPath = Join-Path $tempDir $fileName

    Write-Host "[DOWNLOAD] $Name from $Url ..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $Url -OutFile $downloadPath -UseBasicParsing

    if (-not (Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
    }

    & $PostProcess $downloadPath $TargetDir
    Write-Host "[OK] $Name -> $TargetDir" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# GNU FreeFont (engine UI font)
# ---------------------------------------------------------------------------
$fontsDir = Join-Path (Join-Path (Join-Path $repoRoot "Data") "Generated") "Fonts"
if ((Test-Path (Join-Path $fontsDir "FreeSans.otf")) -and -not $Force) {
    Write-Host "[SKIP] FreeSans.otf already exists" -ForegroundColor Yellow
} else {
    Write-Host "[DOWNLOAD] GNU FreeFont ..." -ForegroundColor Cyan
    $fontArchive = Join-Path $tempDir "freefont-otf.tar.gz"
    Invoke-WebRequest -Uri "https://ftp.gnu.org/gnu/freefont/freefont-otf-20120503.tar.gz" -OutFile $fontArchive -UseBasicParsing
    tar -xzf $fontArchive -C $tempDir 2>$null
    if (-not (Test-Path $fontsDir)) {
        New-Item -ItemType Directory -Path $fontsDir -Force | Out-Null
    }
    Copy-Item (Join-Path (Join-Path $tempDir "freefont-20120503") "FreeSans.otf") (Join-Path $fontsDir "FreeSans.otf") -Force
    Write-Host "[OK] FreeSans.otf -> $fontsDir" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Stanford Bunny (PLY)
# ---------------------------------------------------------------------------
Download-Asset -Name "Stanford Bunny" `
    -Url "http://graphics.stanford.edu/pub/3Dscanrep/bunny.tar.gz" `
    -TargetDir (Join-Path $modelsDir "bunny") `
    -PostProcess {
        param($archive, $dest)
        tar -xzf $archive -C $dest --strip-components=1 2>$null
        # The archive contains bunny/reconstruction/bun_zipper.ply
        $ply = Get-ChildItem -Path $dest -Recurse -Filter "bun_zipper.ply" | Select-Object -First 1
        if ($ply) {
            Copy-Item $ply.FullName (Join-Path $dest "bunny.ply") -Force
            Write-Host "  Extracted bunny.ply (convert to OBJ with a mesh tool if needed)"
        }
    }

# ---------------------------------------------------------------------------
# Stanford Dragon (PLY)
# ---------------------------------------------------------------------------
Download-Asset -Name "Stanford Dragon" `
    -Url "http://graphics.stanford.edu/pub/3Dscanrep/dragon/dragon_recon.tar.gz" `
    -TargetDir (Join-Path $modelsDir "dragon") `
    -PostProcess {
        param($archive, $dest)
        tar -xzf $archive -C $dest --strip-components=1 2>$null
        $ply = Get-ChildItem -Path $dest -Recurse -Filter "dragon_vrip.ply" | Select-Object -First 1
        if ($ply) {
            Copy-Item $ply.FullName (Join-Path $dest "dragon.ply") -Force
            Write-Host "  Extracted dragon.ply (convert to OBJ with a mesh tool if needed)"
        }
    }

# ---------------------------------------------------------------------------
# ShaderBall / Material Orb (FBX, public domain)
# ---------------------------------------------------------------------------
Download-Asset -Name "ShaderBall (Material Orb)" `
    -Url "https://github.com/derkreature/ShaderBall/archive/refs/heads/master.zip" `
    -TargetDir (Join-Path $modelsDir "orb") `
    -PostProcess {
        param($archive, $dest)
        Expand-Archive -Path $archive -DestinationPath $tempDir -Force
        $extracted = Join-Path $tempDir "ShaderBall-master"
        $fbx = Get-ChildItem -Path $extracted -Recurse -Filter "*.fbx" | Select-Object -First 1
        if ($fbx) {
            Copy-Item $fbx.FullName (Join-Path $dest "ShaderBall.fbx") -Force
            Write-Host "  Extracted ShaderBall.fbx"
        }
        # Copy any textures
        Get-ChildItem -Path $extracted -Recurse -Include "*.png","*.jpg","*.tga" | ForEach-Object {
            Copy-Item $_.FullName $dest -Force
        }
    }

# ---------------------------------------------------------------------------
# Intel Sponza Base
# ---------------------------------------------------------------------------
Download-Asset -Name "Intel Sponza Base" `
    -Url "https://cdrdv2.intel.com/v1/dl/getContent/830833?fileName=main1_sponza.zip" `
    -TargetDir (Join-Path $modelsDir "Sponza_PBR") `
    -PostProcess {
        param($archive, $dest)
        Expand-Archive -Path $archive -DestinationPath $dest -Force
    }

# ---------------------------------------------------------------------------
# Intel Colorful Curtains
# ---------------------------------------------------------------------------
Download-Asset -Name "Intel Colorful Curtains" `
    -Url "https://cdrdv2.intel.com/v1/dl/getContent/726650?explicitVersion=true&fileName=PKG_A_Curtains.zip" `
    -TargetDir (Join-Path $modelsDir "Sponza_Curtains") `
    -PostProcess {
        param($archive, $dest)
        Expand-Archive -Path $archive -DestinationPath $dest -Force
    }

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------
if (Test-Path $tempDir) {
    Remove-Item $tempDir -Recurse -Force
}

Write-Host ""
Write-Host "Asset download complete." -ForegroundColor Green
Write-Host "Run the engine and press Y to import models (converts to engine format)."
