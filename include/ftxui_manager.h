#pragma once

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <map>

#include "screen_base.h"
#include "radio_state.h"
#include "log_manager.h"

namespace ELRS
{
    namespace UI
    {

        using namespace ftxui;

        class FTXUIManager
        {
        public:
            FTXUIManager();
            ~FTXUIManager();

            bool initialize();
            void run();
            void shutdown();

            // Screen navigation
            void switchToScreen(ScreenType screenType);
            ScreenType getCurrentScreen() const { return currentScreen_; }

        private:
            // Screen creation methods
            Component createMainScreen();
            Component createGraphsScreen();
            Component createConfigScreen();
            Component createMonitorScreen();
            Component createTxTestScreen();
            Component createRxTestScreen();
            Component createBindScreen();
            Component createUpdateScreen();
            Component createLogScreen();
            Component createExportScreen();
            Component createSettingsScreen();

            // Common UI elements
            Element createHeader();
            Element createFooter();
            Element createDeviceInfo();
            Element createConnectionStats();

            // Data access helpers
            std::string getDeviceStatus();
            std::string getConnectionInfo();
            std::vector<std::string> getRecentLogs(int maxEntries = 20);

            // Navigation helpers
            void handleGlobalKey(Event event);
            Component createScreenContainer(Component content, const std::string &title);

        public:
            // Static helper functions
            static std::string getScreenName(ScreenType screenType);

        private:
            // Member variables
            ScreenInteractive screen_;
            Component mainContainer_;
            Component currentComponent_;
            ScreenType currentScreen_;

            // State
            bool running_;
            bool initialized_;

            // UI refresh timer
            std::chrono::steady_clock::time_point lastUpdate_;
            static constexpr int UPDATE_INTERVAL_MS = 100;

            // Screen navigation function keys
            std::map<ScreenType, std::string> screenTitles_;
            std::map<int, ScreenType> functionKeyMapping_; // F1-F12 -> ScreenType mapping
        };

    } // namespace UI
} // namespace ELRS