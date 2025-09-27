#include "screens/config_screen.h"
#include <iostream>
#include <iomanip>

namespace ELRS
{
    namespace UI
    {
        ConfigScreen::ConfigScreen()
            : ScreenBase(ScreenType::Config, "Config"), selectedOption_(0), inEditMode_(false), lastUpdate_(std::chrono::steady_clock::now())
        {
        }

        bool ConfigScreen::initialize(const RenderContext &renderContext,
                                      const NavigationContext &navContext)
        {
            logInfo("Initializing config screen");
            initializeConfigOptions();
            loadConfiguration();
            return true;
        }

        void ConfigScreen::onActivate()
        {
            logInfo("Config screen activated");
            markForRefresh();
        }

        void ConfigScreen::onDeactivate()
        {
            logDebug("Config screen deactivated");
            if (inEditMode_)
            {
                inEditMode_ = false;
                editBuffer_.clear();
            }
        }

        void ConfigScreen::update(std::chrono::milliseconds deltaTime)
        {
            // Only refresh if in edit mode (user actively editing) or configuration changed
            if (inEditMode_)
            {
                // Refresh periodically while editing to show cursor blink, etc.
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

                if (elapsed.count() >= CONFIG_UPDATE_INTERVAL_MS)
                {
                    markForRefresh();
                    lastUpdate_ = now;
                }
            }
            // Note: Configuration changes will trigger refresh via markForRefresh() in key handlers
        }

        void ConfigScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Device Configuration", Color::BrightWhite);

            // Render config content
            renderConfigList(renderContext);
            renderConfigDetails(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);

            if (inEditMode_)
            {
                std::cout << "EDIT MODE: Enter to save, ESC to cancel, Type to modify value";
            }
            else
            {
                std::cout << "UP/DOWN: Navigate, ENTER: Edit, S: Save Config, ESC/F1: Return, F12: Exit";
            }

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(Color::BrightGreen);

            auto &radioState = getRadioState();
            auto deviceConfig = radioState.getDeviceConfiguration();

