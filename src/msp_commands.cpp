#include "msp_commands.h"
#include "usb_bridge.h"
#include <iostream>

namespace ELRS
{

    MspCommands::MspCommands(UsbBridge *usb_bridge)
        : usb_bridge_(usb_bridge)
    {
    }

    bool MspCommands::sendBindCommand()
    {
        // From elrsv3.lua: crossfireTelemetryPush(0x2D, { deviceId, handsetId, fieldId, status })
        // deviceId = 0xEE (TX module), handsetId = 0xEF (ELRS Lua), fieldId = 0x00 (bind), status = 1 (execute)
        uint8_t payload[] = {
            ELRS_DEVICE_TX,  // Device ID (TX module)
            ELRS_HANDSET_ID, // Handset ID (ELRS Lua)
            0x00,            // Field ID (0 for bind command)
            0x01             // Status (1 = execute bind)
        };

        std::cout << "🔗 ELRS_BIND: Sending bind command to TX module..." << std::endl;
        bool result = sendMspCommand(MSP_ELRS_TELEMETRY_PUSH, payload, sizeof(payload));

        if (result)
        {
            std::cout << "✅ BIND_COMMAND: Successfully sent to transmitter" << std::endl;
            std::cout << "📡 BIND_STATUS: TX should enter binding mode" << std::endl;
        }
        else
        {
            std::cout << "❌ BIND_FAILED: " << getLastError() << std::endl;
        }

        return result;
    }

    bool MspCommands::sendDeviceDiscovery()
    {
        // From elrsv3.lua: crossfireTelemetryPush(0x28, { 0x00, 0xEA })
        uint8_t payload[] = {0x00, 0xEA};

        std::cout << "🔍 ELRS_DISCOVERY: Scanning for ELRS devices..." << std::endl;
        bool result = sendMspCommand(MSP_DEVICE_DISCOVERY, payload, sizeof(payload));

        if (result)
        {
            std::cout << "✅ DISCOVERY_SENT: Device scan command transmitted" << std::endl;
        }
        else
        {
            std::cout << "❌ DISCOVERY_FAILED: " << getLastError() << std::endl;
        }

        return result;
    }

    bool MspCommands::sendLinkStatsRequest()
    {
        // Request link statistics from TX module
        uint8_t payload[] = {
            ELRS_DEVICE_TX,  // Device ID (TX module)
            ELRS_HANDSET_ID, // Handset ID (ELRS Lua)
            0x00,            // Field ID (0 for link stats)
            0x00             // Status (0 for request)
        };

        std::cout << "📊 ELRS_LINKSTATS: Requesting telemetry data..." << std::endl;
        bool result = sendMspCommand(MSP_ELRS_TELEMETRY_PUSH, payload, sizeof(payload));

        if (result)
        {
            std::cout << "✅ LINKSTATS_REQUEST: Telemetry request sent" << std::endl;
        }
        else
        {
            std::cout << "❌ LINKSTATS_FAILED: " << getLastError() << std::endl;
        }

        return result;
    }

    bool MspCommands::sendPowerIncrease()
    {
        uint8_t payload[] = {0x01}; // Increase power flag

        std::cout << "📶 POWER_INCREASE: Boosting TX power level..." << std::endl;
        bool result = sendMspCommand(MSP_POWER_CONTROL, payload, sizeof(payload));

        if (result)
        {
            std::cout << "✅ POWER_UP: Command sent - TX power should increase" << std::endl;
        }
        else
        {
            std::cout << "❌ POWER_UP_FAILED: " << getLastError() << std::endl;
        }

        return result;
    }

    bool MspCommands::sendPowerDecrease()
    {
        uint8_t payload[] = {0x00}; // Decrease power flag

        std::cout << "📉 POWER_DECREASE: Reducing TX power level..." << std::endl;
        bool result = sendMspCommand(MSP_POWER_CONTROL, payload, sizeof(payload));

        if (result)
        {
            std::cout << "✅ POWER_DOWN: Command sent - TX power should decrease" << std::endl;
        }
        else
        {
            std::cout << "❌ POWER_DOWN_FAILED: " << getLastError() << std::endl;
        }

        return result;
    }

    bool MspCommands::sendModelSelect(uint8_t model_id)
    {
        uint8_t payload[] = {model_id};

        std::cout << "🔀 MODEL_SELECT: Switching to model " << static_cast<int>(model_id) << "..." << std::endl;
        bool result = sendMspCommand(MSP_MODEL_SELECT, payload, sizeof(payload));

        if (result)
        {
            std::cout << "✅ MODEL_SWITCH: Command sent - Should switch to model " << static_cast<int>(model_id) << std::endl;
        }
        else
        {
            std::cout << "❌ MODEL_SWITCH_FAILED: " << getLastError() << std::endl;
        }

        return result;
    }

    bool MspCommands::sendMspCommand(uint8_t function, const uint8_t *payload, uint8_t payload_size)
    {
        if (!usb_bridge_ || !usb_bridge_->isConnected())
        {
            setError("USB device not connected");
            return false;
        }

        std::array<uint8_t, 64> frame;
        uint8_t frame_size;

        buildMspCommand(function, payload, payload_size, frame, frame_size);

        return usb_bridge_->write(frame.data(), frame_size);
    }

    void MspCommands::buildMspCommand(uint8_t function, const uint8_t *payload, uint8_t payload_size,
                                      std::array<uint8_t, 64> &out, uint8_t &out_size)
    {
        // MSP v2 frame format: $M< or $M> [size] [function] [payload...] [crc]
        out[0] = '$';
        out[1] = 'M';
        out[2] = '<'; // Direction (< = to FC)
        out[3] = payload_size;
        out[4] = function;

        // Copy payload
        if (payload && payload_size > 0)
        {
            for (uint8_t i = 0; i < payload_size; i++)
            {
                out[5 + i] = payload[i];
            }
        }

        // Calculate CRC (size + function + payload)
        uint8_t crc = calculateMspCrc(&out[3], payload_size + 1);
        out[5 + payload_size] = crc;

        out_size = 6 + payload_size;
    }

    uint8_t MspCommands::calculateMspCrc(const uint8_t *data, uint8_t length)
    {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < length; i++)
        {
            crc ^= data[i];
        }
        return crc;
    }

    void MspCommands::setError(const std::string &error)
    {
        last_error_ = error;
    }

} // namespace ELRS
