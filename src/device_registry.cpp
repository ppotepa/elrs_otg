#include "device_registry.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <newdev.h>
#pragma comment(lib, "newdev.lib")
#endif

namespace ELRS {
namespace Devices {

    DeviceRegistry& DeviceRegistry::getInstance() {
        static DeviceRegistry instance;
        return instance;
    }

    DeviceRegistry::DeviceRegistry() {
        initializeDevices();
    }

    void DeviceRegistry::initializeDevices() {
        // BetaFPV Devices
        registerDevice({
            0x10C4, 0xEA60,
            Manufacturer::BetaFPV,
            "SuperD 2.4G",
            "BetaFPV SuperD 2.4G ExpressLRS Module",
            DriverType::CP210x,
            "src/devices/betafpv/platform",
            {"win", "linux", "mac"},
            true,
            "Most common BetaFPV ELRS module"
        });

        registerDevice({
            0x10C4, 0xEA70,
            Manufacturer::BetaFPV,
            "Lite 2.4G",
            "BetaFPV Lite 2.4G ExpressLRS Module", 
            DriverType::CP210x,
            "src/devices/betafpv/platform",
            {"win", "linux", "mac"},
            true,
            "Compact BetaFPV ELRS module"
        });

        // Happymodel Devices
        registerDevice({
            0x0483, 0x5740,
            Manufacturer::Happymodel,
            "ES24TX",
            "Happymodel ES24TX 2.4G ExpressLRS Module",
            DriverType::STM32_VCP,
            "src/devices/happymodel/platform",
            {"win", "linux", "mac"},
            true,
            "Popular Happymodel TX module"
        });

        registerDevice({
            0x1209, 0x5741,
            Manufacturer::Happymodel,
            "EP2 2.4G",
            "Happymodel EP2 2.4G ExpressLRS Module",
            DriverType::STM32_VCP,
            "src/devices/happymodel/platform",
            {"win", "linux", "mac"},
            true,
            "High-power Happymodel module"
        });

        // SIYI Devices
        registerDevice({
            0x0403, 0x6001,
            Manufacturer::SIYI,
            "FM30 2.4G",
            "SIYI FM30 2.4G ExpressLRS Module",
            DriverType::FTDI,
            "src/devices/siyi/platform",
            {"win", "linux", "mac"},
            false,
            "SIYI gimbal integration module"
        });

        // Matek Devices
        registerDevice({
            0x0483, 0x5742,
            Manufacturer::Matek,
            "R24-S",
            "Matek R24-S 2.4G ExpressLRS Receiver",
            DriverType::STM32_VCP,
            "src/devices/matek/platform",
            {"win", "linux"},
            false,
            "Matek receiver with UART"
        });

        // Radiomaster Devices
        registerDevice({
            0x2E8A, 0x000A,
            Manufacturer::Radiomaster,
            "Ranger",
            "Radiomaster Ranger 2.4G ExpressLRS Module",
            DriverType::ESP32_CDC,
            "src/devices/radiomaster/platform",
            {"win", "linux", "mac"},
            true,
            "High-range Radiomaster module"
        });

        registerDevice({
            0x303A, 0x1001,
            Manufacturer::Radiomaster,
            "Zorro ELRS",
            "Radiomaster Zorro Internal ELRS Module",
            DriverType::ESP32_CDC,
            "src/devices/radiomaster/platform",
            {"win", "linux", "mac"},
            true,
            "Built-in ELRS in Zorro radio"
        });

        // Generic STM32 devices
        registerDevice({
            0x0483, 0x5740,
            Manufacturer::Generic_STM32,
            "STM32 VCP",
            "Generic STM32 Virtual COM Port",
            DriverType::STM32_VCP,
            "src/devices/generic/stm32/platform",
            {"win", "linux", "mac"},
            true,
            "Generic STM32 with VCP support"
        });

        // Generic ESP32 devices
        registerDevice({
            0x10C4, 0xEA60,
            Manufacturer::Generic_ESP32,
            "ESP32 CP210x",
            "Generic ESP32 with CP210x USB-Serial",
            DriverType::CP210x,
            "src/devices/generic/esp32/platform",
            {"win", "linux", "mac"},
            true,
            "Generic ESP32 development board"
        });

        std::cout << "[DeviceRegistry] Initialized " << devices_.size() << " device definitions" << std::endl;
    }

    const DeviceInfo* DeviceRegistry::findDevice(uint16_t vid, uint16_t pid) const {
        auto key = std::make_pair(vid, pid);
        auto it = devices_.find(key);
        return (it != devices_.end()) ? &it->second : nullptr;
    }

    std::vector<DeviceInfo> DeviceRegistry::getAllDevices() const {
        std::vector<DeviceInfo> result;
        for (const auto& pair : devices_) {
            result.push_back(pair.second);
        }
        return result;
    }

