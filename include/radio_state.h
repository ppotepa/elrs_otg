#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>
#include <memory>

namespace ELRS
{

    /**
     * Connection status enumeration
     */
    enum class ConnectionStatus
    {
        Disconnected,
        Connecting,
        Connected,
        Error,
        Timeout
    };

    /**
     * Radio operation mode
     */
    enum class RadioMode
    {
        Normal,
        Binding,
        Testing,
        Updating,
        Configuration
    };

    /**
     * Live telemetry data structure
     */
    struct LiveTelemetry
    {
        // Signal quality metrics
        int rssi1 = -120;    // Primary RSSI (dBm)
        int rssi2 = -120;    // Secondary RSSI (dBm)
        int linkQuality = 0; // Link quality percentage (0-100)
        int snr = 0;         // Signal-to-noise ratio (dB)
        int txPower = 0;     // TX power (dBm)

        // Packet statistics
        uint32_t packetsReceived = 0;    // Total packets received
        uint32_t packetsTransmitted = 0; // Total packets transmitted
        uint32_t packetsLost = 0;        // Total packets lost
        uint32_t packetRate = 0;         // Packets per second

        // Power and battery
        double voltage = 0.0; // Battery voltage (V)
        double current = 0.0; // Current draw (A)
        int temperature = 0;  // Temperature (°C)

        // Timing
        std::chrono::steady_clock::time_point lastUpdate;
        bool isValid = false;

        LiveTelemetry()
        {
            lastUpdate = std::chrono::steady_clock::now();
        }
    };

    /**
     * Device configuration and info
     */
    struct DeviceConfiguration
    {
        std::string productName;
        std::string manufacturer;
        std::string serialNumber;
        std::string firmwareVersion;
        std::string hardwareVersion;
        uint16_t vid = 0;
        uint16_t pid = 0;

        // ELRS specific settings
        std::string frequency = "2.4 GHz";
        std::string protocol = "ExpressLRS";
        int baudRate = 420000;
        bool isVerified = false;
    };

    /**
     * State change callback type
     */
    using StateChangeCallback = std::function<void()>;

    /**
     * RadioState - Centralized state management for ELRS radio
     * Singleton pattern providing React-like state management
     */
    class RadioState
    {
    public:
        // Singleton access
        static RadioState &getInstance();

        // Delete copy constructor and assignment operator
        RadioState(const RadioState &) = delete;
        RadioState &operator=(const RadioState &) = delete;

        // Connection management
        void setConnectionStatus(ConnectionStatus status);
        ConnectionStatus getConnectionStatus() const;
        std::string getConnectionStatusString() const;

        // Radio mode management
        void setRadioMode(RadioMode mode);
        RadioMode getRadioMode() const;
        std::string getRadioModeString() const;

        // Device configuration
        void setDeviceConfiguration(const DeviceConfiguration &config);
        DeviceConfiguration getDeviceConfiguration() const;

        // Live telemetry updates
        void updateTelemetry(const LiveTelemetry &telemetry);
        void updateRSSI(int rssi1, int rssi2 = -120);
        void updateLinkQuality(int quality);
        void updateTxPower(int power);
        void updatePacketStats(uint32_t rx, uint32_t tx, uint32_t lost = 0);
        void updateBattery(double voltage, double current);
        void updateTemperature(int temp);

        // Telemetry getters
        LiveTelemetry getLiveTelemetry() const;
        int getRSSI() const;
        int getLinkQuality() const;
        int getTxPower() const;
        uint32_t getPacketsReceived() const;
        uint32_t getPacketsTransmitted() const;
        double getBatteryVoltage() const;

        // Calculated metrics
        double getPacketLossRate() const;
        std::string getUptimeString() const;
        std::string getLastUpdateTimeString() const;
        bool isTelemetryFresh(int maxAgeMs = 5000) const;

        // Error and status management
        void setLastError(const std::string &error);
        std::string getLastError() const;
        void clearError();
        bool hasError() const;

        // State change notifications (React-like)
        void subscribeToChanges(StateChangeCallback callback);
        void unsubscribeFromChanges();

        // Statistics and history
        void resetStatistics();
        std::vector<int> getRSSIHistory(int maxPoints = 100) const;
        std::vector<int> getLinkQualityHistory(int maxPoints = 100) const;
        std::vector<int> getTxPowerHistory(int maxPoints = 100) const;

        // Spectrum analysis data
        void updateSpectrumData(const std::vector<int> &data);
        std::vector<int> getSpectrumData() const;
        bool isSpectrumFresh(int maxAgeMs = 1000) const;
        size_t getSpectrumBinCount() const;
        std::chrono::steady_clock::time_point getSpectrumLastUpdate() const;

        // System state
        void markSystemReady();
        bool isSystemReady() const;
        std::chrono::steady_clock::time_point getStartTime() const;

    private:
        RadioState();
        ~RadioState() = default;

        // Thread safety
        mutable std::mutex state_mutex_;

        // State data
        std::atomic<ConnectionStatus> connection_status_{ConnectionStatus::Disconnected};
        std::atomic<RadioMode> radio_mode_{RadioMode::Normal};
        DeviceConfiguration device_config_;
        LiveTelemetry live_telemetry_;
        std::string last_error_;
        std::atomic<bool> has_error_{false};
        std::atomic<bool> system_ready_{false};

        // History tracking for graphs
        std::vector<int> rssi_history_;
        std::vector<int> link_quality_history_;
        std::vector<int> tx_power_history_;
        static constexpr size_t MAX_HISTORY_SIZE = 200;
        std::vector<int> spectrum_data_;
        std::chrono::steady_clock::time_point spectrum_last_update_;
        static constexpr size_t MAX_SPECTRUM_SIZE = 256;

        // Timing
        std::chrono::steady_clock::time_point start_time_;

        // State change callback
        StateChangeCallback state_change_callback_;

        // Helper methods
        void notifyStateChange();
        void addToHistory(std::vector<int> &history, int value);
        std::string formatDuration(std::chrono::steady_clock::duration duration) const;
    };

} // namespace ELRS