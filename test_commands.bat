@echo off
REM ELRS OTG Demo - Windows Command Line Testing
echo.
echo === ELRS OTG Demo - Command Line Interface ===
echo.

set exe=.\build\Release\elrs_otg_demo.exe

if not exist "%exe%" (
    echo ERROR: Executable not found: %exe%
    echo Run build.ps1 first to build the application
    exit /b 1
)

echo 1. Testing Help Command:
%exe% help
echo.

echo 2. Testing Device Scan:
%exe% scan
echo.

echo 3. Testing Serial Connection:
%exe% init-serial
echo.

echo 4. Testing Control Inputs:
%exe% test-controls
echo.

echo 5. Testing Model Setting:
%exe% set-model 3
echo.

echo 6. Testing Power Commands:
%exe% power-up
%exe% power-down
echo.

echo 7. Testing Status Display:
%exe% status
echo.

echo === All Command-Line Tests Complete ===
echo.
echo Available commands:
echo   - Connection: scan, init, init-serial
echo   - Control: start-tx, stop-tx, test-controls, test-elrs, test-safety
echo   - Configuration: set-model, power-up, power-down, power-menu
echo   - Monitoring: status, monitor, rescan
echo   - Management: disconnect, reconnect, exit
echo   - Testing: test-all (runs complete suite)
echo.
echo Perfect for automation, testing, and CI/CD pipelines!