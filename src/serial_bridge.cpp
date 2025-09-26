#define NOMINMAX // Prevent Windows max/min macro conflicts
#include "serial_bridge.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>

#pragma comment(lib, "setupapi.lib")
#endif

namespace ELRS
{

    SerialBridge::SerialBridge()
        : connected_(false)
#ifdef _WIN32
          ,
          serial_handle_(INVALID_HANDLE_VALUE)
#endif
    {
    }

    SerialBridge::~SerialBridge()
    {
        disconnect();
    }

    std::vector<SerialBridge::ComPortInfo> SerialBridge::scanComPorts()
    {
#ifdef _WIN32
        return enumerateWindowsComPorts();
#else
        // Linux/Mac implementation would go here
        return {};
#endif
    }

    std::vector<SerialBridge::ComPortInfo> SerialBridge::findElrsComPorts()
    {
        auto all_ports = scanComPorts();
        std::vector<ComPortInfo> elrs_ports;

        for (const auto &port : all_ports)
        {
            // Look for CP210x devices (as mentioned in the guide)
            if (port.description.find("CP210") != std::string::npos ||
                port.description.find("Silicon Labs") != std::string::npos ||
                port.hardware_id.find("VID_10C4&PID_EA60") != std::string::npos ||
                port.hardware_id.find("VID_0483&PID_5740") != std::string::npos) // STM32 VID/PID
            {
                elrs_ports.push_back(port);
            }
        }

        std::cout << "[SERIAL] Found " << elrs_ports.size() << " potential ELRS COM ports" << std::endl;
        for (const auto &port : elrs_ports)
        {
            std::cout << "   " << port.port << " - " << port.description << std::endl;
        }

        return elrs_ports;
    }

    bool SerialBridge::connect(const std::string &port, int baud_rate)
    {
        if (connected_)
        {
            disconnect();
        }

#ifdef _WIN32
        std::cout << "[SERIAL] Connecting to " << port << " at " << baud_rate << " baud (8-N-1)" << std::endl;

        // Open COM port
        serial_handle_ = CreateFileA(
            port.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,             // No sharing
            NULL,          // No security attributes
            OPEN_EXISTING, // Open existing device
            0,             // Not overlapped I/O
            NULL           // No template
        );

        if (serial_handle_ == INVALID_HANDLE_VALUE)
        {
            setError("Failed to open COM port " + port + ": " + std::to_string(GetLastError()));
            return false;
        }

        // Configure serial port
        if (!configureSerialPort(baud_rate))
        {
            CloseHandle(serial_handle_);
            serial_handle_ = INVALID_HANDLE_VALUE;
            return false;
        }

        connected_ = true;
        connected_port_.port = port;

        std::cout << "[SERIAL] Successfully connected to " << port << " at " << baud_rate << " baud" << std::endl;
        std::cout << "[SERIAL] Ready for CRSF communication as per practical guide" << std::endl;

        return true;
#else
        setError("Serial port support not implemented for this platform");
        return false;
#endif
    }

    void SerialBridge::disconnect()
    {
        if (!connected_)
            return;

#ifdef _WIN32
        if (serial_handle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(serial_handle_);
            serial_handle_ = INVALID_HANDLE_VALUE;
        }
#endif

        connected_ = false;
        std::cout << "[SERIAL] Disconnected from " << connected_port_.port << std::endl;
    }

    bool SerialBridge::isConnected() const
    {
        return connected_;
    }

    bool SerialBridge::write(const uint8_t *data, size_t length, int timeout_ms)
    {
        if (!connected_)
        {
            setError("Not connected to any COM port");
            return false;
        }

#ifdef _WIN32
        DWORD bytes_written = 0;
        BOOL result = WriteFile(serial_handle_, data, static_cast<DWORD>(length), &bytes_written, NULL);

        if (!result || bytes_written != length)
        {
            setError("Serial write failed: " + std::to_string(GetLastError()));
            return false;
        }

        return true;
#else
        setError("Serial write not implemented for this platform");
        return false;
#endif
    }

