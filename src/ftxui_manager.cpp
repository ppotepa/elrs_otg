#include "ftxui_manager.h"
#include "log_manager.h"
#include "radio_state.h"
#include <ftxui/component/event.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>

using namespace ftxui;

namespace ELRS
{
    namespace UI
    {

        FTXUIManager::FTXUIManager()
            : screen_(ScreenInteractive::Fullscreen()), currentScreen_(ScreenType::Main), running_(false), initialized_(false), lastUpdate_(std::chrono::steady_clock::now())
        {
            // Initialize screen titles
            screenTitles_[ScreenType::Main] = "Device Information";
            screenTitles_[ScreenType::Graphs] = "Signal Graphs";
            screenTitles_[ScreenType::Config] = "Configuration";
            screenTitles_[ScreenType::Monitor] = "Live Monitor";
            screenTitles_[ScreenType::TxTest] = "TX Test";
            screenTitles_[ScreenType::RxTest] = "RX Test";
            screenTitles_[ScreenType::Bind] = "Bind Mode";
            screenTitles_[ScreenType::Update] = "Firmware Update";
            screenTitles_[ScreenType::Logs] = "System Logs";
            screenTitles_[ScreenType::Export] = "Data Export";
            screenTitles_[ScreenType::Settings] = "Settings";

            // Initialize F-key mappings
            functionKeyMapping_[1] = ScreenType::Main;      // F1
            functionKeyMapping_[2] = ScreenType::Graphs;    // F2
            functionKeyMapping_[3] = ScreenType::Config;    // F3
            functionKeyMapping_[4] = ScreenType::Monitor;   // F4
            functionKeyMapping_[5] = ScreenType::TxTest;    // F5
            functionKeyMapping_[6] = ScreenType::RxTest;    // F6
            functionKeyMapping_[7] = ScreenType::Bind;      // F7
            functionKeyMapping_[8] = ScreenType::Update;    // F8
            functionKeyMapping_[9] = ScreenType::Logs;      // F9
            functionKeyMapping_[10] = ScreenType::Export;   // F10
            functionKeyMapping_[11] = ScreenType::Settings; // F11
        }

        FTXUIManager::~FTXUIManager()
        {
            shutdown();
        }

        bool FTXUIManager::initialize()
        {
            if (initialized_)
            {
                return true;
            }

            LogManager::getInstance().info("FTXUI_MGR", "Initializing FTXUI manager");

            // Create initial screen component
            switchToScreen(ScreenType::Main);

            initialized_ = true;
            LogManager::getInstance().info("FTXUI_MGR", "FTXUI manager initialized successfully");
            return true;
        }

        void FTXUIManager::run()
        {
            if (!initialized_)
            {
                LogManager::getInstance().error("FTXUI_MGR", "Manager not initialized");
                return;
            }

            LogManager::getInstance().info("FTXUI_MGR", "Starting FTXUI main loop");
            running_ = true;

            // Create main container and attach current screen
            mainContainer_ = Container::Vertical({});
            if (currentComponent_)
            {
                mainContainer_->Add(currentComponent_);
            }

            // Add global key handler
            auto mainComponent = CatchEvent(mainContainer_, [this](Event event)
                                            {
        // Handle F-key navigation
        if (event == Event::F1) {
            switchToScreen(ScreenType::Main);
            return true;
        }
        if (event == Event::F2) {
            switchToScreen(ScreenType::Graphs);
            return true;
        }
        if (event == Event::F3) {
            switchToScreen(ScreenType::Config);
            return true;
        }
        if (event == Event::F4) {
            switchToScreen(ScreenType::Monitor);
            return true;
        }
        if (event == Event::F5) {
            switchToScreen(ScreenType::TxTest);
            return true;
        }
        if (event == Event::F6) {
            switchToScreen(ScreenType::RxTest);
            return true;
        }
        if (event == Event::F7) {
            switchToScreen(ScreenType::Bind);
            return true;
        }
        if (event == Event::F8) {
            switchToScreen(ScreenType::Update);
            return true;
        }
        if (event == Event::F9) {
            switchToScreen(ScreenType::Logs);
            return true;
        }
        if (event == Event::F10) {
            switchToScreen(ScreenType::Export);
            return true;
        }
        if (event == Event::F11) {
            switchToScreen(ScreenType::Settings);
            return true;
        }
        if (event == Event::F12) {
            running_ = false;
            screen_.Exit();
            return true;
        }
        
        // Handle Escape key
        if (event == Event::Escape) {
            if (currentScreen_ != ScreenType::Main) {
                switchToScreen(ScreenType::Main);
            } else {
                running_ = false;
                screen_.Exit();
            }
            return true;
        }
        
        return false; });

            screen_.Loop(mainComponent);

            LogManager::getInstance().info("FTXUI_MGR", "FTXUI main loop ended");
        }

