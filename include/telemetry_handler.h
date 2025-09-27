#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <vector>

namespace ELRS
{

    class UsbBridge; // Forward declaration

    /**
     * Telemetry data structures
     */
    struct LinkStats
    {
        int rssi1 = 0;
        int rssi2 = 0;
        int link_quality = 0;
        int snr = 0;
        int tx_power = 0;
        bool valid = false;
    };

    struct BatteryInfo
    {
        int voltage_mv = 0;   // mV
        int current_ma = 0;   // mA
        int capacity_mah = 0; // mAh
        bool valid = false;
    };

    /**
     * Telemetry Handler for ELRS
     * Processes incoming telemetry data from the transmitter
     */
    class TelemetryHandler
    {
    public:
        // Callback function types
        using LinkStatsCallback = std::function<void(const LinkStats &)>;
        using BatteryCallback = std::function<void(const BatteryInfo &)>;
        using SpectrumCallback = std::function<void(const std::vector<int> &)>;

        TelemetryHandler(UsbBridge *usb_bridge);
        ~TelemetryHandler();

        // Start/stop telemetry monitoring
        void start();
        void stop();
        bool isRunning() const { return running_.load(); }

        // Register callbacks
        void setLinkStatsCallback(LinkStatsCallback callback) { link_stats_callback_ = callback; }
        void setBatteryCallback(BatteryCallback callback) { battery_callback_ = callback; }
        void setSpectrumCallback(SpectrumCallback callback) { spectrum_callback_ = callback; }

        // Get latest telemetry data
        LinkStats getLatestLinkStats() const { return latest_link_stats_; }
        BatteryInfo getLatestBattery() const { return latest_battery_; }
        std::vector<int> getLatestSpectrum() const { return latest_spectrum_; }

        std::string getLastError() const { return last_error_; }

    private:
        enum class MspParserState
        {
            Idle,
            ExpectM,
            ExpectDirection,
            ExpectLength,
            ExpectFunction,
            ReadPayload,
            ExpectChecksum
        };

        UsbBridge *usb_bridge_;
        std::atomic<bool> running_{false};
        std::unique_ptr<std::thread> telemetry_thread_;

        // Callbacks
        LinkStatsCallback link_stats_callback_;
        BatteryCallback battery_callback_;
        SpectrumCallback spectrum_callback_;

        // Latest data
        LinkStats latest_link_stats_;
        BatteryInfo latest_battery_;
        std::vector<int> latest_spectrum_;

        std::string last_error_;

        // Telemetry processing
        void telemetryLoop();
        void feedMspByte(uint8_t byte);
        void resetMspParser();
        void handleMspFrame(uint8_t function, bool fromDevice, const std::vector<uint8_t> &payload);
        bool parseLinkStats(const uint8_t *data, int length, LinkStats &stats);
        bool parseBatteryInfo(const uint8_t *data, int length, BatteryInfo &battery);

        void setError(const std::string &error);

        // MSP parser state
        MspParserState msp_state_ = MspParserState::Idle;
        bool msp_from_device_ = false;
        uint8_t msp_expected_length_ = 0;
        uint8_t msp_function_ = 0;
        uint8_t msp_checksum_ = 0;
        std::vector<uint8_t> msp_payload_;
    };

} // namespace ELRS
