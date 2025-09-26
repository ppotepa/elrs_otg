#include "telemetry_handler.h"
#include "usb_bridge.h"
#include <iostream>
#include <chrono>
#include <cstring>

namespace ELRS
{

    TelemetryHandler::TelemetryHandler(UsbBridge *usb_bridge)
        : usb_bridge_(usb_bridge)
    {
    }

    TelemetryHandler::~TelemetryHandler()
    {
        stop();
    }

    void TelemetryHandler::start()
    {
        if (running_.load())
        {
            return; // Already running
        }

        if (!usb_bridge_ || !usb_bridge_->isConnected())
        {
            setError("USB device not connected");
            return;
        }

        running_.store(true);
        telemetry_thread_ = std::make_unique<std::thread>(&TelemetryHandler::telemetryLoop, this);

        std::cout << "📡 TELEMETRY: Started monitoring at 50Hz" << std::endl;
    }

    void TelemetryHandler::stop()
    {
        if (!running_.load())
        {
            return; // Already stopped
        }

        running_.store(false);

        if (telemetry_thread_ && telemetry_thread_->joinable())
        {
            telemetry_thread_->join();
        }
        telemetry_thread_.reset();

        std::cout << "📡 TELEMETRY: Stopped monitoring" << std::endl;
    }

    void TelemetryHandler::telemetryLoop()
    {
        uint8_t buffer[256];

        std::cout << "📡 TELEMETRY_LOOP: Active - reading from USB at 50Hz" << std::endl;

        while (running_.load())
        {
            if (!usb_bridge_->isConnected())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Read with short timeout to keep loop responsive
            int bytes_read = usb_bridge_->read(buffer, sizeof(buffer), 20);

            if (bytes_read > 0)
            {
                processTelemetryFrame(buffer, bytes_read);
            }

            // 50Hz telemetry loop (20ms interval)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        std::cout << "📡 TELEMETRY_LOOP: Exited" << std::endl;
    }

    void TelemetryHandler::processTelemetryFrame(const uint8_t *frame, int length)
    {
        if (length < 4)
        {
            return; // Too short to be valid
        }

        // Simple telemetry frame parsing
        // In a real implementation, this would parse CRSF telemetry frames

        // Check for MSP response frame: $M> [size] [function] [payload...] [crc]
        if (length >= 5 && frame[0] == '$' && frame[1] == 'M' && frame[2] == '>')
        {
            uint8_t payload_size = frame[3];
            uint8_t function = frame[4];

            if (length >= 6 + payload_size)
            {
                const uint8_t *payload = &frame[5];

                // Parse different telemetry types
                LinkStats link_stats;
                BatteryInfo battery_info;

                switch (function)
                {
                case 0x2D: // ELRS telemetry response
                    if (parseLinkStats(payload, payload_size, link_stats))
                    {
                        latest_link_stats_ = link_stats;
                        if (link_stats_callback_)
                        {
                            link_stats_callback_(link_stats);
                        }
                    }
                    break;

                case 0x2E: // Battery telemetry
                    if (parseBatteryInfo(payload, payload_size, battery_info))
                    {
                        latest_battery_ = battery_info;
                        if (battery_callback_)
                        {
                            battery_callback_(battery_info);
                        }
                    }
                    break;

                default:
                    // Unknown telemetry function
                    break;
                }
            }
        }
    }

    bool TelemetryHandler::parseLinkStats(const uint8_t *data, int length, LinkStats &stats)
    {
        if (length < 10)
        {
            return false; // Not enough data
        }

        // Parse ELRS link statistics (simplified)
        // Real implementation would follow ELRS telemetry format
        stats.rssi1 = static_cast<int>(data[0]) - 128; // Convert to signed
        stats.rssi2 = static_cast<int>(data[1]) - 128;
        stats.link_quality = data[2];
        stats.snr = static_cast<int>(data[3]) - 128;
        stats.tx_power = data[4];
        stats.valid = true;

        return true;
    }

    bool TelemetryHandler::parseBatteryInfo(const uint8_t *data, int length, BatteryInfo &battery)
    {
        if (length < 6)
        {
            return false; // Not enough data
        }

        // Parse battery telemetry (simplified)
        battery.voltage_mv = (data[0] << 8) | data[1];   // Big-endian voltage in mV
        battery.current_ma = (data[2] << 8) | data[3];   // Big-endian current in mA
        battery.capacity_mah = (data[4] << 8) | data[5]; // Big-endian capacity in mAh
        battery.valid = true;

        return true;
    }

    void TelemetryHandler::setError(const std::string &error)
    {
        last_error_ = error;
        std::cerr << "TELEMETRY_ERROR: " << error << std::endl;
    }

} // namespace ELRS

