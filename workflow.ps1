# ELRS OTG Demo - Complete Setup and Test Workflow
# This script demonstrates the full driver installation and testing workflow

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "   ELRS OTG Demo - Complete Workflow Guide    " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

$exe = ".\build\Release\elrs_otg_demo.exe"

if (-not (Test-Path $exe)) {
    Write-Host "❌ Building project first..." -ForegroundColor Red
    .\build.ps1
    if (-not (Test-Path $exe)) {
        Write-Host "❌ Build failed!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "🚀 ELRS OTG Demo - Complete Setup Workflow" -ForegroundColor Green
Write-Host ""

# Step 1: Check current driver status
Write-Host "📋 Step 1: Check Driver Status" -ForegroundColor Blue
Write-Host "Command: $exe check-driver" -ForegroundColor Gray
& $exe check-driver
Write-Host ""

# Step 2: Show available commands
Write-Host "📋 Step 2: Available Commands" -ForegroundColor Blue
Write-Host "Driver Management:" -ForegroundColor Yellow
Write-Host "  • check-driver       - Check if CP210x driver is installed" -ForegroundColor White
Write-Host "  • install-driver     - Install CP210x driver (requires admin)" -ForegroundColor White
Write-Host "  • uninstall-driver   - Uninstall CP210x driver (requires admin)" -ForegroundColor White
Write-Host "  • driver-info        - Show driver installation information" -ForegroundColor White
Write-Host ""
Write-Host "Device Connection:" -ForegroundColor Yellow
Write-Host "  • scan               - Scan for USB and COM port devices" -ForegroundColor White
Write-Host "  • init               - Initialize USB connection" -ForegroundColor White
Write-Host "  • init-serial        - Initialize COM port connection (420kbaud)" -ForegroundColor White
Write-Host ""
Write-Host "ELRS Control (after connection):" -ForegroundColor Yellow
Write-Host "  • start-tx           - Start CRSF transmission (250Hz)" -ForegroundColor White
Write-Host "  • stop-tx            - Stop CRSF transmission" -ForegroundColor White
Write-Host "  • test-controls      - Test control inputs" -ForegroundColor White
Write-Host "  • test-elrs          - Test ELRS commands" -ForegroundColor White
Write-Host "  • test-all           - Run complete test suite" -ForegroundColor White
Write-Host "  • monitor [seconds]  - Monitor telemetry" -ForegroundColor White
Write-Host ""

# Step 3: Typical workflow
Write-Host "📋 Step 3: Typical Workflow" -ForegroundColor Blue
Write-Host "1. Check driver:     $exe check-driver" -ForegroundColor Cyan
Write-Host "2. Install driver:   $exe install-driver    (as admin)" -ForegroundColor Cyan
Write-Host "3. Connect device:   Connect ELRS transmitter via USB-C" -ForegroundColor Cyan
Write-Host "4. Initialize:       $exe init-serial" -ForegroundColor Cyan
Write-Host "5. Test controls:    $exe test-controls" -ForegroundColor Cyan
Write-Host "6. Start TX:         $exe start-tx" -ForegroundColor Cyan
Write-Host "7. Monitor:          $exe monitor 30" -ForegroundColor Cyan
Write-Host "8. Stop TX:          $exe stop-tx" -ForegroundColor Cyan
Write-Host ""

# Step 4: Device scan
Write-Host "📋 Step 4: Current Device Scan" -ForegroundColor Blue
Write-Host "Command: $exe scan" -ForegroundColor Gray
& $exe scan
Write-Host ""

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "              Next Steps                       " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if ($isAdmin) {
    Write-Host "✅ Running as Administrator" -ForegroundColor Green
    Write-Host "💡 You can install drivers with: $exe install-driver" -ForegroundColor Cyan
}
else {
    Write-Host "⚠️  Not running as Administrator" -ForegroundColor Yellow
    Write-Host "💡 To install drivers, run PowerShell as Administrator" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "🔌 To connect your ELRS device:" -ForegroundColor Green
Write-Host "   1. Install driver (if needed): $exe install-driver" -ForegroundColor White
Write-Host "   2. Connect ELRS transmitter via USB-C cable" -ForegroundColor White
Write-Host "   3. Initialize connection: $exe init-serial" -ForegroundColor White
Write-Host "   4. Start testing: $exe test-all" -ForegroundColor White
Write-Host ""
Write-Host "📖 For interactive mode: $exe" -ForegroundColor Green
Write-Host "📖 For help: $exe help" -ForegroundColor Green
Write-Host ""