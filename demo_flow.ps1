#!/usr/bin/env powershell
# ELRS OTG Demo - New Auto-Initialization Flow Demo

Write-Host ""
Write-Host "🚁 ELRS OTG Demo - New Auto-Initialization Flow" -ForegroundColor Green
Write-Host "===============================================" -ForegroundColor Green
Write-Host ""

Write-Host "✨ New Features:" -ForegroundColor Cyan
Write-Host "  🔄 Automatic initialization on startup"
Write-Host "  🔍 Auto-detection of ELRS devices"
Write-Host "  🔌 Smart connection (COM port first, then USB)"
Write-Host "  📋 Streamlined menu focused on testing"
Write-Host ""

Write-Host "🎯 New Flow:" -ForegroundColor Yellow
Write-Host "  1. App starts"
Write-Host "  2. Initializing..."
Write-Host "  3. Finding devices..."
Write-Host "  4. Device found: [name] - [info]"
Write-Host "  5. Auto-connect (COM port @ 420kbaud preferred)"
Write-Host "  6. Menu options for testing"
Write-Host ""

Write-Host "🚀 Starting Demo..." -ForegroundColor Green
Write-Host ""

# Run the application
& ".\build\Release\elrs_otg_demo.exe"

Write-Host ""
Write-Host "✅ Demo completed!" -ForegroundColor Green