            std::cout << " Configuration | Device: " << deviceConfig.productName
                      << " | Options: " << configOptions_.size();

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool ConfigScreen::handleKeyPress(FunctionKey key)
        {
            if (inEditMode_)
            {
                switch (key)
                {
                case FunctionKey::Escape:
                    inEditMode_ = false;
                    editBuffer_.clear();
                    markForRefresh();
                    return true;
                case FunctionKey::Enter:
                    if (selectedOption_ >= 0 && selectedOption_ < static_cast<int>(configOptions_.size()))
                    {
                        configOptions_[selectedOption_].value = editBuffer_;
                        inEditMode_ = false;
                        editBuffer_.clear();
                        markForRefresh();
                    }
                    return true;
                case FunctionKey::Backspace:
                    if (!editBuffer_.empty())
                    {
                        editBuffer_.pop_back();
                        markForRefresh();
                    }
                    return true;
                // Handle character input would go here
                default:
                    return false;
                }
            }
            else
            {
                switch (key)
                {
                case FunctionKey::ArrowUp:
                    if (selectedOption_ > 0)
                    {
                        selectedOption_--;
                        markForRefresh();
                    }
                    return true;
                case FunctionKey::ArrowDown:
                    if (selectedOption_ < static_cast<int>(configOptions_.size()) - 1)
                    {
                        selectedOption_++;
                        markForRefresh();
                    }
                    return true;
                case FunctionKey::Enter:
                    if (selectedOption_ >= 0 && selectedOption_ < static_cast<int>(configOptions_.size()))
                    {
                        if (configOptions_[selectedOption_].isEditable)
                        {
                            inEditMode_ = true;
                            editBuffer_ = configOptions_[selectedOption_].value;
                            markForRefresh();
                        }
                    }
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
        }

        void ConfigScreen::cleanup()
        {
            logInfo("Cleaning up config screen");
        }

        void ConfigScreen::initializeConfigOptions()
        {
            configOptions_.clear();

            configOptions_.push_back({"RF Mode", "Normal", "Radio frequency operation mode", true});
            configOptions_.push_back({"TX Power", "100mW", "Transmitter output power", true});
            configOptions_.push_back({"Packet Rate", "250Hz", "Packet transmission rate", true});
            configOptions_.push_back({"TLM Ratio", "1:4", "Telemetry to RC packet ratio", true});
            configOptions_.push_back({"Switch Mode", "Hybrid", "RC switch operation mode", true});
            configOptions_.push_back({"Model Match", "Off", "Model ID matching", true});
            configOptions_.push_back({"Dynamic Power", "On", "Automatic power adjustment", true});
            configOptions_.push_back({"Fan Threshold", "60°C", "Cooling fan activation temperature", true});
            configOptions_.push_back({"WiFi Update", "Off", "WiFi firmware update mode", true});
            configOptions_.push_back({"UART Baud", "420000", "Serial communication speed", true});
            configOptions_.push_back({"Firmware Version", "3.3.2", "Current firmware version", false});
            configOptions_.push_back({"Hardware Version", "2.4", "Hardware revision", false});
        }

        void ConfigScreen::renderConfigList(const RenderContext &context)
        {
            int startY = 3;
            int listWidth = context.terminalWidth / 2 - 2;

            // Config list box
            setColor(Color::BrightYellow);
            drawBox(1, startY, listWidth, configOptions_.size() + 4, "Configuration Options");

            // List items
            for (int i = 0; i < static_cast<int>(configOptions_.size()); ++i)
            {
                int itemY = startY + 2 + i;
                moveCursor(3, itemY);

                if (i == selectedOption_)
                {
                    setColor(inEditMode_ ? Color::BrightRed : Color::BrightWhite);
                    std::cout << "► ";
                }
                else
                {
                    setColor(Color::White);
                    std::cout << "  ";
                }

                std::cout << std::left << std::setw(listWidth - 8) << configOptions_[i].name;

                if (!configOptions_[i].isEditable)
                {
                    setColor(Color::BrightBlack);
                    std::cout << " (RO)";
                }
            }
        }

        void ConfigScreen::renderConfigDetails(const RenderContext &context)
        {
            int startY = 3;
            int startX = context.terminalWidth / 2 + 1;
            int detailWidth = context.terminalWidth / 2 - 2;

            if (selectedOption_ >= 0 && selectedOption_ < static_cast<int>(configOptions_.size()))
            {
                const auto &option = configOptions_[selectedOption_];

                // Detail box
                setColor(Color::BrightCyan);
                drawBox(startX, startY, detailWidth, 8, "Option Details");

                // Option name
                moveCursor(startX + 2, startY + 2);
                setColor(Color::BrightWhite);
                std::cout << "Name: " << option.name;

                // Current value
                moveCursor(startX + 2, startY + 3);
                setColor(inEditMode_ ? Color::BrightYellow : Color::BrightGreen);
                std::cout << "Value: ";
                if (inEditMode_)
                {
                    std::cout << editBuffer_ << "_";
                }
                else
                {
                    std::cout << option.value;
                }

                // Description
                moveCursor(startX + 2, startY + 4);
                setColor(Color::White);
                std::cout << "Description:";

                moveCursor(startX + 2, startY + 5);
                std::cout << option.description;

                // Status
                moveCursor(startX + 2, startY + 6);
                setColor(option.isEditable ? Color::BrightGreen : Color::BrightRed);
                std::cout << "Status: " << (option.isEditable ? "Editable" : "Read-Only");
            }
        }

        void ConfigScreen::saveConfiguration()
        {
            logInfo("Saving configuration to device");
            // Implementation would save to device
        }

        void ConfigScreen::loadConfiguration()
        {
            logInfo("Loading configuration from device");
            // Implementation would load from device
        }
    }
}
