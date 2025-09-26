#pragma once

#include <vector>
#include <string>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

// Forward declarations for libusb types (avoid compile-time dependency)
struct libusb_context;
struct libusb_device_handle;

namespace ELRS
{
    /**
     * Dynamic USB driver loader and manager
     */
    class UsbDriverLoader
    {
    public:
        static UsbDriverLoader &getInstance();
        bool loadDrivers();
        bool isLoaded() const { return drivers_loaded_; }
        std::string getLoadStatus() const { return load_status_; }

    private:
        UsbDriverLoader() = default;
        bool drivers_loaded_ = false;
        std::string load_status_ = "Not loaded";

#ifdef _WIN32
        HMODULE libusb_dll_ = nullptr;
#endif

        bool loadLibusb();
        bool loadWindowsDrivers();
    };

    /**
     * USB Bridge for communicating with ELRS transmitter with runtime driver loading
     */
    class UsbBridge
    {
    public:
        struct DeviceInfo
        {
            uint16_t vid;
            uint16_t pid;
            std::string manufacturer;
            std::string product;
            std::string serial;
            std::string description;
        };

        UsbBridge();
        ~UsbBridge();

        // Runtime driver management
        bool initializeDrivers();
        bool hasUsbSupport() const { return usb_support_available_; }
        std::string getDriverStatus() const;

        // Device discovery and connection
        std::vector<DeviceInfo> findElrsDevices();
        std::vector<DeviceInfo> scanDevices();                      // Alias for findElrsDevices
        bool connect(uint16_t vid = 0x0483, uint16_t pid = 0x5740); // Default STM32 VID/PID
        void disconnect();
        bool isConnected() const { return device_handle_ != nullptr; }

        // Data transmission
        bool write(const uint8_t *data, size_t length, int timeout_ms = 1000);
        int read(uint8_t *buffer, size_t buffer_size, int timeout_ms = 50);

        // Device info
        DeviceInfo getConnectedDeviceInfo() const;
        std::string getLastError() const { return last_error_; }

    private:
        libusb_context *context_;
        libusb_device_handle *device_handle_;
        DeviceInfo connected_device_;
        std::string last_error_;
        bool usb_support_available_;

        // USB endpoints (common for ELRS devices)
        static constexpr uint8_t ENDPOINT_OUT = 0x01;
        static constexpr uint8_t ENDPOINT_IN = 0x81;

        void setError(const std::string &error);
        std::string getUsbErrorString(int error_code);

        // Runtime loading helpers
        bool tryLoadLibusb();
        bool detectUsbDevices();
        bool scanRealUsbDevices(std::vector<DeviceInfo> &devices);
        bool shouldShowSimulatedDevices() const;
    };

} // namespace ELRS