        void FTXUIManager::shutdown()
        {
            if (!initialized_)
            {
                return;
            }

            LogManager::getInstance().info("FTXUI_MGR", "Shutting down FTXUI manager");
            running_ = false;

            if (screen_.Active())
            {
                screen_.Exit();
            }

            initialized_ = false;
            LogManager::getInstance().info("FTXUI_MGR", "FTXUI manager shutdown complete");
        }

        void FTXUIManager::switchToScreen(ScreenType screenType)
        {
            currentScreen_ = screenType;

            switch (screenType)
            {
            case ScreenType::Main:
                currentComponent_ = createMainScreen();
                break;
            case ScreenType::Graphs:
                currentComponent_ = createGraphsScreen();
                break;
            case ScreenType::Config:
                currentComponent_ = createConfigScreen();
                break;
            case ScreenType::Monitor:
                currentComponent_ = createMonitorScreen();
                break;
            case ScreenType::TxTest:
                currentComponent_ = createTxTestScreen();
                break;
            case ScreenType::RxTest:
                currentComponent_ = createRxTestScreen();
                break;
            case ScreenType::Bind:
                currentComponent_ = createBindScreen();
                break;
            case ScreenType::Update:
                currentComponent_ = createUpdateScreen();
                break;
            case ScreenType::Logs:
                currentComponent_ = createLogScreen();
                break;
            case ScreenType::Export:
                currentComponent_ = createExportScreen();
                break;
            case ScreenType::Settings:
                currentComponent_ = createSettingsScreen();
                break;
            }

            if (mainContainer_)
            {
                mainContainer_->DetachAllChildren();
                if (currentComponent_)
                {
                    mainContainer_->Add(currentComponent_);
                }
            }

            LogManager::getInstance().info("FTXUI_MGR", "Switched to screen: " + screenTitles_[screenType]);
        }

