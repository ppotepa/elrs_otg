#include "elrs_transmitter.h"
#include "usb_bridge.h"
#include "serial_bridge.h"
#include "telemetry_handler.h"
#include "msp_commands.h"
#include "crsf_protocol.h"
#include <iostream>
#include <chrono>
#include <mutex>
#include <cmath>

namespace ELRS
{

    ElrsTransmitter::ElrsTransmitter(UsbBridge *usb_bridge)
        : usb_bridge_(usb_bridge), serial_bridge_(nullptr), using_serial_mode_(false)
    {
        // Initialize subsystems
        telemetry_handler_ = std::make_unique<TelemetryHandler>(usb_bridge);
        msp_commands_ = std::make_unique<MspCommands>(usb_bridge);

        std::cout << "[INIT] ELRS_TX: Transmitter initialized (USB mode)" << std::endl;
    }

    ElrsTransmitter::ElrsTransmitter(SerialBridge *serial_bridge)
        : usb_bridge_(nullptr), serial_bridge_(serial_bridge), using_serial_mode_(true)
    {
        // For serial mode, we'll create simplified telemetry and MSP handlers
        // or implement serial versions later
        std::cout << "[INIT] ELRS_TX: Transmitter initialized (Serial mode)" << std::endl;
        std::cout << "[INIT] SERIAL_TX: Ready for CRSF over serial at 420kbaud" << std::endl;
    }

    ElrsTransmitter::~ElrsTransmitter()
    {
        stop();
    }

    bool ElrsTransmitter::start()
    {
        if (running_.load())
        {
            return true; // Already running
        }

        // Check connection based on mode
        if (using_serial_mode_)
        {
            if (!serial_bridge_ || !serial_bridge_->isConnected())
            {
                setError("Serial device not connected");
                return false;
            }
        }
        else
        {
            if (!usb_bridge_ || !usb_bridge_->isConnected())
            {
                setError("USB device not connected");
                return false;
            }
        }

        running_.store(true);
        tx_thread_ = std::make_unique<std::thread>(&ElrsTransmitter::transmissionLoop, this);

        // Start telemetry monitoring (USB mode only for now)
        if (telemetry_handler_)
        {
            telemetry_handler_->start();
        }

        if (using_serial_mode_)
        {
            std::cout << "[CRSF] SERIAL_TX_START: ✅ CRSF transmitter active at 250Hz (Serial mode)!" << std::endl;
            std::cout << "[CRSF] SERIAL_ACTIVE: Sending channel data every 4ms over COM port" << std::endl;
        }
        else
        {
            std::cout << "[CRSF] TX_LOOP_START: [OK] CRSF transmitter active at 250Hz!" << std::endl;
            std::cout << "[CRSF] TX_LOOP_ACTIVE: Sending channel data every 4ms" << std::endl;
        }
        std::cout << "[CRSF] TX_CHANNELS: AETR + AUX mapping active" << std::endl;

        return true;
    }

    void ElrsTransmitter::stop()
    {
        if (!running_.load())
        {
            return; // Already stopped
        }

        running_.store(false);

        // Stop telemetry first
        telemetry_handler_->stop();

        // Wait for TX thread to finish
        if (tx_thread_ && tx_thread_->joinable())
        {
            tx_thread_->join();
        }
        tx_thread_.reset();

        std::cout << "🚁 TX_LOOP_STOP: ✅ CRSF transmission stopped" << std::endl;
        std::cout << "🚁 TX_LOOP_INACTIVE: Transmitter should show 'No Signal'" << std::endl;
    }

    void ElrsTransmitter::setControlInputs(const ControlInputs &inputs)
    {
        std::lock_guard<std::mutex> lock(inputs_mutex_);

        // Log significant control changes
        static ControlInputs last_inputs;
        static int input_counter = 0;

        bool significant_change = (std::abs(inputs.roll - last_inputs.roll) > 0.1f) ||
                                  (std::abs(inputs.pitch - last_inputs.pitch) > 0.1f) ||
                                  (std::abs(inputs.yaw - last_inputs.yaw) > 0.1f) ||
                                  (std::abs(inputs.throttle - last_inputs.throttle) > 0.1f) ||
                                  (inputs.armed != last_inputs.armed);

        control_inputs_ = inputs;

        if (significant_change || (++input_counter % 100 == 0))
        {
            std::cout << "🎮 CONTROL_INPUT: R=" << inputs.roll << " P=" << inputs.pitch
                      << " Y=" << inputs.yaw << " T=" << inputs.throttle;
            if (inputs.armed)
            {
                std::cout << " [ARMED]";
            }
            std::cout << std::endl;

            last_inputs = inputs;
        }
    }

    ElrsTransmitter::ControlInputs ElrsTransmitter::getControlInputs() const
    {
        std::lock_guard<std::mutex> lock(inputs_mutex_);
        return control_inputs_;
    }

