#include "telemetry_handler.h"
#include "usb_bridge.h"
#include "radio_state.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <utility>
#include <cstdint>

namespace ELRS
{

    TelemetryHandler::TelemetryHandler(UsbBridge *usb_bridge)
        : usb_bridge_(usb_bridge)
    {
        msp_payload_.reserve(128);
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

            for (int i = 0; i < bytes_read; ++i)
            {
                feedMspByte(buffer[i]);
            }

            // 50Hz telemetry loop (20ms interval)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        std::cout << "📡 TELEMETRY_LOOP: Exited" << std::endl;
    }

    void TelemetryHandler::feedMspByte(uint8_t byte)
    {
        switch (msp_state_)
        {
        case MspParserState::Idle:
            if (byte == '$')
            {
                msp_state_ = MspParserState::ExpectM;
            }
            break;

        case MspParserState::ExpectM:
            if (byte == 'M')
            {
                msp_state_ = MspParserState::ExpectDirection;
            }
            else
            {
                resetMspParser();
            }
            break;

        case MspParserState::ExpectDirection:
            if (byte == '<' || byte == '>')
            {
                msp_from_device_ = (byte == '>');
                msp_state_ = MspParserState::ExpectLength;
            }
            else
            {
                resetMspParser();
            }
            break;

        case MspParserState::ExpectLength:
            msp_expected_length_ = byte;
            msp_checksum_ = byte;
            msp_payload_.clear();
            msp_state_ = MspParserState::ExpectFunction;
            break;

        case MspParserState::ExpectFunction:
            msp_function_ = byte;
            msp_checksum_ ^= byte;
            if (msp_expected_length_ == 0)
            {
                msp_state_ = MspParserState::ExpectChecksum;
            }
            else
            {
                msp_state_ = MspParserState::ReadPayload;
            }
            break;

        case MspParserState::ReadPayload:
            msp_payload_.push_back(byte);
            msp_checksum_ ^= byte;
            if (msp_payload_.size() >= msp_expected_length_)
            {
                msp_state_ = MspParserState::ExpectChecksum;
            }
            break;

        case MspParserState::ExpectChecksum:
        {
            uint8_t expected = msp_checksum_;
            bool checksum_ok = (expected == byte);
            if (checksum_ok)
            {
                handleMspFrame(msp_function_, msp_from_device_, msp_payload_);
            }
            resetMspParser();
            break;
        }
        }
    }

    void TelemetryHandler::resetMspParser()
    {
        msp_state_ = MspParserState::Idle;
        msp_from_device_ = false;
        msp_expected_length_ = 0;
        msp_function_ = 0;
        msp_checksum_ = 0;
        msp_payload_.clear();
    }

    void TelemetryHandler::handleMspFrame(uint8_t function, bool fromDevice, const std::vector<uint8_t> &payload)
    {
        if (!fromDevice)
        {
            return;
        }

        LinkStats link_stats;
        BatteryInfo battery_info;

        switch (function)
        {
        case 0x2D: // ELRS telemetry response
            if (parseLinkStats(payload.data(), static_cast<int>(payload.size()), link_stats))
            {
                latest_link_stats_ = link_stats;
                if (link_stats_callback_)
                {
                    link_stats_callback_(link_stats);
                }
            }
            break;

        case 0x2E: // Battery telemetry
            if (parseBatteryInfo(payload.data(), static_cast<int>(payload.size()), battery_info))
            {
                latest_battery_ = battery_info;
                if (battery_callback_)
                {
                    battery_callback_(battery_info);
                }
            }
            break;

        default:
            break;
        }
    }

    bool TelemetryHandler::parseLinkStats(const uint8_t *data, int length, LinkStats &stats)
    {
        if (!data || length <= 0)
        {
            return false;
        }

        int offset = 0;

        if (length >= 10)
        {
            // Legacy ELRS/CRSF-style payload with dual RSSI and extended scalars
            stats.rssi1 = static_cast<int>(static_cast<int8_t>(data[0]));
            stats.rssi2 = static_cast<int>(static_cast<int8_t>(data[1]));
            stats.link_quality = data[2];
            stats.snr = static_cast<int>(static_cast<int8_t>(data[3]));
            stats.tx_power = data[4];
            stats.valid = true;
            offset = 10; // Preserve existing behaviour: extra telemetry bytes occupy first 10 slots
        }
        else if (length >= 4)
        {
            // Compact payload: RSSI, LQ, SNR, TX power followed by optional bins
            stats.rssi1 = static_cast<int>(static_cast<int8_t>(data[0]));
            stats.rssi2 = stats.rssi1;
            stats.link_quality = data[1];
            stats.snr = static_cast<int>(static_cast<int8_t>(data[2]));
            stats.tx_power = data[3];
            stats.valid = true;
            offset = 4;
        }
        else
        {
            return false;
        }

        if (length > offset)
        {
            std::vector<int> spectrum;
            spectrum.reserve(length - offset);
            for (int i = offset; i < length; ++i)
            {
                spectrum.push_back(static_cast<int>(data[i]));
            }

            if (!spectrum.empty())
            {
                latest_spectrum_ = std::move(spectrum);
                RadioState::getInstance().updateSpectrumData(latest_spectrum_);
                if (spectrum_callback_)
                {
                    spectrum_callback_(latest_spectrum_);
                }
            }
        }

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
