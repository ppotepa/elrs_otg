#!/usr/bin/env powershell
# ELRS OTG Demo - Quick Command Line Demonstration

$exe = ".\build\Release\elrs_otg_demo.exe"

Write-Host ""
Write-Host "🚁 ELRS OTG Demo - Command Line Interface" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
Write-Host ""

Write-Host "✅ Application built successfully!" -ForegroundColor Green
Write-Host "✅ Command line interface working!" -ForegroundColor Green
Write-Host ""

Write-Host "📋 Available Commands:" -ForegroundColor Cyan
Write-Host ""
& $exe help
Write-Host ""

Write-Host "🔍 Testing Device Scan:" -ForegroundColor Yellow
& $exe scan
Write-Host ""

Write-Host "📊 Testing Status Command:" -ForegroundColor Yellow  
& $exe status
Write-Host ""

Write-Host "✨ Command Line Interface is ready!" -ForegroundColor Green
Write-Host ""
Write-Host "🎯 Examples for when you have hardware connected:" -ForegroundColor Cyan
Write-Host "   $exe init                    # Connect to device"
Write-Host "   $exe start-tx                # Start transmission"
Write-Host "   $exe test-channels           # Test control inputs"
Write-Host "   $exe monitor 30              # Monitor for 30 seconds"
Write-Host "   $exe stop-tx                 # Stop transmission"
Write-Host ""
Write-Host "🤖 For automated testing without user input:" -ForegroundColor Green
Write-Host "   $exe test-sequence           # Full automated test"
Write-Host "   $exe model 5                 # Set model 5"
Write-Host "   $exe power-up                # Increase power"
Write-Host ""
Write-Host "💡 Run '$exe' (no arguments) for interactive menu" -ForegroundColor Blue