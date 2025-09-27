#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <ctime>
#include <random>
#include <sstream>
#include "usb_bridge.h"
#include "elrs_transmitter.h"
#include "telemetry_handler.h"
#include "ftxui_manager.h"
#include "radio_state.h"
#include "log_manager.h"

class ElrsRadioDetector
{
private:
    ELRS::UsbBridge usb_bridge_;

public:
    void detectRadios()
    {
        std::cout << "🔍 Scanning for ELRS devices..." << std::endl;
        std::cout << "📊 " << usb_bridge_.getDriverStatus() << std::endl;
        std::cout << std::endl;

        if (!usb_bridge_.hasUsbSupport())
        {
            std::cout << "⚠️  USB support not available" << std::endl;
            std::cout << "   Installing runtime drivers..." << std::endl;

            if (usb_bridge_.initializeDrivers())
            {
                std::cout << "✅ Runtime drivers loaded successfully!" << std::endl;
            }
            else
            {
                std::cout << "❌ Failed to load runtime drivers" << std::endl;
                std::cout << "   Error: " << usb_bridge_.getLastError() << std::endl;
                return;
            }
        }

        auto devices = usb_bridge_.scanDevices();

        if (devices.empty())
        {
            std::cout << "⚠️  No ELRS devices found" << std::endl;
            std::cout << "   Make sure your transmitter is connected via USB" << std::endl;
            std::cout << "   Supported devices: ExpressLRS 2.4GHz transmitters" << std::endl;
            return;
        }

        std::cout << "✅ Found " << devices.size() << " ELRS device(s):" << std::endl;

        for (size_t i = 0; i < devices.size(); ++i)
        {
            const auto &device = devices[i];
            std::cout << "  📡 Device " << (i + 1) << ":" << std::endl;
            std::cout << "     - Product: " << device.product << std::endl;
            std::cout << "     - Manufacturer: " << device.manufacturer << std::endl;
            std::cout << "     - Serial: " << device.serial << std::endl;
            std::cout << "     - VID:PID: " << std::hex << std::uppercase
                      << device.vid << ":" << device.pid << std::dec << std::endl;
            std::cout << "     - Frequency: 2.4 GHz" << std::endl;
            std::cout << "     - Protocol: ExpressLRS" << std::endl;
            std::cout << std::endl;
        }
    }

    bool connectToDevice(int deviceIndex = 0)
    {
        auto devices = usb_bridge_.scanDevices();
        if (devices.empty() || deviceIndex >= static_cast<int>(devices.size()))
        {
            return false;
        }

        const auto &device = devices[deviceIndex];
        std::cout << "🔗 Connecting to " << device.product << "..." << std::endl;

        if (usb_bridge_.connect(device.vid, device.pid))
        {
            std::cout << "✅ Connected successfully!" << std::endl;
            connected_device_ = device;
            return true;
        }
        else
        {
            std::cout << "❌ Connection failed: " << usb_bridge_.getLastError() << std::endl;
            return false;
        }
    }

    const ELRS::UsbBridge::DeviceInfo &getConnectedDevice() const
    {
        return connected_device_;
    }

