#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace ELRS
{
    namespace Devices
    {

        /**
         * Device manufacturer identifiers
         */
        enum class Manufacturer
        {
            BetaFPV,
            Happymodel,
            SIYI,
            Matek,
            Radiomaster,
            Generic_STM32,
            Generic_ESP32,
            Unknown
        };

        /**
         * Device driver types
         */
        enum class DriverType
        {
            CP210x,    // Silicon Labs CP210x USB to UART
            FTDI,      // FTDI USB to Serial
            CH340,     // WCH CH340 USB to Serial
            STM32_VCP, // STM32 Virtual COM Port
            ESP32_CDC, // ESP32 CDC/ACM
            WinUSB,    // Generic WinUSB driver
            Native     // Built-in Windows driver
        };

        /**
         * Device information structure
         */
        struct DeviceInfo
        {
            uint16_t vid;
            uint16_t pid;
            Manufacturer manufacturer;
            std::string model;
            std::string description;
            DriverType driverType;
            std::string driverPath;                      // Relative path to driver files
            std::vector<std::string> supportedPlatforms; // "win", "linux", "mac"
            bool isVerified;                             // Has this device been tested?
            std::string notes;                           // Additional notes
        };

        /**
         * Device registry - central mapping of VID/PID to device information
         */
        class DeviceRegistry
        {
        public:
            static DeviceRegistry &getInstance();

            // Device lookup
            const DeviceInfo *findDevice(uint16_t vid, uint16_t pid) const;
            std::vector<DeviceInfo> getAllDevices() const;
            std::vector<DeviceInfo> getDevicesByManufacturer(Manufacturer manufacturer) const;

            // Device management
            void registerDevice(const DeviceInfo &device);
            bool isSupported(uint16_t vid, uint16_t pid) const;

            // Driver management
            std::string getDriverPath(uint16_t vid, uint16_t pid, const std::string &platform = "win") const;
            DriverType getDriverType(uint16_t vid, uint16_t pid) const;

            // Utility functions
            static std::string manufacturerToString(Manufacturer manufacturer);
            static std::string driverTypeToString(DriverType driverType);

        private:
            DeviceRegistry();
            void initializeDevices();

            std::map<std::pair<uint16_t, uint16_t>, DeviceInfo> devices_;
        };

        /**
         * Device-specific driver installer
         */
        class DeviceDriverInstaller
        {
        public:
            static bool installDriver(const DeviceInfo &device, const std::string &platform = "win");
            static bool isDriverInstalled(const DeviceInfo &device, const std::string &platform = "win");
            static std::string getDriverInstallationPath(const DeviceInfo &device, const std::string &platform = "win");

        private:
            static bool installWindowsDriver(const DeviceInfo &device);
            static bool installLinuxDriver(const DeviceInfo &device);
            static std::string getInfFileName(DriverType driverType);
        };

    } // namespace Devices
} // namespace ELRS