    std::vector<DeviceInfo> DeviceRegistry::getDevicesByManufacturer(Manufacturer manufacturer) const {
        std::vector<DeviceInfo> result;
        for (const auto& pair : devices_) {
            if (pair.second.manufacturer == manufacturer) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    void DeviceRegistry::registerDevice(const DeviceInfo& device) {
        auto key = std::make_pair(device.vid, device.pid);
        devices_[key] = device;
    }

    bool DeviceRegistry::isSupported(uint16_t vid, uint16_t pid) const {
        return findDevice(vid, pid) != nullptr;
    }

    std::string DeviceRegistry::getDriverPath(uint16_t vid, uint16_t pid, const std::string& platform) const {
        const DeviceInfo* device = findDevice(vid, pid);
        if (!device) return "";
        
        // Check if platform is supported
        if (std::find(device->supportedPlatforms.begin(), device->supportedPlatforms.end(), platform) 
            == device->supportedPlatforms.end()) {
            return "";
        }
        
        return device->driverPath + "/" + platform;
    }

    DriverType DeviceRegistry::getDriverType(uint16_t vid, uint16_t pid) const {
        const DeviceInfo* device = findDevice(vid, pid);
        return device ? device->driverType : DriverType::WinUSB;
    }

    std::string DeviceRegistry::manufacturerToString(Manufacturer manufacturer) {
        switch (manufacturer) {
            case Manufacturer::BetaFPV: return "BetaFPV";
            case Manufacturer::Happymodel: return "Happymodel";
            case Manufacturer::SIYI: return "SIYI";
            case Manufacturer::Matek: return "Matek";
            case Manufacturer::Radiomaster: return "Radiomaster";
            case Manufacturer::Generic_STM32: return "Generic STM32";
            case Manufacturer::Generic_ESP32: return "Generic ESP32";
            default: return "Unknown";
        }
    }

    std::string DeviceRegistry::driverTypeToString(DriverType driverType) {
        switch (driverType) {
            case DriverType::CP210x: return "CP210x";
            case DriverType::FTDI: return "FTDI";
            case DriverType::CH340: return "CH340";
            case DriverType::STM32_VCP: return "STM32 VCP";
            case DriverType::ESP32_CDC: return "ESP32 CDC";
            case DriverType::WinUSB: return "WinUSB";
            case DriverType::Native: return "Native";
            default: return "Unknown";
        }
    }

    // DeviceDriverInstaller implementation
    bool DeviceDriverInstaller::installDriver(const DeviceInfo& device, const std::string& platform) {
        std::cout << "[DriverInstaller] Installing driver for " << device.description 
                  << " on " << platform << std::endl;
        
        if (platform == "win") {
            return installWindowsDriver(device);
        } else if (platform == "linux") {
            return installLinuxDriver(device);
        }
        
        std::cout << "[DriverInstaller] Platform " << platform << " not supported yet" << std::endl;
        return false;
    }

    bool DeviceDriverInstaller::isDriverInstalled(const DeviceInfo& device, const std::string& platform) {
        // Check if driver is already installed
        // This would check Windows device manager, Linux /dev entries, etc.
        return true; // Simplified for now
    }

    std::string DeviceDriverInstaller::getDriverInstallationPath(const DeviceInfo& device, const std::string& platform) {
        return device.driverPath + "/" + platform + "/drv";
    }

    bool DeviceDriverInstaller::installWindowsDriver(const DeviceInfo& device) {
#ifdef _WIN32
        std::string driverPath = getDriverInstallationPath(device, "win");
        std::string infPath = driverPath + "/" + DeviceDriverInstaller::getInfFileName(device.driverType);
        
        std::cout << "[DriverInstaller] Installing Windows driver from: " << infPath << std::endl;
        
        // In a real implementation, this would use Windows APIs to install the driver
        // For now, just report success if the files exist
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(infPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose(hFind);
            std::cout << "[DriverInstaller] Driver files found, installation would proceed" << std::endl;
            return true;
        }
        
        std::cout << "[DriverInstaller] Driver files not found at: " << infPath << std::endl;
        return false;
#else
        return false;
#endif
    }

    bool DeviceDriverInstaller::installLinuxDriver(const DeviceInfo& device) {
        std::cout << "[DriverInstaller] Linux driver installation not implemented" << std::endl;
        return false;
    }

    // Helper function to get INF file name based on driver type
    std::string DeviceDriverInstaller::getInfFileName(DriverType driverType) {
        switch (driverType) {
            case DriverType::CP210x: return "silabser.inf";
            case DriverType::FTDI: return "ftdibus.inf";
            case DriverType::CH340: return "ch341ser.inf";
            default: return "device.inf";
        }
    }

} // namespace Devices
} // namespace ELRS