#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <newdev.h>
#endif

namespace ELRS
{
    /**
     * Windows Driver Installation Utility
     * Handles automated installation of CP210x USB-to-UART drivers
     */
    class DriverInstaller
    {
    public:
        struct DriverInfo
        {
            std::string name;
            std::string version;
            std::string date;
            std::string provider;
            bool installed;
        };

        struct UnknownDeviceInfo
        {
            std::string hardware_id;
            std::string device_desc;
            std::string location;
            bool is_potential_elrs;
            bool needs_driver;
        };

        DriverInstaller();
        ~DriverInstaller();

        // Driver detection and management
        bool isCP210xDriverInstalled();
        std::vector<DriverInfo> getInstalledDrivers();
        std::vector<UnknownDeviceInfo> scanForUnknownElrsDevices();

        // Driver installation
        bool installCP210xDriver();
        bool uninstallCP210xDriver();

        // Driver path management
        std::string getDriverPath();
        bool verifyDriverFiles();

        // System utilities
        bool isRunningAsAdmin();
        bool requestAdminPrivileges();

        std::string getLastError() const { return last_error_; }

    private:
        std::string last_error_;
        std::string driver_base_path_;

#ifdef _WIN32
        bool installDriverFromINF(const std::string &inf_path);
        bool uninstallDriverByHardwareId(const std::string &hardware_id);
        std::string getSystemArchitecture();
        std::vector<std::string> getCP210xHardwareIds();
#endif

        void setError(const std::string &error);
    };
}