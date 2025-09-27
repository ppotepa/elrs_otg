#include "screens/rxtest_screen.h"
#include <iostream>
#include <iomanip>
#include <random>

namespace ELRS
{
    namespace UI
    {
        RxTestScreen::RxTestScreen()
            : ScreenBase(ScreenType::RxTest, "RX Test"), lastUpdate_(std::chrono::steady_clock::now()), isRecording_(false), selectedChannel_(0)
        {
        }

        bool RxTestScreen::initialize(const RenderContext &renderContext,
                                      const NavigationContext &navContext)
        {
            logInfo("Initializing RX test screen");
            initializeChannels();
            return true;
        }

        void RxTestScreen::onActivate()
        {
            logInfo("RX test screen activated");
            markForRefresh();
        }

        void RxTestScreen::onDeactivate()
        {
            logDebug("RX test screen deactivated");
            isRecording_ = false;
        }

        void RxTestScreen::update(std::chrono::milliseconds deltaTime)
        {
            // Check if channel data has changed
            bool dataChanged = false;

            // Simple approach: check if any channel values have changed
            // In a real implementation, we'd listen to actual RC channel updates
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

            if (elapsed.count() >= RX_UPDATE_INTERVAL_MS)
            {
                // Simulate channel data updates and check for changes
                auto oldChannels = channels_;
                updateChannelData();

                // Check if any channel values changed
                for (size_t i = 0; i < channels_.size() && i < oldChannels.size(); ++i)
                {
                    if (channels_[i].value != oldChannels[i].value)
                    {
                        dataChanged = true;
                        break;
                    }
                }

                if (dataChanged)
                {
                    markForRefresh();
                }
                lastUpdate_ = now;
            }
        }

