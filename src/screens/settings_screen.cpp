#include "screens/settings_screen.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>

namespace ELRS
{
    namespace UI
    {
        SettingsScreen::SettingsScreen()
            : ScreenBase(ScreenType::Settings, "Settings"),
              selectedCategory_(0),
              selectedSetting_(0),
              inEditMode_(false),
              settingsModified_(false),
              lastUpdate_(std::chrono::steady_clock::now())
        {
            initializeSettings();
            loadSettings();
        }

        bool SettingsScreen::initialize(const RenderContext &renderContext, const NavigationContext &navContext)
        {
            logInfo("Initializing settings screen");
            return true;
        }

        void SettingsScreen::onActivate()
        {
            logInfo("Settings screen activated");
            markForRefresh();
        }

        void SettingsScreen::onDeactivate()
        {
            logDebug("Settings screen deactivated");
            if (inEditMode_)
            {
                inEditMode_ = false;
                editBuffer_.clear();
            }
        }

        void SettingsScreen::update(std::chrono::milliseconds deltaTime)
        {
            // Refresh periodically to show cursor blink in edit mode
            if (inEditMode_)
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

                if (elapsed.count() >= SETTINGS_UPDATE_INTERVAL_MS)
                {
                    markForRefresh();
                    lastUpdate_ = now;
                }
            }
        }

        void SettingsScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Application Settings", Color::BrightWhite);

            // Render settings content
            renderSettingsCategories(renderContext);
            renderSettingsList(renderContext);
            renderSettingDetails(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);

            if (inEditMode_)
            {
                std::cout << "EDIT MODE: Type new value, ENTER to save, ESC to cancel";
            }
            else
            {
                std::cout << "TAB: Switch Categories | UP/DOWN: Navigate | ENTER: Edit | S: Save | R: Reset | ESC/F1: Return | F12: Exit";
            }

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(settingsModified_ ? Color::BrightYellow : Color::BrightGreen);

