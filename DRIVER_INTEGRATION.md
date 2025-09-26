# ELRS OTG Demo - Driver Management Integration

## Overview
The ELRS OTG Demo now includes comprehensive CP210x driver management functionality, allowing complete automation of the driver installation process using locally provided driver files.

## New Features Added

### 1. Driver Management Commands
- `install-driver` - Install CP210x driver from local files (requires admin)
- `uninstall-driver` - Uninstall CP210x driver (requires admin)
- `check-driver` - Check if CP210x driver is installed
- `driver-info` - Show driver installation information

### 2. Local Driver Files
- CP210x Universal Windows Driver v11.4.0.393
- Located in `platform/win/drv/` folder
- Supports all architectures: x86, x64, ARM, ARM64
- Automatically copied to build output during compilation

### 3. Enhanced CLI Interface
- All driver commands work without device connection
- Smart initialization skipping for driver-only commands
- Comprehensive error handling and user guidance
- Administrator privilege detection and requirements

## Usage Examples

### Basic Driver Management
```bash
# Check current driver status
.\build\Release\elrs_otg_demo.exe check-driver

# Show driver information
.\build\Release\elrs_otg_demo.exe driver-info

# Install driver (requires admin privileges)
.\build\Release\elrs_otg_demo.exe install-driver

# Uninstall driver (requires admin privileges)
.\build\Release\elrs_otg_demo.exe uninstall-driver
```

### Complete Workflow
```bash
# 1. Check driver status
.\build\Release\elrs_otg_demo.exe check-driver

# 2. Install driver if needed (run PowerShell as Administrator)
.\build\Release\elrs_otg_demo.exe install-driver

# 3. Connect ELRS device via USB-C cable

# 4. Initialize serial connection
.\build\Release\elrs_otg_demo.exe init-serial

# 5. Test functionality
.\build\Release\elrs_otg_demo.exe test-all
```

## Test Scripts Available

### PowerShell Scripts
- `test_driver.ps1` - Comprehensive driver testing
- `workflow.ps1` - Complete setup and workflow guide
- `build.ps1` - Build project with driver files
- `run.ps1` - Interactive mode
- `demo.ps1` - Full demo sequence

### Batch Scripts
- `test_driver.bat` - Driver testing for batch users

## Technical Implementation

### Driver Installer Class (`driver_installer.h/.cpp`)
- Windows API integration using setupapi.h and newdev.h
- Automatic system architecture detection
- Driver file verification and validation
- Admin privilege checking and elevation requests
- Comprehensive error reporting

### Build System Integration
- CMakeLists.txt updated to include driver_installer.cpp
- Automatic copying of platform files to build output
- Driver files embedded in executable distribution

### CLI Integration
- Updated handleCommandLine() function with driver commands
- Enhanced help system with driver management section
- Smart initialization control for standalone commands

## Files Added/Modified

### New Files
- `include/driver_installer.h` - Driver installer header
- `src/driver_installer.cpp` - Driver installer implementation
- `test_driver.ps1` - Driver testing script
- `test_driver.bat` - Driver testing batch script
- `workflow.ps1` - Complete workflow guide

### Modified Files
- `main.cpp` - Added driver commands and CLI integration
- `CMakeLists.txt` - Added driver installer source and platform file copying

## Driver Files Structure
```
platform/win/drv/
├── silabser.inf                 # Driver information file
├── silabser.cat                 # Driver catalog (signed)
├── x86/silabser.sys            # 32-bit driver
├── x64/silabser.sys            # 64-bit driver
├── arm/silabser.sys            # ARM driver
├── arm64/silabser.sys          # ARM64 driver
├── SLAB_License_Agreement_VCP_Windows.txt
└── CP210x_Universal_Windows_Driver_ReleaseNotes.txt
```

## Benefits

### For Users
- **No Manual Driver Installation**: Automated CP210x driver installation
- **Self-Contained**: All driver files included, no external downloads needed
- **Admin Handling**: Proper administrator privilege detection and elevation
- **Complete CLI**: Every function accessible via command line for scripting

### for Development
- **Automated Testing**: Complete CLI allows for automated test scripts
- **Standalone Operation**: No dependencies on external driver installation
- **Cross-Architecture**: Support for all Windows architectures
- **Professional Grade**: Windows API integration for reliable driver management

## Next Steps
1. Connect ELRS transmitter device via USB-C
2. Run as Administrator to install drivers
3. Test complete functionality with automated CLI commands
4. Use in production scripts for automated testing and control

The ELRS OTG Demo is now a complete, professional-grade solution for ELRS transmitter control with integrated driver management.