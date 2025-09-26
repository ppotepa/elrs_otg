# ELRS OTG Demo Build Script - Updated for full C++ implementation
param(
    [string]$BuildType = "Release",
    [switch]$Clean = $false,
    [switch]$WithUSB = $false
)

Write-Host "Building ELRS OTG Demo - Full C++ 2.4GHz Radio Controller..." -ForegroundColor Green

# Find CMake
$cmake = $null
$cmakePaths = @(
    "cmake",
    "C:\Program Files\CMake\bin\cmake.exe",
    "C:\Program Files (x86)\CMake\bin\cmake.exe"
)

foreach ($path in $cmakePaths) {
    try {
        if ($path -eq "cmake") {
            $null = Get-Command cmake -ErrorAction Stop
            $cmake = "cmake"
        }
        elseif (Test-Path $path) {
            $cmake = $path
        }
        if ($cmake) { break }
    }
    catch { }
}

if (-not $cmake) {
    Write-Host "❌ CMake not found. Please install CMake from https://cmake.org/download/" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Using CMake: $cmake" -ForegroundColor Green

# Clean build if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "🧹 Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

# Create build directory
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
    Write-Host "📁 Created build directory" -ForegroundColor Blue
}

# Configure and build
Set-Location "build"

try {
    Write-Host "🔧 Configuring project..." -ForegroundColor Blue
    & $cmake .. -DCMAKE_BUILD_TYPE=$BuildType
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    Write-Host "🏗️ Building project..." -ForegroundColor Blue
    & $cmake --build . --config $BuildType
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    Write-Host "🎉 Build completed successfully!" -ForegroundColor Green
    
    # Find the executable
    $exePaths = @("elrs_otg_demo.exe", "Release\elrs_otg_demo.exe", "Debug\elrs_otg_demo.exe")
    $foundExe = $null
    foreach ($exePath in $exePaths) {
        if (Test-Path $exePath) {
            $foundExe = $exePath
            break
        }
    }
    
    if ($foundExe) {
        Write-Host "📦 Executable: build\$foundExe" -ForegroundColor Green
    }
    
}
catch {
    Write-Host "❌ Build failed: $($_.Exception.Message)" -ForegroundColor Red
    Set-Location ..
    exit 1
}
finally {
    Set-Location ..
}

Write-Host ""
Write-Host "🚀 To run: .\run.ps1" -ForegroundColor Cyan
