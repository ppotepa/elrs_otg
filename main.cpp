#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <ctime>
#include "usb_bridge.h"
#include "elrs_transmitter.h"
#include "telemetry_handler.h"

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
            return true;
        }
        else
        {
            std::cout << "❌ Connection failed: " << usb_bridge_.getLastError() << std::endl;
            return false;
        }
    }
};

int main()
{
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
            std::cout << "🎯 System ready for ELRS communication" << std::endl;
            std::cout << "📊 Telemetry monitoring active" << std::endl;
        }
        else
        {
            std::cout << "⚠️  Running in simulation mode" << std::endl;
            std::cout << "   Connect an ELRS transmitter to enable full functionality" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "👋 Goodbye!" << std::endl;
    return 0;
}