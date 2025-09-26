# ELRS OTG Demo - Driver Management Test Script
# Tests all driver-related functionality

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "    ELRS OTG Demo - Driver Management Test    " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

$exe = ".\build\Release\elrs_otg_demo.exe"

if (-not (Test-Path $exe)) {
    Write-Host "❌ Executable not found: $exe" -ForegroundColor Red
    Write-Host "💡 Run: .\build.ps1 first" -ForegroundColor Yellow
    exit 1
}

Write-Host "📋 Testing driver management commands..." -ForegroundColor Green
Write-Host ""

# Test 1: Driver Information
Write-Host "🔍 Test 1: Driver Information" -ForegroundColor Blue
Write-Host "Command: $exe driver-info" -ForegroundColor Gray
& $exe driver-info
Write-Host ""
Start-Sleep -Seconds 1

# Test 2: Check Driver Status
Write-Host "🔍 Test 2: Check Driver Status" -ForegroundColor Blue
Write-Host "Command: $exe check-driver" -ForegroundColor Gray
& $exe check-driver
Write-Host ""
Start-Sleep -Seconds 1

# Test 3: Verify Driver Files
Write-Host "📁 Test 3: Verify Driver Files" -ForegroundColor Blue
$driverPath = ".\build\Release\platform\win\drv"
if (Test-Path $driverPath) {
    Write-Host "✅ Driver folder exists: $driverPath" -ForegroundColor Green
    $files = Get-ChildItem $driverPath -Recurse | Where-Object { -not $_.PSIsContainer }
    Write-Host "📂 Driver files found:" -ForegroundColor Cyan
    foreach ($file in $files) {
        $relativePath = $file.FullName.Replace((Get-Location).Path + "\", "")
        Write-Host "   • $relativePath" -ForegroundColor White
    }
}
else {
    Write-Host "❌ Driver folder not found: $driverPath" -ForegroundColor Red
}
Write-Host ""

# Test 4: Install Driver (Note: Requires Admin)
Write-Host "🔧 Test 4: Install Driver (Admin Required)" -ForegroundColor Blue
Write-Host "Command: $exe install-driver" -ForegroundColor Gray
Write-Host "Note: This will request administrator privileges..." -ForegroundColor Yellow
& $exe install-driver
Write-Host ""
Start-Sleep -Seconds 1

# Test 5: Check Driver After Installation Attempt
Write-Host "🔍 Test 5: Check Driver Status After Installation" -ForegroundColor Blue
Write-Host "Command: $exe check-driver" -ForegroundColor Gray
& $exe check-driver
Write-Host ""

# Test 6: Device Scan After Driver
Write-Host "📡 Test 6: Device Scan" -ForegroundColor Blue
Write-Host "Command: $exe scan" -ForegroundColor Gray
& $exe scan
Write-Host ""

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "           Driver Test Summary                 " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "✅ Driver info command tested" -ForegroundColor Green
Write-Host "✅ Driver check command tested" -ForegroundColor Green
Write-Host "✅ Driver file verification tested" -ForegroundColor Green
Write-Host "⚠️  Driver installation tested (admin required)" -ForegroundColor Yellow
Write-Host "✅ Device scanning tested" -ForegroundColor Green
Write-Host ""
Write-Host "💡 Next steps:" -ForegroundColor Cyan
Write-Host "   1. Run as Administrator to install driver" -ForegroundColor White
Write-Host "   2. Connect ELRS device after driver installation" -ForegroundColor White
Write-Host "   3. Test: .\build\Release\elrs_otg_demo.exe init-serial" -ForegroundColor White
Write-Host ""