    int SerialBridge::read(uint8_t *buffer, size_t buffer_size, int timeout_ms)
    {
        if (!connected_)
        {
            setError("Not connected to any COM port");
            return -1;
        }

#ifdef _WIN32
        // Set read timeout
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = timeout_ms;
        timeouts.ReadTotalTimeoutConstant = timeout_ms;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        SetCommTimeouts(serial_handle_, &timeouts);

        DWORD bytes_read = 0;
        BOOL result = ReadFile(serial_handle_, buffer, static_cast<DWORD>(buffer_size), &bytes_read, NULL);

        if (!result)
        {
            DWORD error = GetLastError();
            if (error != ERROR_TIMEOUT)
            {
                setError("Serial read failed: " + std::to_string(error));
                return -1;
            }
        }

        return static_cast<int>(bytes_read);
#else
        setError("Serial read not implemented for this platform");
        return -1;
#endif
    }

#ifdef _WIN32
    bool SerialBridge::configureSerialPort(int baud_rate)
    {
        DCB dcb = {0};
        dcb.DCBlength = sizeof(DCB);

        if (!GetCommState(serial_handle_, &dcb))
        {
            setError("Failed to get COM state: " + std::to_string(GetLastError()));
            return false;
        }

        // Configure as per guide: 8-N-1 at specified baud rate
        dcb.BaudRate = baud_rate;  // 420000 or 416666 as per guide
        dcb.ByteSize = 8;          // 8 data bits
        dcb.Parity = NOPARITY;     // No parity
        dcb.StopBits = ONESTOPBIT; // 1 stop bit
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fDsrSensitivity = FALSE;
        dcb.fTXContinueOnXoff = FALSE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fErrorChar = FALSE;
        dcb.fNull = FALSE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fAbortOnError = FALSE;

        if (!SetCommState(serial_handle_, &dcb))
        {
            setError("Failed to configure COM port: " + std::to_string(GetLastError()));
            return false;
        }

        // Set buffer sizes
        SetupComm(serial_handle_, 4096, 4096);

        // Purge any existing data
        PurgeComm(serial_handle_, PURGE_TXCLEAR | PURGE_RXCLEAR);

        return true;
    }

    std::vector<SerialBridge::ComPortInfo> SerialBridge::enumerateWindowsComPorts()
    {
        std::vector<ComPortInfo> ports;

        // Use SetupAPI to enumerate COM ports with device information
        HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
        if (hDevInfo == INVALID_HANDLE_VALUE)
        {
            return ports;
        }

        SP_DEVINFO_DATA DeviceInfoData;
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
        {
            ComPortInfo port_info;
            char buffer[256];
            DWORD buffer_size;

            // Get device description
            buffer_size = sizeof(buffer);
            if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
                                                  NULL, (PBYTE)buffer, buffer_size, &buffer_size))
            {
                port_info.description = std::string(buffer);
            }

            // Get hardware ID
            buffer_size = sizeof(buffer);
            if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID,
                                                  NULL, (PBYTE)buffer, buffer_size, &buffer_size))
            {
                port_info.hardware_id = std::string(buffer);
            }

            // Get COM port name from registry
            HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, DICS_FLAG_GLOBAL,
                                             0, DIREG_DEV, KEY_READ);
            if (hKey != INVALID_HANDLE_VALUE)
            {
                buffer_size = sizeof(buffer);
                if (RegQueryValueExA(hKey, "PortName", NULL, NULL, (LPBYTE)buffer, &buffer_size) == ERROR_SUCCESS)
                {
                    port_info.port = std::string(buffer);
                    ports.push_back(port_info);
                }
                RegCloseKey(hKey);
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
        return ports;
    }
#endif

    void SerialBridge::setError(const std::string &error)
    {
        last_error_ = error;
        std::cerr << "[SERIAL_ERROR] " << error << std::endl;
    }

}