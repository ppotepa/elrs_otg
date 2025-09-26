#!/usr/bin/env pwsh
# ELRS OTG Demo - Command Line Testing Script
# This script demonstrates all available command-line options

Write-Host "🧪 ELRS OTG Demo - Command Line Testing Suite" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green
Write-Host ""

$exe = ".\build\Release\elrs_otg_demo.exe"

if (-not (Test-Path $exe)) {
    Write-Host "❌ Executable not found: $exe" -ForegroundColor Red
    Write-Host "Run .\build.ps1 first to build the application" -ForegroundColor Yellow
    exit 1
}

Write-Host "📚 1. Showing help..." -ForegroundColor Cyan
& $exe help
Write-Host ""

Write-Host "🔍 2. Scanning for devices..." -ForegroundColor Cyan
& $exe scan
Write-Host ""

Write-Host "🔗 3. Initializing serial connection..." -ForegroundColor Cyan
& $exe init-serial
Write-Host ""

Write-Host "📊 4. Showing device status..." -ForegroundColor Cyan
& $exe status
Write-Host ""

Write-Host "🎮 5. Testing control inputs..." -ForegroundColor Cyan
& $exe test-controls
Write-Host ""

Write-Host "📡 6. Testing ELRS commands..." -ForegroundColor Cyan
& $exe test-elrs
Write-Host ""

Write-Host "🛡️  7. Testing safety features..." -ForegroundColor Cyan
& $exe test-safety
Write-Host ""

Write-Host "⚡ 8. Testing power control menu..." -ForegroundColor Cyan
& $exe power-menu  
& $exe power-up
Write-Host ""

Write-Host "6. Testing Power Down Command:" -ForegroundColor Yellow
& $exe power-down
Write-Host ""

# Test 7: Test telemetry request (will fail without device)
Write-Host "7. Testing Telemetry Request:" -ForegroundColor Yellow
& $exe telemetry
Write-Host ""

# Test 8: Test monitor command with custom duration (will fail without device)
Write-Host "8. Testing Monitor Command (5 seconds):" -ForegroundColor Yellow
& $exe monitor 5
Write-Host ""

# Test 9: Test invalid command
Write-Host "9. Testing Invalid Command:" -ForegroundColor Yellow
& $exe invalid-command-test
Write-Host ""

Write-Host "=== Command Line Test Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Available Commands:" -ForegroundColor Cyan
Write-Host "• $exe help                  - Show help"
Write-Host "• $exe scan                  - Scan for devices"
Write-Host "• $exe init                  - Initialize and connect"
Write-Host "• $exe status                - Show device status" 
Write-Host "• $exe model <1-8>           - Set model ID"
Write-Host "• $exe power-up              - Increase TX power"
Write-Host "• $exe power-down            - Decrease TX power"
Write-Host "• $exe start-tx              - Start transmission"
Write-Host "• $exe stop-tx               - Stop transmission"
Write-Host "• $exe test-channels         - Test control inputs"
Write-Host "• $exe test-sequence         - Full test sequence"
Write-Host "• $exe monitor <seconds>     - Monitor telemetry"
Write-Host "• $exe bind                  - Put in bind mode"
Write-Host "• $exe arm                   - Arm device (DANGEROUS!)"
Write-Host "• $exe disarm                - Disarm device"
Write-Host "• $exe emergency             - Emergency stop"
Write-Host "• $exe telemetry             - Request telemetry"
Write-Host "• $exe                       - Interactive menu"
Write-Host ""
Write-Host "Example automated test sequence:" -ForegroundColor Green
Write-Host "  $exe init && $exe start-tx && $exe test-channels && $exe stop-tx"