# ELRS OTG Demo - C++ 2.4GHz Radio Controller

A comprehensive C++ console application for testing ExpressLRS (ELRS) 2.4GHz radio transmitter communication. **Implements both USB and serial communication methods following the practical guide approach**.

## Purpose

This application allows you to connect your ELRS transmitter directly to a PC and test all radio functionalities using **two connection methods**:

### Method 1: Serial COM Port (Recommended - Follows Practical Guide)
- **CP210x USB-to-UART** at **420kbaud** (8-N-1)
- Standard Windows COM port communication
- Matches BETAFPV SuperG practical guide exactly
- Requires CP210x driver installation

### Method 2: Direct USB (Advanced)
- LibUSB direct hardware access
- For advanced users and development

## Features

- **🚁 Real-time Transmission**: 250Hz CRSF channel data transmission
- **📡 ELRS Commands**: BIND, Device Discovery, Link Stats, Power Control, Model Selection
- **📊 Telemetry Monitoring**: Live RSSI, Link Quality, Battery data
- **🛡️ Safety Features**: ARM/DISARM with emergency stop
- **🎮 Control Testing**: Simulated stick movements and channel testing
- **🔍 Dual Detection**: Automatic USB device and COM port scanning
- **⚙️ Command Line**: Full CLI interface for automated testing
- **🔌 Serial Support**: CP210x COM port communication at 420kbaud

## Hardware Requirements

- **ELRS Transmitter**: SuperG, BETAFPV, or compatible ELRS TX module
- **USB Cable**: USB-C or Micro-USB (depending on your TX)
- **CP210x Driver**: Silicon Labs USB-to-UART Bridge driver
- **Windows PC**: With Visual Studio Build Tools

## Driver Installation (Required for Serial Method)

1. **Download CP210x Driver**: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
2. **Install Driver**: Run the installer
3. **Verify**: Check Device Manager for "Silicon Labs CP210x USB to UART Bridge"

## Quick Start

### Interactive Mode (Auto-Initialization)

**Step 1: Build the Application**
```powershell
.\build.ps1
```

**Step 2: Connect Hardware**
1. Connect ELRS transmitter via USB-C
2. Install CP210x driver if needed  
3. Verify COM port appears in Device Manager

**Step 3: Run Application (Auto-Initialize)**
```powershell
# Simply run the app - it will auto-initialize!
.\build\Release\elrs_otg_demo.exe
```

**The app will automatically:**
- 🔄 Initialize system
- 🔍 Find ELRS devices
- 🔌 Show found device: **[Device Name] - [Connection Info]**
- 🔗 Auto-connect (COM port @ 420kbaud preferred, USB fallback)
- 📋 Present streamlined menu for testing

**Step 4: Use Interactive Menu**
```
+================ MAIN MENU ================+
|  1. Start CRSF transmission (250Hz)        |
|  2. Stop CRSF transmission                 |
|  3. Test control inputs                    |
|  4. Test ELRS commands (bind, power, etc) |
|  5. Test safety features (arm/disarm)     |
|  6. Monitor telemetry                      |
|  7. Power control menu                     |
|  8. Show device status                     |
|  9. Rescan devices                         |
| 10. Reconnect device                       |
| 11. Disconnect device                      |
|  0. Exit                                   |
+============================================+
```

### Command-Line Interface (Perfect for Automation)

**Every interactive menu option has a corresponding command-line argument!**

