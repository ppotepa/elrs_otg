#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <array>
#include <mutex>

namespace ELRS
{

    class UsbBridge;        // Forward declaration
    class SerialBridge;     // Forward declaration
    class TelemetryHandler; // Forward declaration
    class MspCommands;      // Forward declaration

    /**
     * Main ELRS Transmitter Controller
     * Handles 250Hz CRSF transmission loop and control logic
     */
    class ElrsTransmitter
    {
    public:
        struct ControlInputs
        {
            float roll = 0.0f;     // -1.0 to +1.0
            float pitch = 0.0f;    // -1.0 to +1.0
            float yaw = 0.0f;      // -1.0 to +1.0
            float throttle = 0.0f; // 0.0 to +1.0
            bool armed = false;    // AUX1 channel for arming
            bool mode1 = false;    // AUX2 channel
            bool mode2 = false;    // AUX3 channel
        };

        ElrsTransmitter(UsbBridge *usb_bridge);
        ElrsTransmitter(SerialBridge *serial_bridge); // Constructor for serial mode
        ~ElrsTransmitter();

        // Control the transmitter
        bool start();
        void stop();
        bool isRunning() const { return running_.load(); }

        // Set control inputs
        void setControlInputs(const ControlInputs &inputs);
        ControlInputs getControlInputs() const;

        // Safety controls
        void setArmed(bool armed);
        bool isArmed() const { return control_inputs_.armed; }
        void emergencyStop();

        // Access to subsystems
        TelemetryHandler *getTelemetryHandler() const { return telemetry_handler_.get(); }
        MspCommands *getMspCommands() const { return msp_commands_.get(); }

        std::string getLastError() const { return last_error_; }

    private:
        UsbBridge *usb_bridge_;
        SerialBridge *serial_bridge_;
        bool using_serial_mode_;
        std::unique_ptr<TelemetryHandler> telemetry_handler_;
        std::unique_ptr<MspCommands> msp_commands_;

        // Transmitter state
        std::atomic<bool> running_{false};
        std::unique_ptr<std::thread> tx_thread_;
        ControlInputs control_inputs_;
        mutable std::mutex inputs_mutex_;

        std::string last_error_;

        // 250Hz transmission loop
        void transmissionLoop();
        void buildChannelFrame(std::array<uint8_t, 26> &frame);

        void setError(const std::string &error);
    };

} // namespace ELRS
