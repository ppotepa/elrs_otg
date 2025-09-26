#!/usr/bin/env powershell
# ELRS OTG Demo - Device Monitoring Flow Demo

Write-Host ""
Write-Host "🚁 ELRS OTG Demo - Device Monitoring Flow" -ForegroundColor Green
Write-Host "===========================================" -ForegroundColor Green
Write-Host ""

Write-Host "✨ New Behavior:" -ForegroundColor Cyan
Write-Host "  ✅ App starts and initializes"
Write-Host "  🔍 Automatically scans for devices"
Write-Host "  📱 If device found: Shows menu immediately"
Write-Host "  ⏳ If NO device found: Starts monitoring thread"
Write-Host "  🔌 Waits for device to be plugged in"
Write-Host "  🚫 NO menu shown until device is connected"
Write-Host ""

Write-Host "🎯 Flow Demonstration:" -ForegroundColor Yellow
Write-Host "  1. App starts"
Write-Host "  2. Initializing..."
Write-Host "  3. Finding devices..."
Write-Host "  4. No devices found"
Write-Host "  5. Starting device monitor..."
Write-Host "  6. Waiting for ELRS device to be connected..."
Write-Host "  7. (Scanning every 2 seconds...)"
Write-Host "  8. Device plugged in → Detected → Connected → Menu shown"
Write-Host ""

Write-Host "🔄 Running Demo (will wait for device):" -ForegroundColor Green
Write-Host "   Press Ctrl+C to stop monitoring" -ForegroundColor Yellow
Write-Host ""

# Run the application - it will wait for device
& ".\build\Release\elrs_otg_demo.exe"