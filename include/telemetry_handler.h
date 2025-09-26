#pragma once

#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <memory>

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

        TelemetryHandler(UsbBridge *usb_bridge);
        ~TelemetryHandler();

        // Start/stop telemetry monitoring
        void start();
        void stop();
        bool isRunning() const { return running_.load(); }

        // Register callbacks
        void setLinkStatsCallback(LinkStatsCallback callback) { link_stats_callback_ = callback; }
        void setBatteryCallback(BatteryCallback callback) { battery_callback_ = callback; }

        // Get latest telemetry data
        LinkStats getLatestLinkStats() const { return latest_link_stats_; }
        BatteryInfo getLatestBattery() const { return latest_battery_; }

        std::string getLastError() const { return last_error_; }

    private:
        UsbBridge *usb_bridge_;
        std::atomic<bool> running_{false};
        std::unique_ptr<std::thread> telemetry_thread_;

        // Callbacks
        LinkStatsCallback link_stats_callback_;
        BatteryCallback battery_callback_;

        // Latest data
        LinkStats latest_link_stats_;
        BatteryInfo latest_battery_;

        std::string last_error_;

        // Telemetry processing
        void telemetryLoop();
        void processTelemetryFrame(const uint8_t *frame, int length);
        bool parseLinkStats(const uint8_t *data, int length, LinkStats &stats);
        bool parseBatteryInfo(const uint8_t *data, int length, BatteryInfo &battery);

        void setError(const std::string &error);
    };

} // namespace ELRS
