@echo off
echo ===============================================
echo     ELRS OTG Demo - Driver Management Test    
echo ===============================================
echo.

set exe=.\build\Release\elrs_otg_demo.exe

if not exist "%exe%" (
    echo ❌ Executable not found: %exe%
    echo 💡 Run: .\build.ps1 first
    pause
    exit /b 1
)

echo 📋 Testing driver management commands...
echo.

echo 🔍 Test 1: Driver Information
echo Command: %exe% driver-info
%exe% driver-info
echo.
timeout /t 2 > nul

echo 🔍 Test 2: Check Driver Status
echo Command: %exe% check-driver
%exe% check-driver
echo.
timeout /t 2 > nul

echo 📁 Test 3: Verify Driver Files
set driverPath=.\build\Release\platform\win\drv
if exist "%driverPath%" (
    echo ✅ Driver folder exists: %driverPath%
    echo 📂 Driver files found:
    dir /s /b "%driverPath%\*.*" | findstr /v "Directory"
) else (
    echo ❌ Driver folder not found: %driverPath%
)
echo.

echo 🔧 Test 4: Install Driver (Admin Required)
echo Command: %exe% install-driver
echo Note: This will request administrator privileges...
%exe% install-driver
echo.
timeout /t 2 > nul

echo 🔍 Test 5: Check Driver Status After Installation
echo Command: %exe% check-driver
%exe% check-driver
echo.

echo 📡 Test 6: Device Scan
echo Command: %exe% scan
%exe% scan
echo.

echo ===============================================
echo            Driver Test Summary
echo ===============================================
echo.
echo ✅ Driver info command tested
echo ✅ Driver check command tested
echo ✅ Driver file verification tested
echo ⚠️  Driver installation tested (admin required)
echo ✅ Device scanning tested
echo.
echo 💡 Next steps:
echo    1. Run as Administrator to install driver
echo    2. Connect ELRS device after driver installation
echo    3. Test: %exe% init-serial
echo.
pause