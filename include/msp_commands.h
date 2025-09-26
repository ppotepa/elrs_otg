#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace ELRS
{

    class UsbBridge; // Forward declaration

    /**
     * MSP (MultiWii Serial Protocol) Commands for ELRS
     * Based on the ELRS Lua script implementation from SuperGOTG
     */
    class MspCommands
    {
    public:
        // MSP command codes
        static constexpr uint8_t MSP_ELRS_TELEMETRY_PUSH = 0x2D;
        static constexpr uint8_t MSP_POWER_CONTROL = 0xF5;
        static constexpr uint8_t MSP_MODEL_SELECT = 0xF6;
        static constexpr uint8_t MSP_DEVICE_DISCOVERY = 0x28;

        // ELRS device IDs (from elrsv3.lua)
        static constexpr uint8_t ELRS_DEVICE_TX = 0xEE;
        static constexpr uint8_t ELRS_HANDSET_ID = 0xEF;

        MspCommands(UsbBridge *usb_bridge);

        // ELRS specific commands
        bool sendBindCommand();
        bool sendDeviceDiscovery();
        bool sendLinkStatsRequest();
        bool sendPowerIncrease();
        bool sendPowerDecrease();
        bool sendModelSelect(uint8_t model_id = 1);

        // Generic MSP command sender
        bool sendMspCommand(uint8_t function, const uint8_t *payload = nullptr, uint8_t payload_size = 0);

        std::string getLastError() const { return last_error_; }

    private:
        UsbBridge *usb_bridge_;
        std::string last_error_;

        void setError(const std::string &error);

        // MSP frame building
        void buildMspCommand(uint8_t function, const uint8_t *payload, uint8_t payload_size,
                             std::array<uint8_t, 64> &out, uint8_t &out_size);
        uint8_t calculateMspCrc(const uint8_t *data, uint8_t length);
    };

} // namespace ELRS
