#pragma once

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "screen_base.h"
#include "radio_state.h"
#include "log_manager.h"

namespace ELRS
{
    class ElrsTransmitter;
    class TelemetryHandler;
    class MspCommands;

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

            void setTransmitter(std::shared_ptr<ElrsTransmitter> transmitter);
            void enableAutoLinkStats(bool enable);

            // Screen navigation
            void switchToScreen(ScreenType screenType);
            ScreenType getCurrentScreen() const { return currentScreen_; }

            // Static helper functions
            static std::string getScreenName(ScreenType screenType);

        private:
            struct ConfigOption
            {
                std::string name;
                std::string description;
                std::vector<std::string> values;
                int currentIndex = 0;
                bool editable = false;
                std::function<bool(int newIndex, int previousIndex)> onChange;
            };

            struct RxTestResult
            {
                std::string name;
                bool passed = false;
                std::string detail;
            };

            struct ExportOption
            {
                std::string name;
                std::string description;
                bool selected = false;
                std::function<bool(const std::filesystem::path &)> exporter;
            };

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
            Element createSparkline(const std::vector<int> &values) const;

            // Data access helpers
            std::string getDeviceStatus();
            std::string getConnectionInfo();
            std::vector<std::string> getRecentLogs(int maxEntries = 20);
            std::string formatVoltage(double voltage) const;
            std::string formatCurrent(double current) const;
            std::string formatTemperature(int temperature) const;
            std::string formatTimePoint(const std::chrono::steady_clock::time_point &tp) const;

            // Setup helpers
            void setupTelemetry();
            void teardownTelemetry();
            void rebuildConfigOptions();
            void syncConfigOptionsFromTelemetry();
            void handleConfigAdjustment(int direction);
            void applyConfigSelection(int newIndex);
            bool sendTxPowerAdjustment(int direction);
            void updateConfigStatus(const std::string &message, LogLevel level = LogLevel::Info);

            Element buildTelemetryGrid();

            void startTxTest(const std::string &testName);
            void stopTxTest(bool userRequested = true);
            void txTestWorker(const std::string &testName);
            void txTestContinuousWave();
            void txTestModulatedSignal();
            void txTestPowerSweep();

            void runRxDiagnostics();

            void beginBinding();
            void cancelBinding();
            void updateBindingState();

            void startFirmwareUpdate();
            void completeFirmwareUpdate(bool success);

            bool exportTelemetryCSV(const std::filesystem::path &directory);
            bool exportConfigurationJSON(const std::filesystem::path &directory);
            bool exportLogsTXT(const std::filesystem::path &directory);
            bool exportTestReportXML(const std::filesystem::path &directory);

            void applySettings();
            void startAutoLinkStatsThread();
            void stopAutoLinkStatsThread();
            void startRefreshThread();
            void stopRefreshThread();

            // Navigation helpers
            bool handleGlobalKey(Event event);
            Component createScreenContainer(Component content, const std::string &title);

            // Member variables
            ScreenInteractive screen_;
            Component mainContainer_;
            Component currentComponent_;
            ScreenType currentScreen_;

            bool running_;
            bool initialized_;

            std::chrono::steady_clock::time_point lastUpdate_;
            static constexpr int DEFAULT_UPDATE_INTERVAL_MS = 100;
            int updateIntervalMs_;

            std::map<ScreenType, std::string> screenTitles_;
            std::map<int, ScreenType> functionKeyMapping_;

            std::shared_ptr<ElrsTransmitter> transmitter_;
            TelemetryHandler *telemetryHandler_;
            MspCommands *mspCommands_;

            std::thread refreshThread_;
            std::atomic<bool> refreshThreadRunning_;
            std::thread autoLinkStatsThread_;
            std::atomic<bool> autoLinkStatsRunning_;
            std::atomic<bool> txTestRunning_;
            std::atomic<bool> txTestStopRequested_;
            std::thread txTestThread_;

            std::vector<ConfigOption> configOptions_;
            std::vector<std::string> configOptionLabels_;
            std::vector<int> txPowerLevels_;
            int configSelectedIndex_;
            int configTxPowerIndex_;
            int configModelIndex_;
            std::vector<int> modelIdOptions_;
            std::string configStatusMessage_;

            bool monitorPaused_;
            std::string monitorStatusMessage_;

            std::vector<std::string> txTestNames_;
            int txTestSelectedIndex_;
            std::string txTestStatusMessage_;
            std::string txTestActiveName_;
            bool transmitterWasRunningBeforeTest_;

            std::vector<RxTestResult> rxTestResults_;
            bool rxTestInProgress_;
            std::string rxTestStatusMessage_;

            bool bindingActive_;
            std::chrono::steady_clock::time_point bindStartTime_;
            std::string bindStatusMessage_;

            bool updateInProgress_;
            double updateProgress_;
            std::string updateStatusMessage_;
            std::atomic<bool> firmwareUpdateThreadRunning_;
            std::thread firmwareUpdateThread_;

            std::vector<ExportOption> exportOptions_;
            std::string exportStatusMessage_;

            std::vector<int> refreshRateOptions_;
            int settingsRefreshRateOptionIndex_;
            int settingsLogLevelIndex_;
            bool autoLinkStatsEnabled_;
            bool telemetryActive_;
            mutable std::mutex stateMutex_;
        };

    } // namespace UI
} // namespace ELRS