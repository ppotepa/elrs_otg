#!/usr/bin/env pwsh
# Setup script for libusb-1.0 on Windows
# This script downloads and sets up libusb for the ELRS OTG Demo

Write-Host "ELRS OTG Demo - LibUSB Setup Script" -ForegroundColor Green
Write-Host "====================================" -ForegroundColor Green
Write-Host ""

$libusb_version = "1.0.27"
$libusb_url = "https://github.com/libusb/libusb/releases/download/v$libusb_version/libusb-$libusb_version.7z"
$lib_dir = Join-Path $PSScriptRoot "lib"
$temp_file = Join-Path $env:TEMP "libusb-$libusb_version.7z"

# Create lib directory if it doesn't exist
if (!(Test-Path $lib_dir)) {
    New-Item -ItemType Directory -Path $lib_dir -Force | Out-Null
    Write-Host "[INFO] Created lib directory: $lib_dir" -ForegroundColor Yellow
}

Write-Host "[INFO] Downloading libusb-$libusb_version..." -ForegroundColor Yellow
try {
    Invoke-WebRequest -Uri $libusb_url -OutFile $temp_file -UseBasicParsing
    Write-Host "[OK] Downloaded libusb-$libusb_version.7z" -ForegroundColor Green
}
catch {
    Write-Host "[ERROR] Failed to download libusb: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "[INFO] Please download manually from: $libusb_url" -ForegroundColor Yellow
    exit 1
}

# Check if 7-Zip is available
$seven_zip = Get-Command "7z" -ErrorAction SilentlyContinue
if (!$seven_zip) {
    # Try common 7-Zip installation paths
    $seven_zip_paths = @(
        "C:\Program Files\7-Zip\7z.exe",
        "C:\Program Files (x86)\7-Zip\7z.exe"
    )
    
    foreach ($path in $seven_zip_paths) {
        if (Test-Path $path) {
            $seven_zip = Get-Command $path
            break
        }
    }
}

if (!$seven_zip) {
    Write-Host "[ERROR] 7-Zip not found. Please install 7-Zip or extract libusb-$libusb_version.7z manually to $lib_dir" -ForegroundColor Red
    Write-Host "[INFO] Download 7-Zip from: https://www.7-zip.org/download.html" -ForegroundColor Yellow
    Write-Host "[INFO] Libusb archive saved to: $temp_file" -ForegroundColor Yellow
    exit 1
}

Write-Host "[INFO] Extracting libusb..." -ForegroundColor Yellow
try {
    $extract_dir = Join-Path $env:TEMP "libusb-extract"
    & $seven_zip.Source x $temp_file "-o$extract_dir" -y | Out-Null
    
    # Find the libusb directory structure
    $libusb_root = Get-ChildItem $extract_dir -Directory | Select-Object -First 1
    
    if ($libusb_root) {
        # Copy header files
        $include_src = Join-Path $libusb_root.FullName "include\libusb-1.0"
        $include_dst = Join-Path $lib_dir "libusb-1.0"
        
        if (Test-Path $include_src) {
            Copy-Item -Path $include_src -Destination $include_dst -Recurse -Force
            Write-Host "[OK] Copied header files to $include_dst" -ForegroundColor Green
        }
        
        # Copy library files (try different architectures)
        $lib_arch_dirs = @("MS64\dll", "MS32\dll", "MinGW64\dll", "MinGW32\dll")
        $lib_copied = $false
        
        foreach ($arch_dir in $lib_arch_dirs) {
            $lib_src = Join-Path $libusb_root.FullName $arch_dir
            if (Test-Path $lib_src) {
                Copy-Item -Path "$lib_src\*" -Destination $lib_dir -Force
                Write-Host "[OK] Copied library files from $arch_dir" -ForegroundColor Green
                $lib_copied = $true
                break
            }
        }
        
        if (!$lib_copied) {
            Write-Host "[WARN] Could not find suitable library files" -ForegroundColor Yellow
        }
    }
    
    # Cleanup
    Remove-Item $temp_file -Force -ErrorAction SilentlyContinue
    Remove-Item $extract_dir -Recurse -Force -ErrorAction SilentlyContinue
    
    Write-Host "[OK] LibUSB setup completed!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "1. Run: .\build.ps1" -ForegroundColor White
    Write-Host "2. Test: .\build\Release\elrs_otg_demo.exe scan" -ForegroundColor White
    Write-Host ""
}
catch {
    Write-Host "[ERROR] Failed to extract libusb: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}