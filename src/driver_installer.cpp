#include "driver_installer.h"
#include <iostream>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <newdev.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <shellapi.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "newdev.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "cfgmgr32.lib")
#endif

namespace ELRS
{
    DriverInstaller::DriverInstaller()
    {
        // Set the driver base path relative to executable
        char exe_path[MAX_PATH];
        GetModuleFileNameA(NULL, exe_path, MAX_PATH);
        std::filesystem::path exe_dir = std::filesystem::path(exe_path).parent_path();
        driver_base_path_ = (exe_dir / "platform" / "win" / "drv").string();

        std::cout << "[DRIVER] Driver base path: " << driver_base_path_ << std::endl;
    }

    DriverInstaller::~DriverInstaller()
    {
    }

    bool DriverInstaller::isCP210xDriverInstalled()
    {
#ifdef _WIN32
        // Check if CP210x devices are present and have drivers
        HDEVINFO device_info_set = SetupDiGetClassDevsA(
            nullptr,
            "USB\\VID_10C4&PID_EA60", // CP210x VID/PID
            nullptr,
            DIGCF_PRESENT | DIGCF_ALLCLASSES);

        if (device_info_set == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        SP_DEVINFO_DATA device_info_data;
        device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

        bool driver_found = false;
        for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); i++)
        {
            // Check if device has a driver installed
            CHAR driver_key[256];
            if (SetupDiGetDeviceRegistryPropertyA(
                    device_info_set,
                    &device_info_data,
                    SPDRP_DRIVER,
                    nullptr,
                    (PBYTE)driver_key,
                    sizeof(driver_key),
                    nullptr))
            {
                driver_found = true;
                break;
            }
        }

        SetupDiDestroyDeviceInfoList(device_info_set);
        return driver_found;
#else
        return false;
#endif
    }

    std::vector<DriverInstaller::DriverInfo> DriverInstaller::getInstalledDrivers()
    {
        std::vector<DriverInfo> drivers;

#ifdef _WIN32
        // Query installed drivers for CP210x devices
        HDEVINFO device_info_set = SetupDiGetClassDevs(
            &GUID_DEVCLASS_PORTS,
            nullptr,
            nullptr,
            DIGCF_PRESENT);

        if (device_info_set != INVALID_HANDLE_VALUE)
        {
            SP_DEVINFO_DATA device_info_data;
            device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

            for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); i++)
            {
                CHAR description[256];
                if (SetupDiGetDeviceRegistryPropertyA(
                        device_info_set,
                        &device_info_data,
                        SPDRP_DEVICEDESC,
                        nullptr,
                        (PBYTE)description,
                        sizeof(description),
                        nullptr))
                {
                    std::string desc_str(description);

                    if (desc_str.find("CP210x") != std::string::npos ||
                        desc_str.find("Silicon Labs") != std::string::npos)
                    {
                        DriverInfo info;
                        info.name = desc_str;
                        info.installed = true;
                        info.version = "Unknown";
                        info.date = "Unknown";
                        info.provider = "Silicon Labs";
                        drivers.push_back(info);
                    }
                }
            }

            SetupDiDestroyDeviceInfoList(device_info_set);
        }
