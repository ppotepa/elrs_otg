# ELRS OTG Demo - Enhanced Device Detection Test
# Tests the new unknown device detection functionality

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "  ELRS OTG Demo - Enhanced Device Detection   " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

$exe = ".\build\Release\elrs_otg_demo.exe"

if (-not (Test-Path $exe)) {
    Write-Host "❌ Executable not found: $exe" -ForegroundColor Red
    Write-Host "💡 Run: .\build.ps1 first" -ForegroundColor Yellow
    exit 1
}

Write-Host "🔍 Testing enhanced device detection..." -ForegroundColor Green
Write-Host ""

# Test 1: Regular Enhanced Scan
Write-Host "🔍 Test 1: Enhanced Device Scan (includes unknown device detection)" -ForegroundColor Blue
Write-Host "Command: $exe scan" -ForegroundColor Gray
& $exe scan
Write-Host ""
Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
Write-Host ""

# Test 2: Unknown Device Scan Only
Write-Host "🔍 Test 2: Unknown Device Scan Only" -ForegroundColor Blue  
Write-Host "Command: $exe scan-unknown" -ForegroundColor Gray
& $exe scan-unknown
Write-Host ""
Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
Write-Host ""

# Test 3: Driver Status Check
Write-Host "🔍 Test 3: Driver Status Check" -ForegroundColor Blue
Write-Host "Command: $exe check-driver" -ForegroundColor Gray
& $exe check-driver
Write-Host ""
Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
Write-Host ""

# Test 4: Complete Workflow Demonstration
Write-Host "🚀 Test 4: Complete ELRS Setup Workflow" -ForegroundColor Blue
Write-Host ""
Write-Host "Step 1: Check for unknown devices..." -ForegroundColor Yellow
& $exe scan-unknown
Write-Host ""

Write-Host "Step 2: Check driver status..." -ForegroundColor Yellow
& $exe check-driver
Write-Host ""

Write-Host "Step 3: If devices found without drivers, install driver..." -ForegroundColor Yellow
Write-Host "Command: $exe install-driver (requires admin)" -ForegroundColor Gray
Write-Host "Note: This would require running as Administrator" -ForegroundColor Red
Write-Host ""

Write-Host "Step 4: After driver installation, scan again..." -ForegroundColor Yellow
Write-Host "Command: $exe scan (should now find COM ports)" -ForegroundColor Gray
Write-Host ""

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "            Detection Summary                  " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

# Check what was detected
Write-Host "🔍 Final Detection Check:" -ForegroundColor Green
& $exe scan-unknown

Write-Host ""
Write-Host "📋 Key Improvements:" -ForegroundColor Cyan
Write-Host "  ✅ Detects physically connected ELRS devices" -ForegroundColor Green
Write-Host "  ✅ Identifies devices that need drivers" -ForegroundColor Green  
Write-Host "  ✅ Distinguishes Silicon Labs CP210x devices" -ForegroundColor Green
Write-Host "  ✅ Shows hardware IDs and device descriptions" -ForegroundColor Green
Write-Host "  ✅ Provides clear installation instructions" -ForegroundColor Green
Write-Host "  ✅ Enhanced status reporting in regular scan" -ForegroundColor Green
Write-Host ""
Write-Host "💡 Workflow for users:" -ForegroundColor Cyan
Write-Host "   1. Connect ELRS device via USB-C" -ForegroundColor White
Write-Host "   2. Run: $exe scan" -ForegroundColor White
Write-Host "   3. If 'devices detected but need drivers' - install driver" -ForegroundColor White
Write-Host "   4. Run as admin: $exe install-driver" -ForegroundColor White
Write-Host "   5. Scan again to confirm: $exe scan" -ForegroundColor White
Write-Host "   6. Initialize connection: $exe init-serial" -ForegroundColor White
Write-Host ""