        Component FTXUIManager::createMainScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         hbox({
                                             vbox({
                                                 createDeviceInfo(),
                                                 filler(),
                                             }) | flex_grow,
                                             separator(),
                                             vbox({
                                                 createConnectionStats(),
                                                 filler(),
                                             }) | flex_grow,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createGraphsScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("Signal Graphs") | center | bold,
                                             separator(),
                                             hbox({
                                                 vbox({
                                                     text("TX Power (dBm)"),
                                                     text("                    "),
                                                     text("  |                 "),
                                                     text("  |     /\\          "),
                                                     text("  |    /  \\         "),
                                                     text("  |___/____\\____    "),
                                                     text("                    "),
                                                 }) | border,
                                                 separator(),
                                                 vbox({
                                                     text("RX Signal (dBm)"),
                                                     text("                    "),
                                                     text("  |                 "),
                                                     text("  |  /\\   /\\        "),
                                                     text("  | /  \\_/  \\       "),
                                                     text("  |/        \\___    "),
                                                     text("                    "),
                                                 }) | border,
                                             }),
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createConfigScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("Device Configuration") | center | bold,
                                             separator(),
                                             hbox({
                                                 text("Frequency Band: ") | size(WIDTH, EQUAL, 20),
                                                 text("2.4 GHz") | bold,
                                             }),
                                             hbox({
                                                 text("TX Power: ") | size(WIDTH, EQUAL, 20),
                                                 text("100 mW (20 dBm)") | bold,
                                             }),
                                             hbox({
                                                 text("Packet Rate: ") | size(WIDTH, EQUAL, 20),
                                                 text("500 Hz") | bold,
                                             }),
                                             hbox({
                                                 text("Telemetry Ratio: ") | size(WIDTH, EQUAL, 20),
                                                 text("1:2") | bold,
                                             }),
                                             separator(),
                                             text("Use arrow keys to navigate, Enter to edit") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createMonitorScreen()
        {
            return Renderer([this]
                            {
        auto now = std::chrono::steady_clock::now();
        auto time_str = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count() % 86400);
        
        return vbox({
            createHeader(),
            separator(),
            vbox({
                text("Live Telemetry Monitor") | center | bold,
                separator(),
                hbox({
                    vbox({
                        text("Packets RX: 1234"),
                        text("Packets TX: 5678"),
                        text("Link Quality: 95%"),
                        text("Signal: -85 dBm"),
                        text("SNR: 12 dB"),
                    }) | border,
                    separator(),
                    vbox({
                        text("TX Power: 20 dBm"),
                        text("Temperature: 45°C"),
                        text("Voltage: 3.3V"),
                        text("Current: 120 mA"),
                        text("Uptime: 1h 23m"),
                    }) | border,
                }),
                separator(),
                text("Press SPACE to pause/resume | ESC to return") | dim,
            }) | flex_grow,
            separator(),
            createFooter(),
        }) | border; });
        }

        Component FTXUIManager::createTxTestScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("TX Test Suite") | center | bold,
                                             separator(),
                                             text("Select test to run:"),
                                             text(""),
                                             text("1. Continuous Wave Test"),
                                             text("2. Modulated Signal Test"),
                                             text("3. Power Sweep Test"),
                                             text("4. Frequency Sweep Test"),
                                             text(""),
                                             text("Use UP/DOWN to select, ENTER to start") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createRxTestScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("RX Test Suite") | center | bold,
                                             separator(),
                                             text("Receiver test results:"),
                                             text(""),
                                             text("✓ Sensitivity Test: PASS (-105 dBm)"),
                                             text("✓ Selectivity Test: PASS"),
                                             text("✓ Image Rejection: PASS (>60 dB)"),
                                             text("⏳ Dynamic Range: RUNNING..."),
                                             text(""),
                                             text("Press ENTER to run next test") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createBindScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("Bind Mode") | center | bold,
                                             separator(),
                                             text("Instructions:"),
                                             text("1. Put receiver in bind mode"),
                                             text("2. Press ENTER to start binding"),
                                             text("3. Wait for bind confirmation"),
                                             text(""),
                                             text("Status: Ready to bind") | color(ftxui::Color::Yellow),
                                             text(""),
                                             text("Press ENTER to start binding") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createUpdateScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("Firmware Update") | center | bold,
                                             separator(),
                                             text("Current Version: 3.0.1"),
                                             text("Available Version: 3.0.2"),
                                             text(""),
                                             text("Changelog:"),
                                             text("• Improved packet processing"),
                                             text("• Fixed telemetry issues"),
                                             text("• Better error handling"),
                                             text(""),
                                             text("Press ENTER to start update") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createLogScreen()
        {
            return Renderer([this]
                            {
        auto logs = getRecentLogs(15);
        
        Elements logElements;
        for (const auto& log : logs) {
            logElements.push_back(text(log));
        }
        
        return vbox({
            createHeader(),
            separator(),
            vbox({
                text("System Logs") | center | bold,
                separator(),
                vbox(logElements) | flex_grow | frame,
                separator(),
                text("UP/DOWN to scroll | ESC to return") | dim,
            }) | flex_grow,
            separator(),
            createFooter(),
        }) | border; });
        }

        Component FTXUIManager::createExportScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("Data Export") | center | bold,
                                             separator(),
                                             text("Export Options:"),
                                             text(""),
                                             text("□ Telemetry Data (CSV)"),
                                             text("□ Configuration (JSON)"),
                                             text("□ Log Files (TXT)"),
                                             text("□ Test Results (XML)"),
                                             text(""),
                                             text("Export Path: ./exports/"),
                                             text(""),
                                             text("Use SPACE to toggle options, ENTER to export") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Component FTXUIManager::createSettingsScreen()
        {
            return Renderer([this]
                            { return vbox({
                                         createHeader(),
                                         separator(),
                                         vbox({
                                             text("Application Settings") | center | bold,
                                             separator(),
                                             text("UI Settings:"),
                                             text("• Refresh Rate: 10 Hz"),
                                             text("• Color Theme: Default"),
                                             text("• Log Level: INFO"),
                                             text(""),
                                             text("Connection Settings:"),
                                             text("• Auto-connect: Enabled"),
                                             text("• Timeout: 5 seconds"),
                                             text("• Retry Count: 3"),
                                             text(""),
                                             text("Use arrow keys to navigate") | dim,
                                         }) | flex_grow,
                                         separator(),
                                         createFooter(),
                                     }) |
                                     border; });
        }

        Element FTXUIManager::createHeader()
        {
            return vbox({
                text("ELRS OTG Monitor v1.0 - ExpressLRS 2.4GHz Radio Interface") | center | bold,
                text(screenTitles_[currentScreen_]) | center,
            });
        }

        Element FTXUIManager::createFooter()
        {
            return vbox({
                text("F1:Device F2:Graphs F3:Config F4:Monitor F5:TX Test F6:RX Test F7:Bind F8:Update F9:Log F10:Export F11:Settings F12:Exit") | center | dim,
                text(getConnectionInfo()) | center,
            });
        }

        Element FTXUIManager::createDeviceInfo()
        {
            return vbox({
                       text("Device Information") | bold,
                       separator(),
                       hbox({text("Product: "), text("CP2102 USB to UART Bridge Controller")}),
                       hbox({text("Manufacturer: "), text("Generic ESP32")}),
                       hbox({text("Serial: "), text("REAL0")}),
                       hbox({text("VID:PID: "), text("10C4:EA60")}),
                       hbox({text("Status: "), text("Connected") | color(ftxui::Color::Green)}),
                   }) |
                   border;
        }

        Element FTXUIManager::createConnectionStats()
        {
            return vbox({
                       text("Connection Statistics") | bold,
                       separator(),
                       hbox({text("Packets RX: "), text("1234")}),
                       hbox({text("Packets TX: "), text("5678")}),
                       hbox({text("Link Quality: "), text("95%")}),
                       hbox({text("Signal: "), text("-85 dBm")}),
                       hbox({text("Last Update: "), text("08:30:15")}),
                   }) |
                   border;
        }

        std::string FTXUIManager::getConnectionInfo()
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);

