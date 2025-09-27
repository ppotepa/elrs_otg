#include "screens/monitor_screen.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ELRS
{
    namespace UI
    {
        MonitorScreen::MonitorScreen()
            : ScreenBase(ScreenType::Monitor, "Monitor"), lastUpdate_(std::chrono::steady_clock::now()), isPaused_(false)
        {
        }

        bool MonitorScreen::initialize(const RenderContext &renderContext,
                                       const NavigationContext &navContext)
        {
            logInfo("Initializing monitor screen");
            updateMonitorValues();
            return true;
        }

        void MonitorScreen::onActivate()
        {
            logInfo("Monitor screen activated");
            isPaused_ = false;
            markForRefresh();
        }

        void MonitorScreen::onDeactivate()
        {
            logDebug("Monitor screen deactivated");
        }

        void MonitorScreen::update(std::chrono::milliseconds deltaTime)
        {
            if (isPaused_)
                return;

            // Check if telemetry data has changed since last update
            const auto &radioState = getRadioState();
            const auto &telemetry = radioState.getLiveTelemetry();

            if (telemetry.lastUpdate > lastUpdate_)
            {
                updateMonitorValues();
                markForRefresh();
                lastUpdate_ = telemetry.lastUpdate;
            }
        }

        void MonitorScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Real-Time Telemetry Monitor", ELRS::UI::Color::BrightWhite);

            // Render monitor content
            renderMonitorGrid(renderContext);
            renderAlerts(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(ELRS::UI::Color::BrightBlue);
            std::cout << "SPACE: " << (isPaused_ ? "Resume" : "Pause")
                      << " | R: Reset | ESC/F1: Return | F12: Exit";

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(isPaused_ ? ELRS::UI::Color::BrightYellow : ELRS::UI::Color::BrightGreen);

            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();

            std::cout << " Monitor " << (isPaused_ ? "[PAUSED]" : "[LIVE]")
                      << " | Link Quality: " << telemetry.linkQuality << "% | Update Rate: 10Hz";

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool MonitorScreen::handleKeyPress(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::Space:
                isPaused_ = !isPaused_;
                markForRefresh();
                return true;
            case FunctionKey::Escape:
            case FunctionKey::F1:
                navigateToScreen(ScreenType::Main);
                return true;
            case FunctionKey::F12:
                exitApplication();
                return true;
            default:
                return false;
            }
        }

        void MonitorScreen::cleanup()
        {
            logInfo("Cleaning up monitor screen");
        }

        void MonitorScreen::updateMonitorValues()
        {
            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();
            auto deviceConfig = radioState.getDeviceConfiguration();

            monitorValues_.clear();

            // RF Statistics
            std::stringstream rssi;
            rssi << std::fixed << std::setprecision(1) << telemetry.rssi1;
            monitorValues_.push_back({"RSSI 1", rssi.str(), "dBm",
                                      telemetry.rssi1 > -70 ? ELRS::UI::Color::BrightGreen : telemetry.rssi1 > -90 ? ELRS::UI::Color::BrightYellow
                                                                                                                   : ELRS::UI::Color::BrightRed,
                                      telemetry.rssi1 < -100});

            rssi.str("");
            rssi << std::fixed << std::setprecision(1) << telemetry.rssi2;
            monitorValues_.push_back({"RSSI 2", rssi.str(), "dBm",
                                      telemetry.rssi2 > -70 ? ELRS::UI::Color::BrightGreen : telemetry.rssi2 > -90 ? ELRS::UI::Color::BrightYellow
                                                                                                                   : ELRS::UI::Color::BrightRed,
                                      telemetry.rssi2 < -100});

            monitorValues_.push_back({"Link Quality", std::to_string(telemetry.linkQuality), "%",
                                      telemetry.linkQuality > 80 ? ELRS::UI::Color::BrightGreen : telemetry.linkQuality > 50 ? ELRS::UI::Color::BrightYellow
                                                                                                                             : ELRS::UI::Color::BrightRed,
                                      telemetry.linkQuality < 20});

            monitorValues_.push_back({"SNR", std::to_string(telemetry.snr), "dB",
                                      telemetry.snr > 5 ? ELRS::UI::Color::BrightGreen : telemetry.snr > 0 ? ELRS::UI::Color::BrightYellow
                                                                                                           : ELRS::UI::Color::BrightRed,
                                      telemetry.snr < -5});

            // Power and Performance
            monitorValues_.push_back({"TX Power", std::to_string(telemetry.txPower), "dBm", ELRS::UI::Color::BrightCyan, false});

            std::stringstream temp;
            temp << std::fixed << std::setprecision(1) << telemetry.temperature;
            monitorValues_.push_back({"Temperature", temp.str(), "°C",
                                      telemetry.temperature < 60 ? ELRS::UI::Color::BrightGreen : telemetry.temperature < 80 ? ELRS::UI::Color::BrightYellow
                                                                                                                             : ELRS::UI::Color::BrightRed,
                                      telemetry.temperature > 85});

            monitorValues_.push_back({"Voltage", std::to_string(telemetry.voltage), "V",
                                      telemetry.voltage > 4.5 ? ELRS::UI::Color::BrightGreen : telemetry.voltage > 4.0 ? ELRS::UI::Color::BrightYellow
                                                                                                                       : ELRS::UI::Color::BrightRed,
                                      telemetry.voltage < 3.5});

            // Packet Statistics
            monitorValues_.push_back({"Packets RX", std::to_string(telemetry.packetsReceived), "", ELRS::UI::Color::BrightBlue, false});
            monitorValues_.push_back({"Packets TX", std::to_string(telemetry.packetsTransmitted), "", ELRS::UI::Color::BrightBlue, false});

            if (telemetry.packetsReceived > 0)
            {
                double packetLoss = 100.0 * (1.0 - (double)telemetry.packetsReceived /
                                                       (telemetry.packetsReceived + telemetry.packetsLost));
                std::stringstream loss;
                loss << std::fixed << std::setprecision(2) << packetLoss;
                monitorValues_.push_back({"Packet Loss", loss.str(), "%",
                                          packetLoss < 1 ? ELRS::UI::Color::BrightGreen : packetLoss < 5 ? ELRS::UI::Color::BrightYellow
                                                                                                         : ELRS::UI::Color::BrightRed,
                                          packetLoss > 10});
            }
            else
            {
                monitorValues_.push_back({"Packet Loss", "0.00", "%", Color::BrightGreen, false});
            }

            // Timing
            monitorValues_.push_back({"Packet Rate", std::to_string(telemetry.packetRate), "Hz", Color::BrightMagenta, false});
        }

        void MonitorScreen::renderMonitorGrid(const RenderContext &context)
        {
            int startY = 3;
            int gridWidth = context.terminalWidth - 4;
            int itemsPerRow = 3;
            int itemWidth = gridWidth / itemsPerRow;

            // Main monitoring box
            setColor(ELRS::UI::Color::BrightYellow);
            drawBox(2, startY, gridWidth, 12, "Telemetry Monitor");

            int row = 0;
            int col = 0;

            for (const auto &value : monitorValues_)
            {
                int itemX = 4 + (col * itemWidth);
                int itemY = startY + 2 + (row * 3);

                // Value name
                moveCursor(itemX, itemY);
                setColor(ELRS::UI::Color::White);
                std::cout << value.name << ":";

                // Value with unit
                moveCursor(itemX, itemY + 1);
                setColor(value.color);
                std::string displayValue = value.value;
                if (!value.unit.empty())
                {
                    displayValue += " " + value.unit;
                }

                if (value.isAlert)
                {
                    std::cout << "⚠ " << displayValue;
                }
                else
                {
                    std::cout << displayValue;
                }

                col++;
                if (col >= itemsPerRow)
                {
                    col = 0;
                    row++;
                }
            }
        }

        void MonitorScreen::renderAlerts(const RenderContext &context)
        {
            std::vector<std::string> alerts;

            for (const auto &value : monitorValues_)
            {
                if (value.isAlert)
                {
                    alerts.push_back(value.name + ": " + value.value + " " + value.unit);
                }
            }

            if (!alerts.empty())
            {
                int alertY = context.terminalHeight - 8;
                int alertWidth = context.terminalWidth - 4;

                setColor(ELRS::UI::Color::BrightRed);
                drawBox(2, alertY, alertWidth, alerts.size() + 2, "⚠ ALERTS ⚠");

                for (size_t i = 0; i < alerts.size(); ++i)
                {
                    moveCursor(4, alertY + 1 + static_cast<int>(i));
                    setColor(ELRS::UI::Color::BrightRed);
                    std::cout << "• " << alerts[i];
                }
            }
        }
    }
}