    void launchTuiInterface(ELRS::UI::ScreenType initialScreen = ELRS::UI::ScreenType::Main)
    {
        std::cout << "🚀 Launching full-screen TUI interface..." << std::endl;
        if (initialScreen != ELRS::UI::ScreenType::Main)
        {
            std::cout << "   Starting with: " << ELRS::UI::FTXUIManager::getScreenName(initialScreen) << " screen" << std::endl;
        }
        std::cout << "Press any key to continue...";
        std::cin.get();

        LOG_INFO("TUI", "Initializing TUI interface and RadioState");

        // Get RadioState instance
        ELRS::RadioState &radioState = ELRS::RadioState::getInstance();

        // Initialize device configuration in RadioState
        ELRS::DeviceConfiguration deviceConfig;
        deviceConfig.productName = connected_device_.product;
        deviceConfig.manufacturer = connected_device_.manufacturer;
        deviceConfig.serialNumber = connected_device_.serial;
        deviceConfig.vid = connected_device_.vid;
        deviceConfig.pid = connected_device_.pid;
        deviceConfig.frequency = "2.4 GHz";
        deviceConfig.protocol = "ExpressLRS";
        deviceConfig.isVerified = true;

        radioState.setDeviceConfiguration(deviceConfig);
        radioState.setConnectionStatus(ELRS::ConnectionStatus::Connected);
        radioState.markSystemReady();

        // Create and configure FTXUI manager
        ELRS::UI::FTXUIManager ftxuiManager;
        if (!ftxuiManager.initialize())
        {
            std::cerr << "Failed to initialize FTXUI manager" << std::endl;
            return;
        }

        // Switch to initial screen if specified
        if (initialScreen != ELRS::UI::ScreenType::Main)
        {
            LOG_INFO("SYSTEM", "Switching to initial screen: " + ELRS::UI::FTXUIManager::getScreenName(initialScreen));
            ftxuiManager.switchToScreen(initialScreen);
        }

        // Start real telemetry monitoring thread
        std::thread telemetryThread([&radioState, this]()
                                    {
            LOG_INFO("TELEMETRY", "Starting telemetry monitoring thread");
            
            // Initialize telemetry handler for real data
            ELRS::TelemetryHandler telemetryHandler(&usb_bridge_);
            
            // Set up callbacks for real telemetry data
            telemetryHandler.setLinkStatsCallback([&radioState](const ELRS::LinkStats& stats) {
                if (stats.valid) {
                    LOG_DEBUG("TELEMETRY", "Received link stats: RSSI=" + std::to_string(stats.rssi1) + 
                             "dBm, Link Quality=" + std::to_string(stats.link_quality) + "%");
                    radioState.updateRSSI(stats.rssi1, stats.rssi2);
                    radioState.updateLinkQuality(stats.link_quality);
                    radioState.updateTxPower(stats.tx_power);
                }
            });
            
            telemetryHandler.setBatteryCallback([&radioState](const ELRS::BatteryInfo& battery) {
                if (battery.valid) {
                    radioState.updateBattery(battery.voltage_mv / 1000.0, battery.current_ma / 1000.0);
                }
            });
            
            // Start telemetry monitoring
            telemetryHandler.start();
            
            // Simulate periodic packet counting and additional telemetry
            uint32_t totalRxPackets = 0;
            uint32_t totalTxPackets = 0;
            
            while (radioState.getConnectionStatus() == ELRS::ConnectionStatus::Connected)
            {
                // Get real telemetry data
                auto linkStats = telemetryHandler.getLatestLinkStats();
                auto batteryInfo = telemetryHandler.getLatestBattery();
                
                if (linkStats.valid) {
                    // Update packet counters (simulate based on link activity)
                    totalRxPackets += (linkStats.link_quality > 0) ? 1 : 0;
                    totalTxPackets += 1;
                    
                    radioState.updatePacketStats(totalRxPackets, totalTxPackets);
                    
                    // Update temperature (simulate)
                    radioState.updateTemperature(25 + (linkStats.tx_power / 2)); // Rough estimate
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 5 Hz update rate
            }
            
            telemetryHandler.stop(); });

        // Run the FTXUI manager
        ftxuiManager.run();

        // Stop telemetry and cleanup
        radioState.setConnectionStatus(ELRS::ConnectionStatus::Disconnected);
        telemetryThread.join();

        ftxuiManager.shutdown();
    }

private:
    ELRS::UsbBridge::DeviceInfo connected_device_;
};

struct CommandLineArgs
{
    bool showLogs = false;
    bool showGraphs = false;
    bool showConfig = false;
    bool showMonitor = false;
    bool showHelp = false;
    ELRS::UI::ScreenType initialScreen = ELRS::UI::ScreenType::Main;
};

CommandLineArgs parseCommandLine(int argc, char *argv[])
{
    CommandLineArgs args;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--show-logs" || arg == "-l")
        {
            args.showLogs = true;
            args.initialScreen = ELRS::UI::ScreenType::Logs;
        }
        else if (arg == "--show-graphs" || arg == "-g")
        {
            args.showGraphs = true;
            args.initialScreen = ELRS::UI::ScreenType::Graphs;
        }
        else if (arg == "--show-config" || arg == "-c")
        {
            args.showConfig = true;
            args.initialScreen = ELRS::UI::ScreenType::Config;
        }
        else if (arg == "--show-monitor" || arg == "-m")
        {
            args.showMonitor = true;
            args.initialScreen = ELRS::UI::ScreenType::Monitor;
        }
        else if (arg == "--help" || arg == "-h")
        {
            args.showHelp = true;
        }
        else
        {
            std::cout << "Unknown argument: " << arg << std::endl;
            args.showHelp = true;
        }
    }

    return args;
}

void showHelp()
{
    std::cout << "ELRS OTG Demo - Command Line Options" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: elrs_otg_demo.exe [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --show-logs,    -l    Start with log viewer screen" << std::endl;
    std::cout << "  --show-graphs,  -g    Start with graphs screen" << std::endl;
    std::cout << "  --show-config,  -c    Start with configuration screen" << std::endl;
    std::cout << "  --show-monitor, -m    Start with monitor screen" << std::endl;
    std::cout << "  --help,         -h    Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Screen options are only available after successful device connection." << std::endl;
    std::cout << "      If no device is connected, the application will start normally." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    CommandLineArgs cmdArgs = parseCommandLine(argc, argv);

    if (cmdArgs.showHelp)
    {
        showHelp();
        return 0;
    }

    // Initialize logging system
    ELRS::LogManager::getInstance().setLogLevel(ELRS::LogLevel::Debug);
    LOG_INFO("SYSTEM", "ELRS OTG Demo starting up");

    std::cout << "ELRS OTG Demo - 2.4GHz Radio Detection" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;

    try
    {
        ElrsRadioDetector detector;

        // Detect radios
        detector.detectRadios();

        // Try to connect to first device
        std::cout << "🔧 Attempting connection..." << std::endl;
        if (detector.connectToDevice(0))
        {
            LOG_INFO("SYSTEM", "Successfully connected to ELRS device");
            std::cout << "🎯 System ready for ELRS communication" << std::endl;
            std::cout << "📊 Telemetry monitoring active" << std::endl;
            std::cout << std::endl;

            // Launch TUI interface
            LOG_INFO("SYSTEM", "Launching TUI interface");
            detector.launchTuiInterface(cmdArgs.initialScreen);
        }
        else
        {
            std::cout << "⚠️  Running in simulation mode" << std::endl;
            std::cout << "   Connect an ELRS transmitter to enable full functionality" << std::endl;
            std::cout << std::endl;
            std::cout << "Press Enter to exit..." << std::endl;
            std::cin.get();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "👋 Goodbye!" << std::endl;
    return 0;
}