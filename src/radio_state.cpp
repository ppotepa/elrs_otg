#include "radio_state.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ELRS
{

    RadioState &RadioState::getInstance()
    {
        static RadioState instance;
        return instance;
    }

    RadioState::RadioState()
    {
        start_time_ = std::chrono::steady_clock::now();
        live_telemetry_.lastUpdate = start_time_;

        // Initialize history vectors
        rssi_history_.reserve(MAX_HISTORY_SIZE);
        link_quality_history_.reserve(MAX_HISTORY_SIZE);
        tx_power_history_.reserve(MAX_HISTORY_SIZE);
        spectrum_last_update_ = start_time_;
    }

    // Connection management
    void RadioState::setConnectionStatus(ConnectionStatus status)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        connection_status_ = status;
        notifyStateChange();
    }

    ConnectionStatus RadioState::getConnectionStatus() const
    {
        return connection_status_.load();
    }

    std::string RadioState::getConnectionStatusString() const
    {
        switch (getConnectionStatus())
        {
        case ConnectionStatus::Disconnected:
            return "Disconnected";
        case ConnectionStatus::Connecting:
            return "Connecting...";
        case ConnectionStatus::Connected:
            return "Connected";
        case ConnectionStatus::Error:
            return "Error";
        case ConnectionStatus::Timeout:
            return "Timeout";
        default:
            return "Unknown";
        }
    }

    // Radio mode management
    void RadioState::setRadioMode(RadioMode mode)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        radio_mode_ = mode;
        notifyStateChange();
    }

    RadioMode RadioState::getRadioMode() const
    {
        return radio_mode_.load();
    }

    std::string RadioState::getRadioModeString() const
    {
        switch (getRadioMode())
        {
        case RadioMode::Normal:
            return "Normal";
        case RadioMode::Binding:
            return "Binding";
        case RadioMode::Testing:
            return "Testing";
        case RadioMode::Updating:
            return "Updating";
        case RadioMode::Configuration:
            return "Configuration";
        default:
            return "Unknown";
        }
    }

    // Device configuration
    void RadioState::setDeviceConfiguration(const DeviceConfiguration &config)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        device_config_ = config;
        notifyStateChange();
    }

    DeviceConfiguration RadioState::getDeviceConfiguration() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return device_config_;
    }

    // Live telemetry updates
    void RadioState::updateTelemetry(const LiveTelemetry &telemetry)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_ = telemetry;
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;

        // Update history
        addToHistory(rssi_history_, telemetry.rssi1);
        addToHistory(link_quality_history_, telemetry.linkQuality);
        addToHistory(tx_power_history_, telemetry.txPower);

        notifyStateChange();
    }

    void RadioState::updateRSSI(int rssi1, int rssi2)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.rssi1 = rssi1;
        live_telemetry_.rssi2 = rssi2;
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;

        addToHistory(rssi_history_, rssi1);
        notifyStateChange();
    }

    void RadioState::updateLinkQuality(int quality)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.linkQuality = std::max(0, std::min(100, quality));
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;

        addToHistory(link_quality_history_, live_telemetry_.linkQuality);
        notifyStateChange();
    }

    void RadioState::updateTxPower(int power)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.txPower = power;
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;

        addToHistory(tx_power_history_, power);
        notifyStateChange();
    }

    void RadioState::updatePacketStats(uint32_t rx, uint32_t tx, uint32_t lost)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.packetsReceived = rx;
        live_telemetry_.packetsTransmitted = tx;
        live_telemetry_.packetsLost = lost;
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;
        notifyStateChange();
    }

    void RadioState::updateBattery(double voltage, double current)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.voltage = voltage;
        live_telemetry_.current = current;
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;
        notifyStateChange();
    }

    void RadioState::updateTemperature(int temp)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.temperature = temp;
        live_telemetry_.lastUpdate = std::chrono::steady_clock::now();
        live_telemetry_.isValid = true;
        notifyStateChange();
    }

    // Telemetry getters
    LiveTelemetry RadioState::getLiveTelemetry() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_;
    }

    int RadioState::getRSSI() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_.rssi1;
    }

    int RadioState::getLinkQuality() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_.linkQuality;
    }

    int RadioState::getTxPower() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_.txPower;
    }

    uint32_t RadioState::getPacketsReceived() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_.packetsReceived;
    }

    uint32_t RadioState::getPacketsTransmitted() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_.packetsTransmitted;
    }

    double RadioState::getBatteryVoltage() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return live_telemetry_.voltage;
    }

    // Calculated metrics
    double RadioState::getPacketLossRate() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        uint32_t total = live_telemetry_.packetsReceived + live_telemetry_.packetsLost;
        if (total == 0)
            return 0.0;
        return (static_cast<double>(live_telemetry_.packetsLost) / total) * 100.0;
    }

    std::string RadioState::getUptimeString() const
    {
        auto now = std::chrono::steady_clock::now();
        auto uptime = now - start_time_;
        return formatDuration(uptime);
    }

    std::string RadioState::getLastUpdateTimeString() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    }

    bool RadioState::isTelemetryFresh(int maxAgeMs) const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - live_telemetry_.lastUpdate);
        return age.count() < maxAgeMs;
    }

    // Error and status management
    void RadioState::setLastError(const std::string &error)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        last_error_ = error;
        has_error_ = !error.empty();
        notifyStateChange();
    }

    std::string RadioState::getLastError() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return last_error_;
    }

    void RadioState::clearError()
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        last_error_.clear();
        has_error_ = false;
        notifyStateChange();
    }

    bool RadioState::hasError() const
    {
        return has_error_.load();
    }

    // State change notifications
    void RadioState::subscribeToChanges(StateChangeCallback callback)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_change_callback_ = callback;
    }

    void RadioState::unsubscribeFromChanges()
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_change_callback_ = nullptr;
    }

    // Statistics and history
    void RadioState::resetStatistics()
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        live_telemetry_.packetsReceived = 0;
        live_telemetry_.packetsTransmitted = 0;
        live_telemetry_.packetsLost = 0;

        rssi_history_.clear();
        link_quality_history_.clear();
        tx_power_history_.clear();

        start_time_ = std::chrono::steady_clock::now();
        notifyStateChange();
    }

    std::vector<int> RadioState::getRSSIHistory(int maxPoints) const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (rssi_history_.size() <= static_cast<size_t>(maxPoints))
        {
            return rssi_history_;
        }

        auto start = rssi_history_.end() - maxPoints;
        return std::vector<int>(start, rssi_history_.end());
    }

    std::vector<int> RadioState::getLinkQualityHistory(int maxPoints) const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (link_quality_history_.size() <= static_cast<size_t>(maxPoints))
        {
            return link_quality_history_;
        }

        auto start = link_quality_history_.end() - maxPoints;
        return std::vector<int>(start, link_quality_history_.end());
    }

    std::vector<int> RadioState::getTxPowerHistory(int maxPoints) const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (tx_power_history_.size() <= static_cast<size_t>(maxPoints))
        {
            return tx_power_history_;
        }

        auto start = tx_power_history_.end() - maxPoints;
        return std::vector<int>(start, tx_power_history_.end());
    }

    void RadioState::updateSpectrumData(const std::vector<int> &data)
    {
        if (data.empty())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(state_mutex_);
        spectrum_data_ = data;
        if (spectrum_data_.size() > MAX_SPECTRUM_SIZE)
        {
            spectrum_data_.erase(spectrum_data_.begin(), spectrum_data_.end() - MAX_SPECTRUM_SIZE);
        }
        spectrum_last_update_ = std::chrono::steady_clock::now();
        notifyStateChange();
    }

    std::vector<int> RadioState::getSpectrumData() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return spectrum_data_;
    }

    bool RadioState::isSpectrumFresh(int maxAgeMs) const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (spectrum_data_.empty())
        {
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - spectrum_last_update_);
        return age.count() <= maxAgeMs;
    }

    size_t RadioState::getSpectrumBinCount() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return spectrum_data_.size();
    }

    std::chrono::steady_clock::time_point RadioState::getSpectrumLastUpdate() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return spectrum_last_update_;
    }

    // System state
    void RadioState::markSystemReady()
    {
        system_ready_ = true;
        notifyStateChange();
    }

    bool RadioState::isSystemReady() const
    {
        return system_ready_.load();
    }

    std::chrono::steady_clock::time_point RadioState::getStartTime() const
    {
        return start_time_;
    }

    // Helper methods
    void RadioState::notifyStateChange()
    {
        if (state_change_callback_)
        {
            state_change_callback_();
        }
    }

    void RadioState::addToHistory(std::vector<int> &history, int value)
    {
        history.push_back(value);
        if (history.size() > MAX_HISTORY_SIZE)
        {
            history.erase(history.begin());
        }
    }

    std::string RadioState::formatDuration(std::chrono::steady_clock::duration duration) const
    {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours.count() << ":"
            << std::setfill('0') << std::setw(2) << minutes.count() << ":"
            << std::setfill('0') << std::setw(2) << seconds.count();
        return oss.str();
    }

} // namespace ELRS