#endif

        return drivers;
    }

    bool DriverInstaller::installCP210xDriver()
    {
#ifdef _WIN32
        if (!isRunningAsAdmin())
        {
            setError("Administrator privileges required for driver installation");
            return false;
        }

        if (!verifyDriverFiles())
        {
            setError("Driver files not found or invalid");
            return false;
        }

        std::string inf_path = driver_base_path_ + "/silabser.inf";
        std::cout << "[DRIVER] Installing CP210x driver from: " << inf_path << std::endl;

        return installDriverFromINF(inf_path);
#else
        setError("Driver installation only supported on Windows");
        return false;
#endif
    }

    bool DriverInstaller::uninstallCP210xDriver()
    {
#ifdef _WIN32
        if (!isRunningAsAdmin())
        {
            setError("Administrator privileges required for driver uninstallation");
            return false;
        }

        std::cout << "[DRIVER] Uninstalling CP210x drivers..." << std::endl;

        // Uninstall by hardware IDs
        auto hardware_ids = getCP210xHardwareIds();
        bool success = true;

        for (const auto &hw_id : hardware_ids)
        {
            if (!uninstallDriverByHardwareId(hw_id))
            {
                success = false;
            }
        }

        return success;
#else
        setError("Driver uninstallation only supported on Windows");
        return false;
#endif
    }

    std::string DriverInstaller::getDriverPath()
    {
        return driver_base_path_;
    }

    bool DriverInstaller::verifyDriverFiles()
    {
        std::vector<std::string> required_files = {
            "silabser.inf",
            "silabser.cat",
            "SLAB_License_Agreement_VCP_Windows.txt"};

        std::string arch = getSystemArchitecture();
        required_files.push_back(arch + "/silabser.sys");

        for (const auto &file : required_files)
        {
            std::string full_path = driver_base_path_ + "/" + file;
            if (!std::filesystem::exists(full_path))
            {
                setError("Missing driver file: " + file);
                return false;
            }
        }

        std::cout << "[DRIVER] All required driver files verified for " << arch << std::endl;
        return true;
    }

    bool DriverInstaller::isRunningAsAdmin()
    {
#ifdef _WIN32
        BOOL is_admin = FALSE;
        PSID admin_group = nullptr;
        SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;

        if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &admin_group))
        {
            CheckTokenMembership(nullptr, admin_group, &is_admin);
            FreeSid(admin_group);
        }

        return is_admin == TRUE;
#else
        return false;