        void RxTestScreen::render(const RenderContext &renderContext)
        {
            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Receiver Channel Test", Color::BrightWhite);

            // Render content
            renderChannelGrid(renderContext);
            renderChannelDetails(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);
            std::cout << "UP/DOWN: Select Channel | SPACE: " << (isRecording_ ? "Stop" : "Start")
                      << " Recording | ESC/F1: Return | F12: Exit";

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(isRecording_ ? Color::BrightYellow : Color::BrightGreen);
            std::cout << " RX Test | " << (isRecording_ ? "RECORDING" : "MONITORING")
                      << " | Active Channels: " << channels_.size() << " | Rate: 20Hz";

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool RxTestScreen::handleKeyPress(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::ArrowUp:
                if (selectedChannel_ > 0)
                {
                    selectedChannel_--;
                    markForRefresh();
                }
                return true;
            case FunctionKey::ArrowDown:
                if (selectedChannel_ < static_cast<int>(channels_.size()) - 1)
                {
                    selectedChannel_++;
                    markForRefresh();
                }
                return true;
            case FunctionKey::Space:
                isRecording_ = !isRecording_;
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

        void RxTestScreen::cleanup()
        {
            logInfo("Cleaning up RX test screen");
        }

        void RxTestScreen::initializeChannels()
        {
            channels_.clear();
            const std::vector<std::string> channelNames = {
                "ROLL", "PITCH", "THROTTLE", "YAW", "AUX1", "AUX2", "AUX3", "AUX4",
                "AUX5", "AUX6", "AUX7", "AUX8", "AUX9", "AUX10", "AUX11", "AUX12"};

            for (int i = 0; i < 16; ++i)
            {
                ChannelTest channel;
                channel.channel = i + 1;
                channel.name = channelNames[i];
                channel.value = 1500; // Center value
                channel.minValue = 1000;
                channel.maxValue = 2000;
                channel.isActive = i < 8; // First 8 channels active by default
                channels_.push_back(channel);
            }
        }

        void RxTestScreen::updateChannelData()
        {
            // Simulate channel data with some variation
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> variation(-50, 50);

            for (auto &channel : channels_)
            {
                if (channel.isActive)
                {
                    // Simulate realistic channel movements
                    if (channel.name == "THROTTLE")
                    {
                        channel.value = 1000 + variation(gen) + 200; // Lower range for throttle
                    }
                    else if (channel.name == "ROLL" || channel.name == "PITCH" || channel.name == "YAW")
                    {
                        channel.value = 1500 + variation(gen); // Center around 1500
                    }
                    else
                    {
                        // AUX channels - simulate switch positions
                        if (channel.value < 1200 || channel.value > 1800)
                        {
                            channel.value = 1500 + variation(gen);
                        }
                        else
                        {
                            int rand = variation(gen);
                            if (rand > 30)
                                channel.value = 2000;
                            else if (rand < -30)
                                channel.value = 1000;
                            else
                                channel.value = 1500;
                        }
                    }

                    // Clamp values
                    channel.value = (std::max)(channel.minValue, (std::min)(channel.maxValue, channel.value));
                }
            }
        }

        void RxTestScreen::renderChannelGrid(const RenderContext &context)
        {
            int startY = 3;
            int gridWidth = context.terminalWidth - 4;
            int channelsPerRow = 4;
            int channelWidth = gridWidth / channelsPerRow;

            setColor(Color::BrightYellow);
            drawBox(2, startY, gridWidth, 10, "RC Channels");

            for (int i = 0; i < static_cast<int>(channels_.size()) && i < 16; ++i)
            {
                const auto &channel = channels_[i];
                int row = i / channelsPerRow;
                int col = i % channelsPerRow;
                int channelX = 4 + (col * channelWidth);
                int channelY = startY + 2 + (row * 2);

                // Highlight selected channel
                if (i == selectedChannel_)
                {
                    setColor(Color::BrightWhite);
                    moveCursor(channelX - 1, channelY);
                    std::cout << ">";
                }

                // Channel name
                moveCursor(channelX, channelY);
                setColor(channel.isActive ? Color::BrightGreen : Color::BrightBlack);
                std::cout << "CH" << std::setw(2) << channel.channel << " " << channel.name;

                // Channel value with bar
                moveCursor(channelX, channelY + 1);
                if (channel.isActive)
                {
                    setColor(Color::BrightCyan);
                    std::cout << std::setw(4) << channel.value;

                    // Value bar
                    int barLength = 10;
                    int barValue = (channel.value - channel.minValue) * barLength /
                                   (channel.maxValue - channel.minValue);
                    barValue = (std::max)(0, (std::min)(barLength, barValue));

                    std::cout << " [";
                    for (int b = 0; b < barLength; ++b)
                    {
                        if (b < barValue)
                            std::cout << "■";
                        else
                            std::cout << " ";
                    }
                    std::cout << "]";
                }
                else
                {
                    setColor(Color::BrightBlack);
                    std::cout << "----";
                }
            }
        }

        void RxTestScreen::renderChannelDetails(const RenderContext &context)
        {
            if (selectedChannel_ >= 0 && selectedChannel_ < static_cast<int>(channels_.size()))
            {
                const auto &channel = channels_[selectedChannel_];
                int detailY = context.terminalHeight - 10;
                int detailWidth = context.terminalWidth - 4;

                setColor(Color::BrightCyan);
                drawBox(2, detailY, detailWidth, 6, "Channel Details");

                moveCursor(4, detailY + 1);
                setColor(Color::BrightWhite);
                std::cout << "Channel " << channel.channel << " (" << channel.name << ")";

                moveCursor(4, detailY + 2);
                setColor(Color::BrightGreen);
                std::cout << "Current Value: " << channel.value << " µs";

                moveCursor(30, detailY + 2);
                setColor(Color::BrightBlue);
                std::cout << "Range: " << channel.minValue << " - " << channel.maxValue << " µs";

                moveCursor(4, detailY + 3);
                setColor(channel.isActive ? Color::BrightGreen : Color::BrightRed);
                std::cout << "Status: " << (channel.isActive ? "Active" : "Inactive");

                moveCursor(30, detailY + 3);
                setColor(Color::BrightYellow);
                double percent = 100.0 * (channel.value - channel.minValue) / (channel.maxValue - channel.minValue);
                std::cout << "Position: " << std::fixed << std::setprecision(1) << percent << "%";

                moveCursor(4, detailY + 4);
                setColor(Color::White);
                if (channel.name == "THROTTLE")
                    std::cout << "Function: Engine/Motor Control";
                else if (channel.name == "ROLL" || channel.name == "PITCH" || channel.name == "YAW")
                    std::cout << "Function: Flight Control Axis";
                else
                    std::cout << "Function: Auxiliary Switch/Analog";
            }
        }
    }
}