```powershell
# Show all available commands
.\elrs_otg_demo.exe help

# Device connection (matches interactive flow)
.\elrs_otg_demo.exe scan                  # Scan for devices
.\elrs_otg_demo.exe init-serial           # Connect via COM port (420kbaud)
.\elrs_otg_demo.exe init                  # Connect via USB

# Menu option equivalents (no keyboard input required)
.\elrs_otg_demo.exe start-tx              # Start CRSF transmission [Menu: 1]
.\elrs_otg_demo.exe stop-tx               # Stop CRSF transmission [Menu: 2]
.\elrs_otg_demo.exe test-controls         # Test control inputs [Menu: 3]
.\elrs_otg_demo.exe test-elrs             # Test ELRS commands [Menu: 4]
.\elrs_otg_demo.exe test-safety           # Test safety features [Menu: 5]
.\elrs_otg_demo.exe monitor 30            # Monitor telemetry 30s [Menu: 6]
.\elrs_otg_demo.exe power-menu            # Power control options [Menu: 7]
.\elrs_otg_demo.exe status                # Show device status [Menu: 8]
.\elrs_otg_demo.exe rescan                # Rescan devices [Menu: 9]
.\elrs_otg_demo.exe reconnect             # Reconnect device [Menu: 10]
.\elrs_otg_demo.exe disconnect            # Disconnect device [Menu: 11]
.\elrs_otg_demo.exe exit                  # Exit application [Menu: 0]

# Power control with parameters
.\elrs_otg_demo.exe set-model 3           # Set model ID to 3
.\elrs_otg_demo.exe power-up              # Increase TX power
.\elrs_otg_demo.exe power-down            # Decrease TX power

# Test suites
.\elrs_otg_demo.exe test-all              # Run complete test suite (all menu options)
.\elrs_otg_demo.exe test-sequence         # Run basic test sequence

# No arguments = interactive menu
.\elrs_otg_demo.exe                       # Launch interactive mode
```

**🎯 Perfect for CI/CD, automated testing, and scripting!**

## Requirements

- **CMake**: Download from https://cmake.org/download/
- **Visual Studio Build Tools**: Download from https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
- **CP210x Driver**: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- **LibUSB** (optional): Download from https://libusb.info/ and place in `lib/` directory

## Connection Methods

### Serial COM Port Method (Recommended)
```powershell
# Following practical guide: 420kbaud, 8-N-1
.\elrs_otg_demo.exe init-serial
```
- ✅ Matches practical guide exactly
- ✅ Uses standard Windows COM port
- ✅ 420,000 baud rate (configurable to 416,666)
- ✅ Requires only CP210x driver

### Direct USB Method (Advanced)
```powershell  
# Advanced users - direct LibUSB access
.\elrs_otg_demo.exe init
```
- ⚡ Direct hardware access
- ⚡ Requires LibUSB library
- ⚡ For development and advanced testing

- `B`: BIND mode

- `F`: Device discovery## Building

- `L`: Request link statistics  

- `R`: Reset transmitter```powershell

- `Y`: Toggle debug mode# Build the project

- `ESC/Q`: Quit.\build.ps1



The application displays real-time connection status, channel data, and telemetry.# Run the application  

.\run.ps1

## License

# Install libusb

Educational and testing purposes. Based on ExpressLRS project documentation.vcpkg install libusb:x64-windows

# Build the project
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

## Usage

### Command Line Interface (Automated Testing)

Run with command line arguments for automated testing without user interaction:

```cmd
# Show all available commands
.\elrs_otg_demo.exe help

# Basic device operations
.\elrs_otg_demo.exe scan                    # Scan for ELRS devices
.\elrs_otg_demo.exe init                    # Initialize and connect
.\elrs_otg_demo.exe status                  # Show device status

# Control operations
.\elrs_otg_demo.exe start-tx                # Start CRSF transmission
.\elrs_otg_demo.exe test-channels           # Test control inputs for 10s
.\elrs_otg_demo.exe stop-tx                 # Stop transmission

# Configuration commands
.\elrs_otg_demo.exe model 3                 # Set model ID (1-8)
.\elrs_otg_demo.exe power-up                # Increase TX power
.\elrs_otg_demo.exe power-down              # Decrease TX power

# Monitoring and telemetry
.\elrs_otg_demo.exe telemetry               # Request telemetry once
.\elrs_otg_demo.exe monitor 30              # Monitor telemetry for 30s

# Safety commands
.\elrs_otg_demo.exe arm                     # Arm device (DANGEROUS!)
.\elrs_otg_demo.exe disarm                  # Disarm device
.\elrs_otg_demo.exe emergency               # Emergency stop

# Advanced operations
.\elrs_otg_demo.exe bind                    # Put device in bind mode
.\elrs_otg_demo.exe test-sequence           # Run full test sequence
```

**Automated Test Examples:**
```cmd
# Quick device test
.\elrs_otg_demo.exe scan && .\elrs_otg_demo.exe init && .\elrs_otg_demo.exe status

# Full functionality test
.\elrs_otg_demo.exe test-sequence