            std::cout << " Settings | Category: " << (selectedCategory_ < static_cast<int>(categories_.size()) ? categories_[selectedCategory_] : "Unknown")
                      << " | Modified: " << (settingsModified_ ? "Yes" : "No");

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool SettingsScreen::handleKeyPress(FunctionKey key)
        {
            if (inEditMode_)
            {
                switch (key)
                {
                case FunctionKey::Enter:
                    applySettingChange(editBuffer_);
                    inEditMode_ = false;
                    editBuffer_.clear();
                    markForRefresh();
                    return true;
                case FunctionKey::Escape:
                    inEditMode_ = false;
                    editBuffer_.clear();
                    markForRefresh();
                    return true;
                case FunctionKey::Backspace:
                    if (!editBuffer_.empty())
                    {
                        editBuffer_.pop_back();
                        markForRefresh();
                    }
                    return true;
                default:
                    // Handle regular character input
                    if (key >= FunctionKey::Space && key <= FunctionKey::Tilde)
                    {
                        editBuffer_ += static_cast<char>(key);
                        markForRefresh();
                    }
                    return true;
                }
            }

            switch (key)
            {
            case FunctionKey::Tab:
                selectedCategory_ = (selectedCategory_ + 1) % categories_.size();
                selectedSetting_ = 0; // Reset setting selection
                markForRefresh();
                return true;
            case FunctionKey::ArrowUp:
                {
                    auto categorySettings = getCurrentCategorySettings();
                    if (selectedSetting_ > 0)
                    {
                        selectedSetting_--;
                        markForRefresh();
                    }
                }
                return true;
            case FunctionKey::ArrowDown:
                {
                    auto categorySettings = getCurrentCategorySettings();
                    if (selectedSetting_ < static_cast<int>(categorySettings.size()) - 1)
                    {
                        selectedSetting_++;
                        markForRefresh();
                    }
                }
                return true;
            case FunctionKey::Enter:
                editCurrentSetting();
                return true;
            case FunctionKey::S:
            case FunctionKey::s:
                saveSettings();
                return true;
            case FunctionKey::R:
            case FunctionKey::r:
                resetToDefaults();
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

        void SettingsScreen::cleanup()
        {
            logInfo("Cleaning up settings screen");
            if (settingsModified_)
            {
                logInfo("Settings were modified but not saved");
            }
        }

        void SettingsScreen::initializeSettings()
        {
            // Application Settings
            settings_.push_back({"Application", "Refresh Rate", "Screen refresh rate in milliseconds", 
                               SettingType::Integer, "100", "100", {}, 50, 1000, false});
            settings_.push_back({"Application", "Auto Save", "Automatically save configuration changes", 
                               SettingType::Boolean, "true", "true", {"true", "false"}, 0, 0, false});
            settings_.push_back({"Application", "Theme", "Application color theme", 
                               SettingType::Selection, "Default", "Default", {"Default", "Dark", "Light", "High Contrast"}, 0, 0, false});
            
            // Display Settings
            settings_.push_back({"Display", "Show Timestamps", "Display timestamps in status bars", 
                               SettingType::Boolean, "true", "true", {"true", "false"}, 0, 0, false});
            settings_.push_back({"Display", "Graph Points", "Maximum points to display in graphs", 
                               SettingType::Integer, "100", "100", {}, 10, 1000, false});
            settings_.push_back({"Display", "Animation Speed", "Animation speed multiplier", 
                               SettingType::Float, "1.0", "1.0", {}, 0, 5, false});
            
            // Connection Settings
            settings_.push_back({"Connection", "Baud Rate", "Serial port baud rate", 
                               SettingType::Selection, "420000", "420000", {"9600", "115200", "420000", "921600"}, 0, 0, false});
            settings_.push_back({"Connection", "Auto Connect", "Automatically connect to devices", 
                               SettingType::Boolean, "true", "true", {"true", "false"}, 0, 0, false});
            settings_.push_back({"Connection", "Connection Timeout", "Connection timeout in seconds", 
                               SettingType::Integer, "10", "10", {}, 1, 60, false});
            
            // Logging Settings
            settings_.push_back({"Logging", "Log Level", "Minimum log level to display", 
                               SettingType::Selection, "Info", "Info", {"Debug", "Info", "Warning", "Error"}, 0, 0, false});
            settings_.push_back({"Logging", "Max Log Entries", "Maximum number of log entries to keep", 
                               SettingType::Integer, "1000", "1000", {}, 100, 10000, false});
            settings_.push_back({"Logging", "Log to File", "Save logs to file", 
                               SettingType::Boolean, "false", "false", {"true", "false"}, 0, 0, false});
            
            // Telemetry Settings
            settings_.push_back({"Telemetry", "Update Rate", "Telemetry update rate in Hz", 
                               SettingType::Integer, "10", "10", {}, 1, 100, false});
            settings_.push_back({"Telemetry", "Data Retention", "How long to keep telemetry data (hours)", 
                               SettingType::Integer, "24", "24", {}, 1, 168, false});
            settings_.push_back({"Telemetry", "Auto Export", "Auto-export telemetry data", 
                               SettingType::Boolean, "false", "false", {"true", "false"}, 0, 0, false});
            
            // Extract unique categories
            std::set<std::string> categorySet;
            for (const auto &setting : settings_)
            {
                categorySet.insert(setting.category);
            }
            categories_.assign(categorySet.begin(), categorySet.end());
        }

        void SettingsScreen::loadSettings()
        {
            // In a real implementation, this would load from a configuration file
            logInfo("Loading settings from configuration");
            
            try
            {
                std::ifstream configFile("elrs_otg_config.json");
                if (configFile.is_open())
                {
                    // Simple JSON-like parsing (in real app, use proper JSON library)
                    std::string line;
                    while (std::getline(configFile, line))
                    {
                        // Basic parsing - look for "name": "value" patterns
                        size_t colonPos = line.find(':');
                        if (colonPos != std::string::npos)
                        {
                            std::string name = line.substr(0, colonPos);
                            std::string value = line.substr(colonPos + 1);
                            
                            // Remove quotes and whitespace
                            name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
                            name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
                            value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
                            value.erase(std::remove(value.begin(), value.end(), ' '), value.end());
                            value.erase(std::remove(value.begin(), value.end(), ','), value.end());
                            
                            // Find and update setting
                            for (auto &setting : settings_)
                            {
                                if (setting.name == name)
                                {
                                    setting.currentValue = value;
                                    setting.modified = (value != setting.defaultValue);
                                    break;
                                }
                            }
                        }
                    }
                    configFile.close();
                    logInfo("Settings loaded successfully");
                }
                else
                {
                    logInfo("No configuration file found, using defaults");
                }
            }
            catch (const std::exception &e)
            {
                logError("Failed to load settings: " + std::string(e.what()));
            }
        }

        void SettingsScreen::saveSettings()
        {
            logInfo("Saving settings to configuration file");
            
            try
            {
                std::ofstream configFile("elrs_otg_config.json");
                if (!configFile.is_open())
                {
                    logError("Failed to open configuration file for writing");
                    return;
                }
                
                configFile << "{\n";
                
                for (size_t i = 0; i < settings_.size(); i++)
                {
                    const auto &setting = settings_[i];
                    configFile << "  \"" << setting.name << "\": \"" << setting.currentValue << "\"";
                    if (i < settings_.size() - 1)
                        configFile << ",";
                    configFile << "\n";
                }
                
                configFile << "}\n";
                configFile.close();
                
                // Reset modified flags
                for (auto &setting : settings_)
                {
                    setting.modified = false;
                }
                settingsModified_ = false;
                
                logInfo("Settings saved successfully");
                markForRefresh();
            }
            catch (const std::exception &e)
            {
                logError("Failed to save settings: " + std::string(e.what()));
            }
        }

        void SettingsScreen::resetToDefaults()
        {
            logInfo("Resetting all settings to defaults");
            
            for (auto &setting : settings_)
            {
                setting.currentValue = setting.defaultValue;
                setting.modified = false;
            }
            
            settingsModified_ = true;
            markForRefresh();
        }

        void SettingsScreen::renderSettingsCategories(const RenderContext &renderContext)
        {
            int startY = 3;
            int startX = 5;

            // Categories header
            moveCursor(startX, startY);
            setColor(Color::BrightWhite);
            std::cout << "Categories:";

            // List categories
            for (size_t i = 0; i < categories_.size(); i++)
            {
                moveCursor(startX, startY + 2 + static_cast<int>(i));
                
                if (static_cast<int>(i) == selectedCategory_)
                {
                    setColor(Color::BrightBlue);
                    std::cout << "► " << categories_[i];
                }
                else
                {
                    setColor(Color::White);
                    std::cout << "  " << categories_[i];
                }
            }
        }

        void SettingsScreen::renderSettingsList(const RenderContext &renderContext)
        {
            int startY = 3;
            int startX = 25;
            int maxWidth = renderContext.terminalWidth - startX - 35;

            // Settings header
            moveCursor(startX, startY);
            setColor(Color::BrightWhite);
            std::cout << "Settings:";

            // List settings for current category
            auto categorySettings = getCurrentCategorySettings();
            
            for (size_t i = 0; i < categorySettings.size(); i++)
            {
                const auto &setting = *categorySettings[i];
                int y = startY + 2 + static_cast<int>(i);
                
                moveCursor(startX, y);
                
                if (static_cast<int>(i) == selectedSetting_)
                {
                    setColor(Color::BrightBlue);
                    std::cout << "► ";
                }
                else
                {
                    setColor(Color::White);
                    std::cout << "  ";
                }
                
                // Setting name
                setColor(getSettingColor(setting));
                std::string displayName = setting.name;
                if (displayName.length() > static_cast<size_t>(maxWidth - 20))
                {
                    displayName = displayName.substr(0, maxWidth - 23) + "...";
                }
                std::cout << std::left << std::setw(maxWidth - 15) << displayName;
                
                // Current value
                setColor(setting.modified ? Color::BrightYellow : Color::BrightGreen);
                std::string value = formatSettingValue(setting);
                if (value.length() > 12)
                {
                    value = value.substr(0, 9) + "...";
                }
                std::cout << " [" << value << "]";
            }
        }

        void SettingsScreen::renderSettingDetails(const RenderContext &renderContext)
        {
            auto categorySettings = getCurrentCategorySettings();
            if (selectedSetting_ >= static_cast<int>(categorySettings.size()) || selectedSetting_ < 0)
                return;

            const auto &setting = *categorySettings[selectedSetting_];
            int startY = renderContext.terminalHeight - 10;
            int centerX = renderContext.terminalWidth / 2;

            // Details box
            moveCursor(centerX - 40, startY);
            setColor(Color::BrightCyan);
            std::cout << "╭──────────────────────────────────────────────────────────────────────────────╮";
            
            moveCursor(centerX - 40, startY + 1);
            std::cout << "│                              Setting Details                                 │";
            
            moveCursor(centerX - 40, startY + 2);
            std::cout << "├──────────────────────────────────────────────────────────────────────────────┤";

            // Setting name
            moveCursor(centerX - 40, startY + 3);
            std::cout << "│ Name: ";
            setColor(Color::BrightWhite);
            std::cout << std::left << std::setw(70) << setting.name;
            setColor(Color::BrightCyan);
            std::cout << "│";

            // Description
            moveCursor(centerX - 40, startY + 4);
            std::cout << "│ Description: ";
            setColor(Color::White);
            std::string desc = setting.description;
            if (desc.length() > 63)
            {
                desc = desc.substr(0, 60) + "...";
            }
            std::cout << std::left << std::setw(63) << desc;
            setColor(Color::BrightCyan);
            std::cout << "│";

            // Current value
            moveCursor(centerX - 40, startY + 5);
            std::cout << "│ Current: ";
            setColor(inEditMode_ ? Color::BrightYellow : (setting.modified ? Color::BrightYellow : Color::BrightGreen));
            std::string currentVal = inEditMode_ ? editBuffer_ + "_" : formatSettingValue(setting);
            std::cout << std::left << std::setw(67) << currentVal;
            setColor(Color::BrightCyan);
            std::cout << "│";

            // Default value
            moveCursor(centerX - 40, startY + 6);
            std::cout << "│ Default: ";
            setColor(Color::DarkGray);
            std::cout << std::left << std::setw(67) << setting.defaultValue;
            setColor(Color::BrightCyan);
            std::cout << "│";

            moveCursor(centerX - 40, startY + 7);
            std::cout << "╰──────────────────────────────────────────────────────────────────────────────╯";
        }

        void SettingsScreen::editCurrentSetting()
        {
            auto categorySettings = getCurrentCategorySettings();
            if (selectedSetting_ >= static_cast<int>(categorySettings.size()) || selectedSetting_ < 0)
                return;

            const auto &setting = *categorySettings[selectedSetting_];
            
            if (setting.type == SettingType::Boolean)
            {
                // Toggle boolean values
                std::string newValue = (setting.currentValue == "true") ? "false" : "true";
                applySettingChange(newValue);
            }
            else if (setting.type == SettingType::Selection)
            {
                // Cycle through selection options
                auto it = std::find(setting.options.begin(), setting.options.end(), setting.currentValue);
                if (it != setting.options.end())
                {
                    auto nextIt = std::next(it);
                    if (nextIt != setting.options.end())
                    {
                        applySettingChange(*nextIt);
                    }
                    else if (!setting.options.empty())
                    {
                        applySettingChange(setting.options[0]); // Wrap to first option
                    }
                }
            }
            else
            {
                // Enter edit mode for other types
                inEditMode_ = true;
                editBuffer_ = setting.currentValue;
                markForRefresh();
            }
        }

        void SettingsScreen::applySettingChange(const std::string &newValue)
        {
            auto categorySettings = getCurrentCategorySettings();
            if (selectedSetting_ >= static_cast<int>(categorySettings.size()) || selectedSetting_ < 0)
                return;

            auto &setting = *categorySettings[selectedSetting_];
            
            if (!validateSettingValue(setting, newValue))
            {
                logWarning("Invalid setting value: " + newValue);
                return;
            }
            
            setting.currentValue = newValue;
            setting.modified = (newValue != setting.defaultValue);
            settingsModified_ = true;
            
            logInfo("Changed setting '" + setting.name + "' to: " + newValue);
            markForRefresh();
        }

        std::vector<SettingOption*> SettingsScreen::getCurrentCategorySettings()
        {
            std::vector<SettingOption*> result;
            
            if (selectedCategory_ >= 0 && selectedCategory_ < static_cast<int>(categories_.size()))
            {
                const std::string &category = categories_[selectedCategory_];
                
                for (auto &setting : settings_)
                {
                    if (setting.category == category)
                    {
                        result.push_back(&setting);
                    }
                }
            }
            
            return result;
        }

        std::string SettingsScreen::formatSettingValue(const SettingOption &setting) const
        {
            return setting.currentValue;
        }

        Color SettingsScreen::getSettingColor(const SettingOption &setting) const
        {
            if (setting.modified)
                return Color::BrightYellow;
            else
                return Color::BrightGreen;
        }

        bool SettingsScreen::validateSettingValue(const SettingOption &setting, const std::string &value) const
        {
            switch (setting.type)
            {
            case SettingType::Boolean:
                return (value == "true" || value == "false");
                
            case SettingType::Integer:
                try
                {
                    int intValue = std::stoi(value);
                    return (intValue >= setting.minValue && intValue <= setting.maxValue);
                }
                catch (...)
                {
                    return false;
                }
                
            case SettingType::Float:
                try
                {
                    float floatValue = std::stof(value);
                    return (floatValue >= setting.minValue && floatValue <= setting.maxValue);
                }
                catch (...)
                {
                    return false;
                }
                
            case SettingType::Selection:
                return std::find(setting.options.begin(), setting.options.end(), value) != setting.options.end();
                
            case SettingType::String:
                return !value.empty(); // Basic validation - non-empty string
                
            default:
                return false;
            }
        }
    }
}