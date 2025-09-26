#pragma once

#include <string>
#include <vector>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ELRS
{
    /**
     * Serial Bridge for communicating with ELRS transmitter via COM port
     * Implements the approach described in the practical guide:
     * - Uses CP210x USB-to-UART serial port
     * - 420,000 baud (or 416,666 exact) as specified in guide
     * - 8-N-1 configuration
     */
    class SerialBridge
    {
    public:
        struct ComPortInfo
        {
            std::string port;        // e.g., "COM3"
            std::string description; // e.g., "Silicon Labs CP210x USB to UART Bridge"
            std::string hardware_id; // e.g., "USB\VID_10C4&PID_EA60"
        };

        SerialBridge();
        ~SerialBridge();

        // COM port discovery and connection (matches guide approach)
        std::vector<ComPortInfo> scanComPorts();
        std::vector<ComPortInfo> findElrsComPorts();                   // Filter for CP210x devices
        bool connect(const std::string &port, int baud_rate = 420000); // 420kbaud as per guide
        void disconnect();
        bool isConnected() const;

        // Serial communication (8-N-1 as per guide)
        bool write(const uint8_t *data, size_t length, int timeout_ms = 1000);
        int read(uint8_t *buffer, size_t buffer_size, int timeout_ms = 50);

        // Status and error handling
        std::string getLastError() const { return last_error_; }
        ComPortInfo getConnectedPortInfo() const { return connected_port_; }

    private:
#ifdef _WIN32
        HANDLE serial_handle_;
        bool configureSerialPort(int baud_rate);
        std::vector<ComPortInfo> enumerateWindowsComPorts();
#endif

        ComPortInfo connected_port_;
        std::string last_error_;
        bool connected_;

        void setError(const std::string &error);
    };
}