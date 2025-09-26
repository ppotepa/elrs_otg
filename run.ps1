# ELRS OTG Demo Run Script

Write-Host "Running ELRS OTG Demo..." -ForegroundColor Green

# Find the executable
$exePaths = @(
    "build\elrs_otg_demo.exe",
    "build\Release\elrs_otg_demo.exe", 
    "build\Debug\elrs_otg_demo.exe"
)

$exePath = $null
foreach ($path in $exePaths) {
    if (Test-Path $path) {
        $exePath = $path
        break
    }
}

if (-not $exePath) {
    Write-Host "❌ Executable not found. Please run .\build.ps1 first" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Found: $exePath" -ForegroundColor Green

# Check for libusb
$buildDir = Split-Path $exePath -Parent
$dllPath = Join-Path $buildDir "libusb-1.0.dll"
if (Test-Path $dllPath) {
    Write-Host "✅ LibUSB available - Full USB support enabled" -ForegroundColor Green
}
else {
    Write-Host "⚠️ LibUSB not found - Limited functionality" -ForegroundColor Yellow
    Write-Host "   Download libusb from https://libusb.info/ and place files in lib/" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Starting application..." -ForegroundColor Blue
Write-Host "=" * 50 -ForegroundColor Gray

# Run the application
& $exePath