#endif
    }

    bool DriverInstaller::requestAdminPrivileges()
    {
#ifdef _WIN32
        if (isRunningAsAdmin())
        {
            return true;
        }

        std::cout << "[DRIVER] Requesting administrator privileges..." << std::endl;

        // Get current executable path
        char exe_path[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

        // Restart with admin privileges
        SHELLEXECUTEINFOA sei = {0};
        sei.cbSize = sizeof(SHELLEXECUTEINFOA);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.hwnd = nullptr;
        sei.lpVerb = "runas";
        sei.lpFile = exe_path;
        sei.lpParameters = "install-driver";
        sei.lpDirectory = nullptr;
        sei.nShow = SW_NORMAL;

        if (ShellExecuteExA(&sei))
        {
            std::cout << "[DRIVER] Restarting with administrator privileges..." << std::endl;
            return true;
        }
        else
        {
            setError("Failed to request administrator privileges");
            return false;
        }
#else
        return false;
#endif
    }

#ifdef _WIN32
    bool DriverInstaller::installDriverFromINF(const std::string &inf_path)
    {
        std::wstring inf_path_w(inf_path.begin(), inf_path.end());

        std::cout << "[DRIVER] Installing driver from INF..." << std::endl;

        BOOL result = DiInstallDriverW(
            nullptr,            // Parent window
            inf_path_w.c_str(), // INF file path
            DIIRFLAG_FORCE_INF, // Force installation
            nullptr             // Needs reboot flag
        );

        if (result)
        {
            std::cout << "[DRIVER] ✅ Driver installation successful" << std::endl;
            return true;
        }
        else
        {
            DWORD error = GetLastError();
            std::ostringstream oss;
            oss << "Driver installation failed. Error code: " << error;
            setError(oss.str());
            std::cout << "[DRIVER] ❌ " << last_error_ << std::endl;
            return false;
        }
    }

    bool DriverInstaller::uninstallDriverByHardwareId(const std::string &hardware_id)
    {
        std::wstring hw_id_w(hardware_id.begin(), hardware_id.end());

        // This is a simplified approach - full implementation would require
        // more complex device enumeration and uninstallation
        std::cout << "[DRIVER] Attempting to uninstall driver for: " << hardware_id << std::endl;

        // For now, return true as uninstallation is typically done through Device Manager
        return true;
    }

    std::string DriverInstaller::getSystemArchitecture()
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);

        switch (si.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return "x64";
        case PROCESSOR_ARCHITECTURE_INTEL:
            return "x86";
        case PROCESSOR_ARCHITECTURE_ARM:
            return "arm";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return "arm64";
        default:
            return "x64"; // Default to x64
        }
    }

    std::vector<std::string> DriverInstaller::getCP210xHardwareIds()
    {
        return {
            "USB\\VID_10C4&PID_EA60", // Most common CP210x
            "USB\\VID_10C4&PID_EA61",
            "USB\\VID_10C4&PID_EA70",
            "USB\\VID_10C4&PID_EA71"};
    }

    std::vector<DriverInstaller::UnknownDeviceInfo> DriverInstaller::scanForUnknownElrsDevices()
    {
        std::vector<UnknownDeviceInfo> unknown_devices;

#ifdef _WIN32
        std::cout << "[DRIVER] Scanning for unknown devices that might be ELRS transmitters..." << std::endl;

        // Scan all USB devices, including those without drivers
        HDEVINFO device_info_set = SetupDiGetClassDevsA(
            nullptr,
            "USB",
            nullptr,
            DIGCF_PRESENT | DIGCF_ALLCLASSES);

        if (device_info_set != INVALID_HANDLE_VALUE)
        {
            SP_DEVINFO_DATA device_info_data;
            device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

            for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); i++)
            {
                UnknownDeviceInfo device_info;
                device_info.is_potential_elrs = false;
                device_info.needs_driver = false;

                // Get hardware ID
                CHAR hardware_id[512];
                if (SetupDiGetDeviceRegistryPropertyA(
                        device_info_set,
                        &device_info_data,
                        SPDRP_HARDWAREID,
                        nullptr,
                        (PBYTE)hardware_id,
                        sizeof(hardware_id),
                        nullptr))
                {
                    device_info.hardware_id = std::string(hardware_id);
                }

                // Get device description
                CHAR description[256];
                if (SetupDiGetDeviceRegistryPropertyA(
                        device_info_set,
                        &device_info_data,
                        SPDRP_DEVICEDESC,
                        nullptr,
                        (PBYTE)description,
                        sizeof(description),
                        nullptr))
                {
                    device_info.device_desc = std::string(description);
                }
                else
                {
                    device_info.device_desc = "Unknown Device";
                }

                // Get location information
                CHAR location[256];
                if (SetupDiGetDeviceRegistryPropertyA(
                        device_info_set,
                        &device_info_data,
                        SPDRP_LOCATION_INFORMATION,
                        nullptr,
                        (PBYTE)location,
                        sizeof(location),
                        nullptr))
                {
                    device_info.location = std::string(location);
                }

                // Check if this could be a CP210x device (Silicon Labs VID)
                if (device_info.hardware_id.find("VID_10C4") != std::string::npos)
                {
                    device_info.is_potential_elrs = true;

                    // Check if it needs a driver (no driver installed or problem device)
                    CHAR driver_key[256];
                    if (!SetupDiGetDeviceRegistryPropertyA(
                            device_info_set,
                            &device_info_data,
                            SPDRP_DRIVER,
                            nullptr,
                            (PBYTE)driver_key,
                            sizeof(driver_key),
                            nullptr))
                    {
                        device_info.needs_driver = true;
                    }

                    // Check device status for problems
                    DWORD status = 0;
                    DWORD problem = 0;
                    if (CM_Get_DevNode_Status(&status, &problem, device_info_data.DevInst, 0) == CR_SUCCESS)
                    {
                        if (problem != 0)
                        {
                            device_info.needs_driver = true;
                        }
                    }

                    unknown_devices.push_back(device_info);
                }
                // Also check for generic unknown devices that might be ELRS
                else if (device_info.device_desc.find("Unknown") != std::string::npos ||
                         device_info.device_desc.find("Composite") != std::string::npos)
                {
                    // Check if it's a USB device without proper identification
                    if (device_info.hardware_id.find("USB\\") != std::string::npos)
                    {
                        device_info.is_potential_elrs = false; // Unknown, but could be
                        device_info.needs_driver = true;
                        unknown_devices.push_back(device_info);
                    }
                }
            }

            SetupDiDestroyDeviceInfoList(device_info_set);
        }

        std::cout << "[DRIVER] Found " << unknown_devices.size() << " unknown/problematic USB devices" << std::endl;
#endif

        return unknown_devices;
    }
#endif

    void DriverInstaller::setError(const std::string &error)
    {
        last_error_ = error;
    }
}