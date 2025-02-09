# Set script paths.
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
$dxilOutputDir = Join-Path $scriptRoot "..\Res\Shaders\DXIL"
$hlslSourceDir = Join-Path $scriptRoot "..\Res\Shaders\HLSL"
$dxcExePath = Join-Path $scriptRoot "../build/Tools/dxc/bin/x64/dxc.exe"

# Ensure the DXIL output directory exists.
if (!(Test-Path $dxilOutputDir)) {
    Write-Host "Creating DXIL output directory: $dxilOutputDir"
    New-Item -ItemType Directory -Path $dxilOutputDir | Out-Null
}

# Get all shader files (both raster and ray tracing)
Write-Host "Collecting shaders from $hlslSourceDir..."
$shaderFiles = Get-ChildItem -Path $hlslSourceDir -File | Where-Object { $_.Extension -eq ".hlsl" -or $_.Extension -match "\.(vert|frag|tesc|tese|geom|comp)$" }
if ($shaderFiles.Count -eq 0) {
    Write-Host "No shader files found in $hlslSourceDir. Exiting."
    exit
}

# Define common compilation settings.
$commonArgs = @("-Wno-ignored-attributes", "-Qembed_debug", "/Zi", "/Zss")

# Track shader dependencies (which shaders include which files)
$shaderDependencies = @{}

foreach ($file in $shaderFiles) {
    $sourcePath = $file.FullName
    $shaderDependencies[$file.Name] = @()

    # Read the file to find #include dependencies
    $content = Get-Content -Path $sourcePath
    foreach ($line in $content) {
        if ($line -match '^\s*#include\s*"(.+?)"') {
            $includedFile = $matches[1]
            $shaderDependencies[$file.Name] += $includedFile
        }
    }
}

# Track which shaders need recompilation
$recompileShaders = @{}

foreach ($file in $shaderFiles) {
    $fileName = $file.Name  # Keep full filename (e.g., `foo.vert`)
    $sourcePath = $file.FullName
    $outputPath = Join-Path $dxilOutputDir "$fileName.dxil"  # Preserve full filename

    # Default to DXR (ray tracing) profile and entry point
    $targetProfile = "lib_6_3"
    $entryPoint = "main"

    # Detect rasterization shaders by their extension
    if ($file.Extension -eq ".vert") {
        $targetProfile = "vs_6_3"
    }
    elseif ($file.Extension -eq ".frag") {
        $targetProfile = "ps_6_3"
    }
    elseif ($file.Extension -eq ".tesc") {
        $targetProfile = "hs_6_3"
    }
    elseif ($file.Extension -eq ".tese") {
        $targetProfile = "ds_6_3"
    }
    elseif ($file.Extension -eq ".geom") {
        $targetProfile = "gs_6_3"
    }
    elseif ($file.Extension -eq ".comp") {
        $targetProfile = "cs_6_3"
    }
    else {
        # Any other .hlsl file is considered a DXR shader
        $targetProfile = "lib_6_3"
        if ($file.Name -match "RayGen") {
            $entryPoint = "RayGenShader"
        }
        elseif ($file.Name -match "ClosestHit") {
            $entryPoint = "ClosestHitShader"
        }
        elseif ($file.Name -match "AnyHit") {
            $entryPoint = "AnyHitShader"
        }
        elseif ($file.Name -match "Miss") {
            $entryPoint = "MissShader"
        }
    }

    # Check if the shader needs to be recompiled
    $recompile = $true
    if (Test-Path $outputPath) {
        $sourceTime = (Get-Item $sourcePath).LastWriteTimeUtc
        $outputTime = (Get-Item $outputPath).LastWriteTimeUtc

        # If the source file is newer than the output, recompile
        if ($sourceTime -le $outputTime) {
            $recompile = $false
        }

        # Also check included files
        foreach ($includedFile in $shaderDependencies[$file.Name]) {
            $includedFilePath = Join-Path $hlslSourceDir $includedFile
            if (Test-Path $includedFilePath) {
                $includedTime = (Get-Item $includedFilePath).LastWriteTimeUtc
                if ($includedTime -gt $outputTime) {
                    Write-Host "Dependency changed: $includedFile -> Recompiling $fileName"
                    $recompile = $true
                }
            }
        }
    }

    # Skip compilation if the file hasn't changed and dependencies are up-to-date
    if (-not $recompile) {
        Write-Host "Skipping unchanged shader: $fileName"
        continue
    }

    # Mark this shader for recompilation
    $recompileShaders[$file.Name] = $true

    # Build the argument list.
    $args = @(
        "-T", $targetProfile,
        "-E", $entryPoint,
        "-Fo", $outputPath,
        $sourcePath
    ) + $commonArgs

    Write-Host "Compiling $($file.Name) -> $outputPath (Profile: $targetProfile)"

    # Start the DXC process.
    $process = Start-Process -FilePath $dxcExePath -ArgumentList $args -NoNewWindow -Wait -PassThru

    if ($process.ExitCode -ne 0) {
        Write-Error "Error compiling $($file.Name). Exit code: $($process.ExitCode)"
    }
    else {
        Write-Host "Successfully compiled $($file.Name)."
    }
}

Write-Host "Shader compilation complete."
Pause