    void ElrsTransmitter::setArmed(bool armed)
    {
        std::lock_guard<std::mutex> lock(inputs_mutex_);

        if (armed != control_inputs_.armed)
        {
            control_inputs_.armed = armed;

            if (armed)
            {
                std::cout << "🔴 CRITICAL_COMMAND: ARM initiated - DRONE IS NOW ARMED!" << std::endl;
                std::cout << "🔴 ARM_WARNING: Propellers may spin - ensure safe distance!" << std::endl;
            }
            else
            {
                std::cout << "🟢 SAFETY_COMMAND: DISARM initiated - Drone is now SAFE" << std::endl;
                std::cout << "🟢 DISARM_CONFIRMED: Propellers should stop spinning" << std::endl;
            }
        }
    }

    void ElrsTransmitter::emergencyStop()
    {
        std::lock_guard<std::mutex> lock(inputs_mutex_);

        // Emergency: Set everything to safe values
        control_inputs_.roll = 0.0f;
        control_inputs_.pitch = 0.0f;
        control_inputs_.yaw = 0.0f;
        control_inputs_.throttle = 0.0f;
        control_inputs_.armed = false;
        control_inputs_.mode1 = false;
        control_inputs_.mode2 = false;

        std::cout << "🚨 EMERGENCY_STOP: All controls zeroed and disarmed!" << std::endl;
    }

    void ElrsTransmitter::transmissionLoop()
    {
        std::array<uint8_t, 26> crsf_frame;
        auto last_time = std::chrono::high_resolution_clock::now();

        if (using_serial_mode_)
        {
            std::cout << "🚁 SERIAL_TX_LOOP: Started 250Hz transmission loop (Serial mode)" << std::endl;
        }
        else
        {
            std::cout << "🚁 TX_LOOP: Started 250Hz transmission loop (USB mode)" << std::endl;
        }

        while (running_.load())
        {
            // Check connection based on mode
            bool connected = false;
            if (using_serial_mode_)
            {
                connected = serial_bridge_ && serial_bridge_->isConnected();
            }
            else
            {
                connected = usb_bridge_ && usb_bridge_->isConnected();
            }

            if (!connected)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Build CRSF frame with current inputs
            buildChannelFrame(crsf_frame);

            // Send frame to transmitter based on mode
            bool write_success = false;
            if (using_serial_mode_)
            {
                write_success = serial_bridge_->write(crsf_frame.data(), crsf_frame.size());
            }
            else
            {
                write_success = usb_bridge_->write(crsf_frame.data(), crsf_frame.size());
            }

            if (!write_success)
            {
                // Don't spam errors, just continue
                static int error_count = 0;
                if (++error_count % 50 == 0)
                { // Every 200ms
                    if (using_serial_mode_)
                    {
                        std::cout << "⚠️  SERIAL_TX_ERROR: Failed to send CRSF frame (count: " << error_count << ")" << std::endl;
                    }
                    else
                    {
                        std::cout << "⚠️  TX_ERROR: Failed to send CRSF frame (count: " << error_count << ")" << std::endl;
                    }
                }
            }

            // Maintain 250Hz (4ms interval)
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_time);
            auto target_interval = std::chrono::microseconds(4000); // 4ms = 250Hz

            if (elapsed < target_interval)
            {
                std::this_thread::sleep_for(target_interval - elapsed);
            }

            last_time = std::chrono::high_resolution_clock::now();
        }

        std::cout << "🚁 TX_LOOP: Transmission loop exited" << std::endl;
    }

    void ElrsTransmitter::buildChannelFrame(std::array<uint8_t, 26> &frame)
    {
        ControlInputs inputs = getControlInputs();

        // Build 16-channel array for CRSF
        uint16_t channels[16];

        // AETR mapping (Aileron, Elevator, Throttle, Rudder)
        channels[0] = CrsfProtocol::mapStickToChannel(inputs.roll);        // Aileron (Roll)
        channels[1] = CrsfProtocol::mapStickToChannel(inputs.pitch);       // Elevator (Pitch)
        channels[2] = CrsfProtocol::mapThrottleToChannel(inputs.throttle); // Throttle
        channels[3] = CrsfProtocol::mapStickToChannel(inputs.yaw);         // Rudder (Yaw)

        // AUX channels
        channels[4] = inputs.armed ? CrsfProtocol::CRSF_CHANNEL_VALUE_MAX : CrsfProtocol::CRSF_CHANNEL_VALUE_MIN; // AUX1 (ARM)
        channels[5] = inputs.mode1 ? CrsfProtocol::CRSF_CHANNEL_VALUE_MAX : CrsfProtocol::CRSF_CHANNEL_VALUE_MIN; // AUX2
        channels[6] = inputs.mode2 ? CrsfProtocol::CRSF_CHANNEL_VALUE_MAX : CrsfProtocol::CRSF_CHANNEL_VALUE_MIN; // AUX3

        // Fill remaining channels with middle values
        for (int i = 7; i < 16; i++)
        {
            channels[i] = CrsfProtocol::CRSF_CHANNEL_VALUE_MID;
        }

        // Build CRSF frame
        CrsfProtocol::buildRcChannelsFrame(channels, frame);
    }

    void ElrsTransmitter::setError(const std::string &error)
    {
        last_error_ = error;
        std::cerr << "ELRS_TX_ERROR: " << error << std::endl;
    }

} // namespace ELRS