# Monitor for 1 minute
.\elrs_otg_demo.exe monitor 60
```

### Interactive Menu Mode

1. **Connect Hardware**:
   - Connect ELRS transmitter to PC via USB
   - Put transmitter in USB mode (if required)
   - Ensure drivers are installed

2. **Run Application** (no arguments for interactive menu):
   ```cmd
   .\elrs_otg_demo.exe
   ```

3. **Basic Operation**:
   - Application will show interactive menu
   - Choose options by entering numbers
   - Application auto-detects ELRS devices

## Controls

### Main Commands
- `T` - Start/Stop Transmission
- `R` - Start/Stop Telemetry
- `Y` - Toggle Debug Mode (shows TX/RX packets)
- `A` - Toggle ARM/DISARM
- `Q` - Quit Application

### ELRS Commands
- `B` - Send BIND command
- `D` - Device Discovery
- `S` - Link Stats Request
- `E` - Reset Device
- `P` - Perform Radio Test

### Stick Controls
- `WASD` - Control Roll/Pitch
- `+/-` - Throttle Up/Down
- `0` - Zero throttle
- `Space` - Center all sticks

## Debug Mode

When debug mode is enabled (Y key):
- All TX frames are logged as hex data
- All RX telemetry frames are displayed
- Transmission statistics are shown
- Useful for verifying actual packet transmission

## Expected Output

When working correctly, you should see:
```
✅ Connected to device: ExpressLRS TX Module
📡 Transmission STARTED (250Hz)
📊 Telemetry STARTED
📡 BAT:8.4V RSSI:-45dBm LQ:100% PWR:100mW (1250 frames)
```

## Troubleshooting

### Device Not Found
- Check USB cable connection
- Verify device drivers installed
- Try different USB ports
- Put transmitter in bootloader/USB mode

### No Telemetry Data
- Start both transmission (T) and telemetry (R)
- Check if transmitter is bound to a receiver
- Verify transmitter is powered on and in range

### Build Errors
- Ensure Visual Studio C++ workload is installed
- Check CMake version (3.10+ required)
- Verify vcpkg toolchain path is correct

## Technical Details

### Protocol Implementation
- **CRSF Protocol**: Complete implementation with CRC calculation
- **MSP Commands**: ELRS-specific MSP commands (BIND, Discovery, etc.)
- **USB Communication**: libusb-based USB serial communication
- **Multi-threading**: Separate TX/RX threads for real-time operation

### Architecture
- `CrsfProtocol`: CRSF frame building and parsing
- `UsbBridge`: USB device management and communication
- `Transmitter`: Main transmitter control logic
- `TelemetryHandler`: Telemetry data processing and display

### Performance
- **TX Rate**: 250Hz (4ms intervals)
- **RX Monitoring**: 100Hz polling
- **Latency**: <5ms USB communication
- **Reliability**: Error handling and retry logic

## Contributing

This is a testing tool for ELRS development. Feel free to:
- Add new ELRS command implementations
- Improve telemetry parsing
- Add support for additional USB devices
- Enhance the user interface

## Build Instructions (Windows)

### Prerequisites

1. **Install CMake**
   - Download from: https://cmake.org/download/
   - Add to system PATH during installation

2. **Install C++ Compiler**
   - **Option A (Recommended)**: Visual Studio Build Tools
     - Download: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
     - Install "C++ build tools" workload
   - **Option B**: MinGW-w64
     - Download from: https://www.mingw-w64.org/downloads/
     - Add to system PATH

3. **Install libusb-1.0**
   - Download from: https://libusb.info/
   - Extract `libusb-1.0.lib` and `libusb-1.0.dll` to `lib/` directory
   - Extract headers to `include/` directory (if needed)

### Building

1. **Setup dependencies** (one-time setup):
   ```powershell
   .\setup_dependencies.ps1
   ```
   Follow the manual steps to download and place libusb files.

2. **Build the project**:
   ```powershell
   .\build.ps1
   ```
   
   For debug build:
   ```powershell
   .\build.ps1 -BuildType Debug
   ```
   
   To clean and rebuild:
   ```powershell
   .\build.ps1 -Clean
   ```

3. **Run the application**:
   ```powershell
   .\run.ps1
   ```

### Manual Build (Alternative)

If you prefer manual building:

```powershell
# Create and enter build directory
mkdir build
cd build

# Configure project
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run
.\elrs_otg_demo.exe
```

## License

This project is for educational and testing purposes. ELRS protocol implementation based on ExpressLRS project documentation.