            char buffer[32];
            std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);

            return "Connected | Link Quality: 95% | " + std::string(buffer);
        }

        std::vector<std::string> FTXUIManager::getRecentLogs(int maxEntries)
        {
            auto logs = LogManager::getInstance().getRecentLogs(maxEntries);
            std::vector<std::string> result;

            for (const auto &log : logs)
            {
                result.push_back(log.getFormattedTime() + " [" + log.getLevelString() + "] [" + log.category + "] " + log.message);
            }

            return result;
        }

        std::string FTXUIManager::getScreenName(ScreenType screenType)
        {
            static std::map<ScreenType, std::string> names = {
                {ScreenType::Main, "Main"},
                {ScreenType::Graphs, "Graphs"},
                {ScreenType::Config, "Config"},
                {ScreenType::Monitor, "Monitor"},
                {ScreenType::TxTest, "TxTest"},
                {ScreenType::RxTest, "RxTest"},
                {ScreenType::Bind, "Bind"},
                {ScreenType::Update, "Update"},
                {ScreenType::Logs, "Logs"},
                {ScreenType::Export, "Export"},
                {ScreenType::Settings, "Settings"},
            };

            auto it = names.find(screenType);
            return (it != names.end()) ? it->second : "Unknown";
        }

    } // namespace UI
} // namespace ELRS