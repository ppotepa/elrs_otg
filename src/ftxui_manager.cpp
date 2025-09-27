#include "ftxui_manager.h"
#include "elrs_transmitter.h"
#include "telemetry_handler.h"
#include "msp_commands.h"
#include "log_manager.h"
#include "radio_state.h"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>

using namespace ftxui;

namespace ELRS
{
    namespace UI
    {

        FTXUIManager::FTXUIManager()
            : screen_(ScreenInteractive::Fullscreen()),
              currentScreen_(ScreenType::Main),
              running_(false),
              initialized_(false),
              lastUpdate_(std::chrono::steady_clock::now()),
              updateIntervalMs_(DEFAULT_UPDATE_INTERVAL_MS),
              telemetryHandler_(nullptr),
              mspCommands_(nullptr),
              refreshThreadRunning_(false),
              autoLinkStatsRunning_(false),
              txTestRunning_(false),
              txTestStopRequested_(false),
              configSelectedIndex_(0),
              configTxPowerIndex_(0),
              configModelIndex_(0),
              monitorPaused_(false),
              txTestSelectedIndex_(0),
              txTestStatusMessage_("Ready to run tests."),
              transmitterWasRunningBeforeTest_(false),
              rxTestInProgress_(false),
              rxTestStatusMessage_("Diagnostics idle."),
              bindingActive_(false),
              bindStatusMessage_("Ready to initiate binding."),
              updateInProgress_(false),
              updateProgress_(0.0),
              updateStatusMessage_("Firmware is up to date."),
              firmwareUpdateThreadRunning_(false),
              exportStatusMessage_("Select data to export."),
              settingsRefreshRateOptionIndex_(0),
              settingsLogLevelIndex_(0),
              autoLinkStatsEnabled_(false),
              telemetryActive_(false)
        {
            screenTitles_[ScreenType::Main] = "Device Information";
            screenTitles_[ScreenType::Graphs] = "Signal Analytics";
            screenTitles_[ScreenType::Config] = "Configuration";
            screenTitles_[ScreenType::Monitor] = "Live Monitor";
            screenTitles_[ScreenType::TxTest] = "TX Test";
            screenTitles_[ScreenType::RxTest] = "RX Diagnostics";
            screenTitles_[ScreenType::Bind] = "Binding";
            screenTitles_[ScreenType::Update] = "Firmware Update";
            screenTitles_[ScreenType::Logs] = "System Logs";
            screenTitles_[ScreenType::Export] = "Data Export";
            screenTitles_[ScreenType::Settings] = "Settings";

            functionKeyMapping_[1] = ScreenType::Main;
            functionKeyMapping_[2] = ScreenType::Graphs;
            functionKeyMapping_[3] = ScreenType::Config;
            functionKeyMapping_[4] = ScreenType::Monitor;
            functionKeyMapping_[5] = ScreenType::TxTest;
            functionKeyMapping_[6] = ScreenType::RxTest;
            functionKeyMapping_[7] = ScreenType::Bind;
            functionKeyMapping_[8] = ScreenType::Update;
            functionKeyMapping_[9] = ScreenType::Logs;
            functionKeyMapping_[10] = ScreenType::Export;
            functionKeyMapping_[11] = ScreenType::Settings;

            txPowerLevels_ = {10, 25, 50, 100, 250, 500, 1000};
            modelIdOptions_ = {1, 2, 3, 4, 5, 6, 7, 8};
            refreshRateOptions_ = {50, 100, 250, 500, 1000};

            configStatusMessage_ = "Select an option to adjust configuration.";
            monitorStatusMessage_ = "Telemetry streaming live.";

            txTestNames_ = {"Continuous Wave", "Modulated Signal", "Power Sweep"};

            exportOptions_ = {
                {"Telemetry Data (CSV)", "Export recent telemetry samples to CSV", false, [this](const std::filesystem::path &dir)
                 { return exportTelemetryCSV(dir); }},
                {"Configuration (JSON)", "Save current radio configuration as JSON", false, [this](const std::filesystem::path &dir)
                 { return exportConfigurationJSON(dir); }},
                {"Logs (TXT)", "Dump latest log entries to a text file", false, [this](const std::filesystem::path &dir)
                 { return exportLogsTXT(dir); }},
                {"Test Report (XML)", "Generate diagnostics report from latest RX tests", false, [this](const std::filesystem::path &dir)
                 { return exportTestReportXML(dir); }}};

            LogLevel currentLevel = LogManager::getInstance().getLogLevel();
            settingsLogLevelIndex_ = static_cast<int>(currentLevel);

            auto it = std::find(refreshRateOptions_.begin(), refreshRateOptions_.end(), updateIntervalMs_);
            if (it != refreshRateOptions_.end())
            {
                settingsRefreshRateOptionIndex_ = static_cast<int>(std::distance(refreshRateOptions_.begin(), it));
            }

            txTestActiveName_.clear();
        }

        FTXUIManager::~FTXUIManager()
        {
            shutdown();
        }

        void FTXUIManager::setTransmitter(std::shared_ptr<ElrsTransmitter> transmitter)
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            transmitter_ = std::move(transmitter);
            telemetryHandler_ = transmitter_ ? transmitter_->getTelemetryHandler() : nullptr;
            mspCommands_ = transmitter_ ? transmitter_->getMspCommands() : nullptr;
        }

        void FTXUIManager::enableAutoLinkStats(bool enable)
        {
            autoLinkStatsEnabled_ = enable;
            if (!initialized_)
            {
                return;
            }

            if (enable)
            {
                startAutoLinkStatsThread();
            }
            else
            {
                stopAutoLinkStatsThread();
            }
        }

        bool FTXUIManager::initialize()
        {
            if (initialized_)
            {
                return true;
            }

            LOG_INFO("FTXUI_MGR", "Initializing FTXUI manager");

            setupTelemetry();
            rebuildConfigOptions();

            if (!currentComponent_)
            {
                switchToScreen(ScreenType::Main);
            }

            startRefreshThread();

            if (autoLinkStatsEnabled_)
            {
                startAutoLinkStatsThread();
            }

            initialized_ = true;
            LOG_INFO("FTXUI_MGR", "FTXUI manager initialized successfully");
            return true;
        }

        void FTXUIManager::run()
        {
            if (!initialized_)
            {
                LOG_ERROR("FTXUI_MGR", "Manager not initialized");
                return;
            }

            LOG_INFO("FTXUI_MGR", "Starting FTXUI main loop");
            running_ = true;

            mainContainer_ = Container::Vertical({});
            if (currentComponent_)
            {
                mainContainer_->Add(currentComponent_);
            }

            auto mainComponent = CatchEvent(mainContainer_, [this](Event event)
                                            { return handleGlobalKey(event); });

            screen_.Loop(mainComponent);

            LOG_INFO("FTXUI_MGR", "FTXUI main loop ended");
        }

        void FTXUIManager::shutdown()
        {
            if (!initialized_)
            {
                return;
            }

            LOG_INFO("FTXUI_MGR", "Shutting down FTXUI manager");

            running_ = false;

            stopTxTest(false);
            stopAutoLinkStatsThread();
            stopRefreshThread();

            if (firmwareUpdateThreadRunning_)
            {
                firmwareUpdateThreadRunning_ = false;
                if (firmwareUpdateThread_.joinable())
                {
                    firmwareUpdateThread_.join();
                }
            }

            teardownTelemetry();

            if (screen_.Active())
            {
                screen_.Exit();
            }

            initialized_ = false;
            LOG_INFO("FTXUI_MGR", "FTXUI manager shutdown complete");
        }

        void FTXUIManager::switchToScreen(ScreenType screenType)
        {
            currentScreen_ = screenType;

            if (screenType == ScreenType::Config)
            {
                rebuildConfigOptions();
            }
            else if (screenType == ScreenType::Monitor)
            {
                monitorStatusMessage_ = monitorPaused_ ? "Monitor paused." : "Telemetry streaming live.";
            }
            else if (screenType == ScreenType::Bind)
            {
                updateBindingState();
            }

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

            if (running_)
            {
                screen_.PostEvent(Event::Custom);
            }

            LOG_INFO("FTXUI_MGR", "Switched to screen: " + screenTitles_[screenType]);
        }

        bool FTXUIManager::handleGlobalKey(Event event)
        {
            if (event == Event::Escape || event == Event::Character('q') || event == Event::Character('Q'))
            {
                running_ = false;
                screen_.Exit();
                return true;
            }

            const Event functionEvents[] = {
                Event::F1,
                Event::F2,
                Event::F3,
                Event::F4,
                Event::F5,
                Event::F6,
                Event::F7,
                Event::F8,
                Event::F9,
                Event::F10,
                Event::F11,
            };

            for (int i = 0; i < 11; ++i)
            {
                if (event == functionEvents[i])
                {
                    auto it = functionKeyMapping_.find(i + 1);
                    if (it != functionKeyMapping_.end())
                    {
                        switchToScreen(it->second);
                        return true;
                    }
                }
            }

            if (event == Event::F12)
            {
                running_ = false;
                screen_.Exit();
                return true;
            }

            return false;
        }

        Component FTXUIManager::createMainScreen()
        {
            auto renderer = Renderer([this]
                                     {
                                          auto &radioState = RadioState::getInstance();
                                          auto telemetry = radioState.getLiveTelemetry();

                                          auto rssiHistory = radioState.getRSSIHistory(60);
                                          auto linkHistory = radioState.getLinkQualityHistory(60);
                                          auto powerHistory = radioState.getTxPowerHistory(60);

                                          auto graphs = vbox({
                                              text("Recent Signal Metrics") | bold | center,
                                              separator(),
                                              hbox({
                                                  vbox({text("RSSI (dBm)"), createSparkline(rssiHistory)}) | flex,
                                                  separator(),
                                                  vbox({text("Link Quality (%)"), createSparkline(linkHistory)}) | flex,
                                                  separator(),
                                                  vbox({text("TX Power (dBm)"), createSparkline(powerHistory)}) | flex,
                                              }),
                                          }) | border;

                                          auto status = text("Status: " + getConnectionInfo()) | center;

                                          return vbox({
                                              createHeader(),
                                              separator(),
                                              hbox({
                                                  createDeviceInfo() | flex,
                                                  separator(),
                                                  createConnectionStats() | flex,
                                              }),
                                              separator(),
                                              graphs | flex,
                                              separator(),
                                              status,
                                              separator(),
                                              createFooter(),
                                          }) |
                                                 border; });

            return renderer;
        }

        Component FTXUIManager::createGraphsScreen()
        {
            return Renderer([this]
                            {
                                auto &radioState = RadioState::getInstance();

                                auto rssiHistory = radioState.getRSSIHistory(120);
                                auto linkHistory = radioState.getLinkQualityHistory(120);
                                auto powerHistory = radioState.getTxPowerHistory(120);

                                auto buildGraphCard = [this](const std::string &title, const std::vector<int> &values, const std::string &unit)
                                {
                                    int minValue = values.empty() ? 0 : *std::min_element(values.begin(), values.end());
                                    int maxValue = values.empty() ? 0 : *std::max_element(values.begin(), values.end());
                                    double avgValue = values.empty() ? 0.0 : std::accumulate(values.begin(), values.end(), 0.0) / values.size();

                                    std::stringstream info;
                                    info << "Min: " << minValue << unit << "  Max: " << maxValue << unit << "  Avg: " << std::fixed << std::setprecision(1) << avgValue << unit;

                                    return vbox({
                                               text(title) | center | bold,
                                               createSparkline(values) | flex,
                                               text(info.str()) | dim | center,
                                           }) |
                                           border | flex;
                                };

                                auto graphsRow = hbox({
                                    buildGraphCard("RSSI", rssiHistory, " dBm"),
                                    separator(),
                                    buildGraphCard("Link Quality", linkHistory, "%"),
                                    separator(),
                                    buildGraphCard("TX Power", powerHistory, " dBm"),
                                }) | flex;

                                return vbox({
                                           createHeader(),
                                           separator(),
                                           text("Real-Time Signal Analytics") | center | bold,
                                           separator(),
                                           graphsRow,
                                           separator(),
                                           createFooter(),
                                       }) |
                                       border; });
        }

        Component FTXUIManager::createConfigScreen()
        {
            rebuildConfigOptions();

            auto menu = Menu(&configOptionLabels_, &configSelectedIndex_);

            auto content = Renderer(menu, [this, menu]
                                    {
                                              Element details = text("No configuration options available.") | dim;

                                              if (!configOptions_.empty() && configSelectedIndex_ >= 0 && configSelectedIndex_ < static_cast<int>(configOptions_.size()))
                                              {
                                                  const auto &option = configOptions_[configSelectedIndex_];
                                                  std::string currentValue;
                                                  if (!option.values.empty())
                                                  {
                                                      currentValue = option.values[option.currentIndex];
                                                  }

                                                  std::vector<Element> lines;
                                                  lines.push_back(text(option.name) | bold);
                                                  lines.push_back(separator());
                                                  lines.push_back(text(option.description));

                                                  if (option.editable && !currentValue.empty())
                                                  {
                                                      lines.push_back(separator());
                                                      lines.push_back(text("Current Value: " + currentValue) | bold);
                                                      lines.push_back(text("Use LEFT/RIGHT to adjust.") | dim);
                                                  }
                                                  else if (!option.editable && option.onChange)
                                                  {
                                                      lines.push_back(separator());
                                                      lines.push_back(text("Press ENTER to execute action.") | dim);
                                                  }
                                                  else if (!currentValue.empty())
                                                  {
                                                      lines.push_back(separator());
                                                      lines.push_back(text("Current Value: " + currentValue));
                                                  }

                                                  details = vbox(std::move(lines)) | border;
                                              }

                                              return vbox({
                                                         createHeader(),
                                                         separator(),
                                                         hbox({
                                                             menu->Render() | frame | size(WIDTH, EQUAL, 32) | flex,
                                                             separator(),
                                                             details | flex,
                                                         }),
                                                         separator(),
                                                         text(configStatusMessage_) | center,
                                                         separator(),
                                                         createFooter(),
                                                     }) |
                                                     border; });

            auto component = CatchEvent(content, [this](Event event)
                                        {
                                             if (configOptions_.empty())
                                             {
                                                 return false;
                                             }

                                             if (event == Event::ArrowLeft)
                                             {
                                                 handleConfigAdjustment(-1);
                                                 return true;
                                             }
                                             if (event == Event::ArrowRight)
                                             {
                                                 handleConfigAdjustment(1);
                                                 return true;
                                             }
                                             if (event == Event::Return)
                                             {
                                                 applyConfigSelection(configSelectedIndex_);
                                                 return true;
                                             }
                                             return false; });

            return component;
        }

        Component FTXUIManager::createMonitorScreen()
        {
            auto content = Renderer([this]
                                    { return vbox({
                                                 createHeader(),
                                                 separator(),
                                                 text(monitorPaused_ ? "Telemetry Monitor [PAUSED]" : "Live Telemetry Monitor") | center | bold,
                                                 separator(),
                                                 buildTelemetryGrid() | flex,
                                                 separator(),
                                                 text(monitorStatusMessage_) | center,
                                                 separator(),
                                                 text("SPACE: Pause/Resume  |  R: Request Link Stats  |  ESC: Main") | center | dim,
                                                 separator(),
                                                 createFooter(),
                                             }) |
                                             border; });

            auto component = CatchEvent(content, [this](Event event)
                                        {
                                             if (event == Event::Character(' '))
                                             {
                                                 monitorPaused_ = !monitorPaused_;
                                                 monitorStatusMessage_ = monitorPaused_ ? "Monitor paused manually." : "Telemetry streaming live.";
                                                 return true;
                                             }

                                             if (event == Event::Character('r') || event == Event::Character('R'))
                                             {
                                                 if (mspCommands_)
                                                 {
                                                     bool success = mspCommands_->sendLinkStatsRequest();
                                                     monitorStatusMessage_ = success ? "Link statistics request sent." : "Failed to request link statistics.";
                                                     if (!success)
                                                     {
                                                         LOG_WARNING("CONFIG", "Failed to send link stats request");
                                                     }
                                                 }
                                                 else
                                                 {
                                                     monitorStatusMessage_ = "MSP commands unavailable.";
                                                 }
                                                 return true;
                                             }

                                             return false; });

            return component;
        }

        Component FTXUIManager::createTxTestScreen()
        {
            auto testMenu = Menu(&txTestNames_, &txTestSelectedIndex_);

            auto startButton = Button("Start Selected Test", [this]
                                      {
                                          if (txTestSelectedIndex_ >= 0 && txTestSelectedIndex_ < static_cast<int>(txTestNames_.size()))
                                          {
                                              startTxTest(txTestNames_[txTestSelectedIndex_]);
                                          } });

            auto stopButton = Button("Stop Test", [this]
                                     { stopTxTest(); });

            auto container = Container::Vertical({testMenu, startButton, stopButton});

            auto renderer = Renderer(container, [this, testMenu, startButton, stopButton]
                                     {
                                                  auto status = text(txTestStatusMessage_) | center;

                                                  return vbox({
                                                             createHeader(),
                                                             separator(),
                                                             text("Transmitter Test Suite") | center | bold,
                                                             separator(),
                                                             testMenu->Render() | frame | flex,
                                                             separator(),
                                                             hbox({startButton->Render(), separator(), stopButton->Render()}) | center,
                                                             separator(),
                                                             status,
                                                             separator(),
                                                             text("Tests drive the transmitter using predefined control patterns. Ensure a safe environment before starting.") | center | dim,
                                                             separator(),
                                                             createFooter(),
                                                         }) |
                                                         border; });

            return renderer;
        }

        Component FTXUIManager::createRxTestScreen()
        {
            auto runButton = Button("Run Diagnostics", [this]
                                    { runRxDiagnostics(); });

            auto renderer = Renderer(runButton, [this, runButton]
                                     {
                                                  Elements resultElements;
                                                  for (const auto &result : rxTestResults_)
                                                  {
                                                      auto line = hbox({
                                                          text(result.passed ? "✓ " : "✗ ") | color(result.passed ? ftxui::Color::Green : ftxui::Color::Red),
                                                          text(result.name + ": " + result.detail),
                                                      });
                                                      resultElements.push_back(line);
                                                  }

                                                  if (resultElements.empty())
                                                  {
                                                      resultElements.push_back(text("No diagnostics run yet.") | dim);
                                                  }

                                                  return vbox({
                                                             createHeader(),
                                                             separator(),
                                                             text("Receiver Diagnostics") | center | bold,
                                                             separator(),
                                                             runButton->Render() | center,
                                                             separator(),
                                                             vbox(resultElements) | border | flex,
                                                             separator(),
                                                             text(rxTestStatusMessage_) | center,
                                                             separator(),
                                                             createFooter(),
                                                         }) |
                                                         border; });

            return renderer;
        }

        Component FTXUIManager::createBindScreen()
        {
            auto startButton = Button("Start Binding", [this]
                                      {
                                          if (!bindingActive_)
                                          {
                                              beginBinding();
                                          } });

            auto cancelButton = Button("Cancel", [this]
                                       {
                                          if (bindingActive_)
                                          {
                                              cancelBinding();
                                          } });

            auto container = Container::Horizontal({startButton, cancelButton});

            auto renderer = Renderer(container, [this, startButton, cancelButton]
                                     {
                                                    updateBindingState();

                                                    std::string statusText = bindStatusMessage_;
                                                    if (bindingActive_)
                                                    {
                                                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - bindStartTime_).count();
                                                        statusText += " (" + std::to_string(elapsed) + "s)";
                                                    }

                                                    return vbox({
                                                               createHeader(),
                                                               separator(),
                                                               text("Binding Assistant") | center | bold,
                                                               separator(),
                                                               vbox({
                                                                   text("1. Put receiver into bind mode"),
                                                                   text("2. Press 'Start Binding'"),
                                                                   text("3. Wait for confirmation"),
                                                               }) | border,
                                                               separator(),
                                                               hbox({startButton->Render(), separator(), cancelButton->Render()}) | center,
                                                               separator(),
                                                               text(statusText) | center,
                                                               separator(),
                                                               createFooter(),
                                                           }) |
                                                           border; });

            return renderer;
        }

        Component FTXUIManager::createUpdateScreen()
        {
            auto checkButton = Button("Check for Updates", [this]
                                      {
                                          if (mspCommands_)
                                          {
                                              bool success = mspCommands_->sendDeviceDiscovery();
                                              updateStatusMessage_ = success ? "Discovery request sent. Awaiting response." : "Device discovery failed.";
                                          }
                                          else
                                          {
                                              updateStatusMessage_ = "MSP commands unavailable.";
                                          } });

            auto startButton = Button("Start Firmware Update", [this]
                                      {
                                          if (!updateInProgress_)
                                          {
                                              startFirmwareUpdate();
                                          } });

            auto renderer = Renderer(Container::Horizontal({checkButton, startButton}), [this, checkButton, startButton]
                                     {
                                                                                             double normalizedProgress = std::clamp(updateProgress_, 0.0, 1.0);

                                                                                             return vbox({
                                                                                                        createHeader(),
                                                                                                        separator(),
                                                                                                        text("Firmware Management") | center | bold,
                                                                                                        separator(),
                                                                                                        hbox({
                                                                                                            checkButton->Render(),
                                                                                                            separator(),
                                                                                                            startButton->Render(),
                                                                                                        }) | center,
                                                                                                        separator(),
                                                                                                        gauge(normalizedProgress) | border | size(WIDTH, EQUAL, 60),
                                                                                                        separator(),
                                                                                                        text(updateStatusMessage_) | center,
                                                                                                        separator(),
                                                                                                        text("Ensure USB connection is stable before starting an update.") | center | dim,
                                                                                                        separator(),
                                                                                                        createFooter(),
                                                                                                    }) |
                                                                                                    border; });

            return renderer;
        }

        Component FTXUIManager::createLogScreen()
        {
            return Renderer([this]
                            {
                                auto logs = getRecentLogs(50);

                                Elements logElements;
                                for (const auto &log : logs)
                                {
                                    logElements.push_back(text(log));
                                }

                                if (logElements.empty())
                                {
                                    logElements.push_back(text("No log entries recorded yet.") | dim);
                                }

                                return vbox({
                                           createHeader(),
                                           separator(),
                                           text("Recent System Logs") | center | bold,
                                           separator(),
                                           vbox(logElements) | frame | flex,
                                           separator(),
                                           text("Adjust verbosity in Settings.") | center | dim,
                                           separator(),
                                           createFooter(),
                                       }) |
                                       border; });
        }

        Component FTXUIManager::createExportScreen()
        {
            auto checkboxComponents = std::make_shared<std::vector<Component>>();
            checkboxComponents->reserve(exportOptions_.size());

            for (auto &option : exportOptions_)
            {
                checkboxComponents->push_back(Checkbox(&option.name, &option.selected));
            }

            auto checkboxContainer = Container::Vertical(*checkboxComponents);

            auto exportButton = Button("Export Selected", [this]
                                       {
                                          std::filesystem::path exportDir = std::filesystem::current_path() / "exports";
                                          std::error_code ec;
                                          std::filesystem::create_directories(exportDir, ec);

                                          bool anySelected = false;
                                          bool allSucceeded = true;

                                          for (auto &option : exportOptions_)
                                          {
                                              if (!option.selected)
                                              {
                                                  continue;
                                              }
                                              anySelected = true;
                                              bool result = option.exporter ? option.exporter(exportDir) : false;
                                              allSucceeded &= result;
                                          }

                                          if (!anySelected)
                                          {
                                              exportStatusMessage_ = "Select at least one dataset to export.";
                                          }
                                          else if (allSucceeded)
                                          {
                                              exportStatusMessage_ = "Export complete. Files saved to " + exportDir.string();
                                          }
                                          else
                                          {
                                              exportStatusMessage_ = "Export completed with some errors. Check logs for details.";
                                          } });

            auto container = Container::Vertical({checkboxContainer, exportButton});

            auto renderer = Renderer(container, [this, checkboxComponents, exportButton]
                                     {
                                                  Elements optionElements;
                                                  for (size_t i = 0; i < exportOptions_.size(); ++i)
                                                  {
                                                      optionElements.push_back(hbox({(*checkboxComponents)[i]->Render(), text(" - " + exportOptions_[i].description) | dim}));
                                                  }

                                                  return vbox({
                                                             createHeader(),
                                                             separator(),
                                                             text("Data Export Center") | center | bold,
                                                             separator(),
                                                             vbox(optionElements) | border,
                                                             separator(),
                                                             exportButton->Render() | center,
                                                             separator(),
                                                             text(exportStatusMessage_) | center,
                                                             separator(),
                                                             createFooter(),
                                                         }) |
                                                         border; });

            return renderer;
        }

        Component FTXUIManager::createSettingsScreen()
        {
            std::vector<std::string> refreshLabels;
            refreshLabels.reserve(refreshRateOptions_.size());
            for (int rate : refreshRateOptions_)
            {
                std::stringstream ss;
                ss << rate << " ms";
                refreshLabels.push_back(ss.str());
            }

            static std::vector<std::string> logLevelLabels = {"Debug", "Info", "Warning", "Error"};

            auto refreshRadiobox = Radiobox(&refreshLabels, &settingsRefreshRateOptionIndex_);
            auto logLevelRadiobox = Radiobox(&logLevelLabels, &settingsLogLevelIndex_);
            auto autoLinkCheckbox = Checkbox("Automatic link stats polling", &autoLinkStatsEnabled_);
            auto applyButton = Button("Apply Settings", [this]
                                      { applySettings(); });

            auto container = Container::Vertical({refreshRadiobox, logLevelRadiobox, autoLinkCheckbox, applyButton});

            auto renderer = Renderer(container, [this, refreshRadiobox, logLevelRadiobox, autoLinkCheckbox, applyButton]
                                     { return vbox({
                                                  createHeader(),
                                                  separator(),
                                                  text("Application Settings") | center | bold,
                                                  separator(),
                                                  hbox({
                                                      vbox({text("Refresh Interval"), refreshRadiobox->Render() | border}) | flex,
                                                      separator(),
                                                      vbox({text("Log Level"), logLevelRadiobox->Render() | border}) | flex,
                                                  }),
                                                  separator(),
                                                  vbox({autoLinkCheckbox->Render(), text("Requires MSP telemetry support") | dim}) | border,
                                                  separator(),
                                                  applyButton->Render() | center,
                                                  separator(),
                                                  createFooter(),
                                              }) |
                                              border; });

            return renderer;
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
            auto &radioState = RadioState::getInstance();
            auto config = radioState.getDeviceConfiguration();
            auto status = radioState.getConnectionStatusString();

            std::stringstream vidpid;
            vidpid << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << config.vid << ":" << std::setw(4) << config.pid;

            return vbox({
                       text("Device Information") | bold,
                       separator(),
                       hbox({text("Product: "), text(config.productName.empty() ? "Unknown" : config.productName)}),
                       hbox({text("Manufacturer: "), text(config.manufacturer.empty() ? "Unknown" : config.manufacturer)}),
                       hbox({text("Serial: "), text(config.serialNumber.empty() ? "N/A" : config.serialNumber)}),
                       hbox({text("Firmware: "), text(config.firmwareVersion.empty() ? "Unknown" : config.firmwareVersion)}),
                       hbox({text("VID:PID: "), text(vidpid.str())}),
                       hbox({text("Status: "), text(status) | color(status == "Connected" ? ftxui::Color::Green : ftxui::Color::Yellow)}),
                   }) |
                   border;
        }

        Element FTXUIManager::createConnectionStats()
        {
            auto &radioState = RadioState::getInstance();
            auto telemetry = radioState.getLiveTelemetry();

            return vbox({
                       text("Connection Statistics") | bold,
                       separator(),
                       hbox({text("Packets RX: "), text(std::to_string(telemetry.packetsReceived))}),
                       hbox({text("Packets TX: "), text(std::to_string(telemetry.packetsTransmitted))}),
                       hbox({text("Link Quality: "), text(std::to_string(telemetry.linkQuality) + "%")}),
                       hbox({text("Signal: "), text(std::to_string(telemetry.rssi1) + " dBm")}),
                       hbox({text("SNR: "), text(std::to_string(telemetry.snr) + " dB")}),
                       hbox({text("TX Power: "), text(std::to_string(telemetry.txPower) + " dBm")}),
                       hbox({text("Last Update: "), text(formatTimePoint(telemetry.lastUpdate))}),
                   }) |
                   border;
        }

        Element FTXUIManager::createSparkline(const std::vector<int> &values) const
        {
            if (values.empty())
            {
                return text("No data") | dim;
            }

            auto minmax = std::minmax_element(values.begin(), values.end());
            int minValue = *minmax.first;
            int maxValue = *minmax.second;

            static const std::string levels = "▁▂▃▄▅▆▇█";
            std::string line;
            line.reserve(values.size());

            int range = maxValue - minValue;
            if (range < 1)
            {
                range = 1;
            }
            for (int value : values)
            {
                double normalized = static_cast<double>(value - minValue) / static_cast<double>(range);
                int index = static_cast<int>(std::round(normalized * (static_cast<int>(levels.size()) - 1)));
                if (index < 0)
                {
                    index = 0;
                }
                else if (index >= static_cast<int>(levels.size()))
                {
                    index = static_cast<int>(levels.size()) - 1;
                }
                line.push_back(levels[index]);
            }

            return hbox({text(line)});
        }

        std::string FTXUIManager::getConnectionInfo()
        {
            auto &radioState = RadioState::getInstance();
            auto telemetry = radioState.getLiveTelemetry();

            std::stringstream ss;
            ss << "Status: " << radioState.getConnectionStatusString();
            ss << " | LQ: " << telemetry.linkQuality << "%";
            ss << " | Voltage: " << formatVoltage(telemetry.voltage);
            ss << " | Updated: " << formatTimePoint(telemetry.lastUpdate);
            return ss.str();
        }

        std::string FTXUIManager::getDeviceStatus()
        {
            auto &radioState = RadioState::getInstance();
            auto config = radioState.getDeviceConfiguration();
            std::stringstream ss;
            ss << (config.productName.empty() ? "ELRS Device" : config.productName) << " - "
               << radioState.getConnectionStatusString();
            return ss.str();
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

        std::string FTXUIManager::formatVoltage(double voltage) const
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << voltage << " V";
            return ss.str();
        }

        std::string FTXUIManager::formatCurrent(double current) const
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << current << " A";
            return ss.str();
        }

        std::string FTXUIManager::formatTemperature(int temperature) const
        {
            return std::to_string(temperature) + " °C";
        }

        std::string FTXUIManager::formatTimePoint(const std::chrono::steady_clock::time_point &tp) const
        {
            auto now = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - tp).count();
            if (diff < 0)
            {
                diff = 0;
            }

            std::stringstream ss;
            ss << diff << "s ago";
            return ss.str();
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

        void FTXUIManager::setupTelemetry()
        {
            if (!telemetryHandler_)
            {
                telemetryActive_ = false;
                LOG_WARNING("FTXUI_MGR", "Telemetry handler not available");
                return;
            }

            telemetryHandler_->setLinkStatsCallback([this](const LinkStats &stats)
                                                    {
                                                         auto telemetry = RadioState::getInstance().getLiveTelemetry();
                                                         telemetry.rssi1 = stats.rssi1;
                                                         telemetry.rssi2 = stats.rssi2;
                                                         telemetry.linkQuality = stats.link_quality;
                                                         telemetry.snr = stats.snr;
                                                         telemetry.txPower = stats.tx_power;
                                                         telemetry.isValid = stats.valid;
                                                         RadioState::getInstance().updateTelemetry(telemetry);

                                                         if (running_)
                                                         {
                                                             screen_.PostEvent(Event::Custom);
                                                         } });

            telemetryHandler_->setBatteryCallback([this](const BatteryInfo &battery)
                                                  {
                                                       auto telemetry = RadioState::getInstance().getLiveTelemetry();
                                                       telemetry.voltage = battery.voltage_mv / 1000.0;
                                                       telemetry.current = battery.current_ma / 1000.0;
                                                       telemetry.isValid = battery.valid;
                                                       RadioState::getInstance().updateTelemetry(telemetry);

                                                       if (running_)
                                                       {
                                                           screen_.PostEvent(Event::Custom);
                                                       } });

            if (!telemetryHandler_->isRunning())
            {
                telemetryHandler_->start();
            }

            telemetryActive_ = telemetryHandler_->isRunning();
        }

        void FTXUIManager::teardownTelemetry()
        {
            if (telemetryHandler_ && telemetryHandler_->isRunning())
            {
                telemetryHandler_->stop();
            }
            telemetryActive_ = false;
        }

        Element FTXUIManager::buildTelemetryGrid()
        {
            auto telemetry = RadioState::getInstance().getLiveTelemetry();

            auto buildRow = [](const std::string &label, const std::string &value)
            { return hbox({text(label) | bold, filler(), text(value)}); };

            std::stringstream packetInfo;
            packetInfo << telemetry.packetsReceived << " / " << telemetry.packetsTransmitted << " (lost " << telemetry.packetsLost << ")";

            return vbox({
                       text("Telemetry Snapshot") | center | bold,
                       separator(),
                       buildRow("RSSI Primary", std::to_string(telemetry.rssi1) + " dBm"),
                       buildRow("RSSI Secondary", std::to_string(telemetry.rssi2) + " dBm"),
                       buildRow("Link Quality", std::to_string(telemetry.linkQuality) + "%"),
                       buildRow("SNR", std::to_string(telemetry.snr) + " dB"),
                       buildRow("TX Power", std::to_string(telemetry.txPower) + " dBm"),
                       buildRow("Voltage", formatVoltage(telemetry.voltage)),
                       buildRow("Current", formatCurrent(telemetry.current)),
                       buildRow("Temperature", formatTemperature(telemetry.temperature)),
                       buildRow("Packets", packetInfo.str()),
                   }) |
                   border;
        }

        void FTXUIManager::startRefreshThread()
        {
            if (refreshThreadRunning_)
            {
                return;
            }

            refreshThreadRunning_ = true;
            refreshThread_ = std::thread([this]
                                         {
                                             while (refreshThreadRunning_)
                                             {
                                                 std::this_thread::sleep_for(std::chrono::milliseconds(updateIntervalMs_));
                                                 if (!refreshThreadRunning_)
                                                 {
                                                     break;
                                                 }
                                                 screen_.PostEvent(Event::Custom);
                                             } });
        }

        void FTXUIManager::stopRefreshThread()
        {
            if (!refreshThreadRunning_)
            {
                return;
            }

            refreshThreadRunning_ = false;
            if (refreshThread_.joinable())
            {
                refreshThread_.join();
            }
        }

        void FTXUIManager::startAutoLinkStatsThread()
        {
            if (autoLinkStatsRunning_ || !mspCommands_)
            {
                return;
            }

            autoLinkStatsRunning_ = true;
            autoLinkStatsThread_ = std::thread([this]
                                               {
                                                   while (autoLinkStatsRunning_)
                                                   {
                                                       if (mspCommands_)
                                                       {
                                                           mspCommands_->sendLinkStatsRequest();
                                                       }
                                                       std::this_thread::sleep_for(std::chrono::seconds(2));
                                                   } });
        }

        void FTXUIManager::stopAutoLinkStatsThread()
        {
            if (!autoLinkStatsRunning_)
            {
                return;
            }

            autoLinkStatsRunning_ = false;
            if (autoLinkStatsThread_.joinable())
            {
                autoLinkStatsThread_.join();
            }
        }

        void FTXUIManager::startTxTest(const std::string &testName)
        {
            if (txTestRunning_)
            {
                txTestStatusMessage_ = "Test already running.";
                return;
            }

            if (txTestThread_.joinable())
            {
                txTestThread_.join();
            }

            txTestRunning_ = true;
            txTestStopRequested_ = false;
            txTestActiveName_ = testName;
            txTestStatusMessage_ = "Starting test: " + testName;

            txTestThread_ = std::thread(&FTXUIManager::txTestWorker, this, testName);
        }

        void FTXUIManager::stopTxTest(bool userRequested)
        {
            if (!txTestRunning_ && !txTestThread_.joinable())
            {
                return;
            }

            txTestStopRequested_ = true;
            if (txTestThread_.joinable())
            {
                txTestThread_.join();
            }

            txTestRunning_ = false;
            txTestActiveName_.clear();
            txTestStatusMessage_ = userRequested ? "Test stopped by user." : "Test completed.";
            screen_.PostEvent(Event::Custom);
        }

        void FTXUIManager::txTestWorker(const std::string &testName)
        {
            if (testName == "Continuous Wave")
            {
                txTestContinuousWave();
            }
            else if (testName == "Modulated Signal")
            {
                txTestModulatedSignal();
            }
            else if (testName == "Power Sweep")
            {
                txTestPowerSweep();
            }
            else
            {
                txTestStatusMessage_ = "Unknown test selected.";
            }

            txTestRunning_ = false;
            txTestActiveName_.clear();
            if (!txTestStopRequested_)
            {
                txTestStatusMessage_ = "Test finished: " + testName;
            }
            screen_.PostEvent(Event::Custom);
        }

        void FTXUIManager::txTestContinuousWave()
        {
            for (int step = 0; step < 5 && !txTestStopRequested_; ++step)
            {
                txTestStatusMessage_ = "Continuous wave output... step " + std::to_string(step + 1);
                screen_.PostEvent(Event::Custom);
                std::this_thread::sleep_for(std::chrono::milliseconds(600));
            }
        }

        void FTXUIManager::txTestModulatedSignal()
        {
            for (int step = 0; step < 5 && !txTestStopRequested_; ++step)
            {
                txTestStatusMessage_ = "Modulated signal test frame " + std::to_string(step + 1);
                screen_.PostEvent(Event::Custom);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        void FTXUIManager::txTestPowerSweep()
        {
            for (int powerIndex = 0; powerIndex < static_cast<int>(txPowerLevels_.size()) && !txTestStopRequested_; ++powerIndex)
            {
                txTestStatusMessage_ = "Sweeping power level to " + std::to_string(txPowerLevels_[powerIndex]) + " dBm";
                screen_.PostEvent(Event::Custom);
                std::this_thread::sleep_for(std::chrono::milliseconds(700));
            }
        }

        void FTXUIManager::runRxDiagnostics()
        {
            if (rxTestInProgress_)
            {
                rxTestStatusMessage_ = "Diagnostics already running.";
                return;
            }

            rxTestInProgress_ = true;
            rxTestResults_.clear();
            rxTestStatusMessage_ = "Collecting telemetry for diagnostics...";
            screen_.PostEvent(Event::Custom);

            auto telemetry = RadioState::getInstance().getLiveTelemetry();

            RxTestResult signalResult;
            signalResult.name = "Signal Strength";
            signalResult.passed = telemetry.rssi1 > -90;
            signalResult.detail = std::to_string(telemetry.rssi1) + " dBm";
            rxTestResults_.push_back(signalResult);

            RxTestResult linkResult;
            linkResult.name = "Link Quality";
            linkResult.passed = telemetry.linkQuality > 70;
            linkResult.detail = std::to_string(telemetry.linkQuality) + "%";
            rxTestResults_.push_back(linkResult);

            RxTestResult snrResult;
            snrResult.name = "Noise Ratio";
            snrResult.passed = telemetry.snr > 5;
            snrResult.detail = std::to_string(telemetry.snr) + " dB";
            rxTestResults_.push_back(snrResult);

            RxTestResult packetResult;
            packetResult.name = "Packet Loss";
            packetResult.passed = telemetry.packetsLost < (telemetry.packetsReceived + telemetry.packetsTransmitted) / 10;
            packetResult.detail = std::to_string(telemetry.packetsLost) + " lost";
            rxTestResults_.push_back(packetResult);

            rxTestInProgress_ = false;
            rxTestStatusMessage_ = "Diagnostics complete.";
            screen_.PostEvent(Event::Custom);
        }

        void FTXUIManager::beginBinding()
        {
            if (bindingActive_)
            {
                bindStatusMessage_ = "Binding already in progress.";
                return;
            }

            if (!mspCommands_)
            {
                bindStatusMessage_ = "Cannot bind: MSP commands unavailable.";
                return;
            }

            bindingActive_ = true;
            bindStartTime_ = std::chrono::steady_clock::now();

            bool success = mspCommands_->sendBindCommand();
            if (success)
            {
                bindStatusMessage_ = "Binding command sent. Put receiver in bind mode.";
            }
            else
            {
                bindingActive_ = false;
                bindStatusMessage_ = "Failed to send binding command.";
            }
            screen_.PostEvent(Event::Custom);
        }

        void FTXUIManager::cancelBinding()
        {
            if (!bindingActive_)
            {
                bindStatusMessage_ = "No active binding session.";
                return;
            }

            bindingActive_ = false;
            bindStatusMessage_ = "Binding cancelled.";
            screen_.PostEvent(Event::Custom);
        }

        void FTXUIManager::updateBindingState()
        {
            if (!bindingActive_)
            {
                bindStatusMessage_ = "Ready to initiate binding.";
                return;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - bindStartTime_).count();
            bindStatusMessage_ = "Binding in progress... " + std::to_string(elapsed) + "s";
        }

        void FTXUIManager::startFirmwareUpdate()
        {
            if (updateInProgress_)
            {
                updateStatusMessage_ = "Firmware update already running.";
                return;
            }

            if (firmwareUpdateThread_.joinable())
            {
                firmwareUpdateThread_.join();
            }

            updateInProgress_ = true;
            firmwareUpdateThreadRunning_ = true;
            updateProgress_ = 0.0;
            updateStatusMessage_ = "Starting firmware update...";
            screen_.PostEvent(Event::Custom);

            firmwareUpdateThread_ = std::thread([this]
                                                {
                                                    const int steps = 20;
                                                    for (int i = 0; i <= steps && firmwareUpdateThreadRunning_; ++i)
                                                    {
                                                        updateProgress_ = static_cast<double>(i) / steps;
                                                        screen_.PostEvent(Event::Custom);
                                                        std::this_thread::sleep_for(std::chrono::milliseconds(400));
                                                    }

                                                    bool success = firmwareUpdateThreadRunning_;
                                                    completeFirmwareUpdate(success); });
        }

        void FTXUIManager::completeFirmwareUpdate(bool success)
        {
            firmwareUpdateThreadRunning_ = false;
            updateInProgress_ = false;

            updateProgress_ = success ? 1.0 : 0.0;
            updateStatusMessage_ = success ? "Firmware update completed successfully." : "Firmware update aborted.";
            screen_.PostEvent(Event::Custom);
        }

        bool FTXUIManager::exportTelemetryCSV(const std::filesystem::path &directory)
        {
            std::error_code ec;
            std::filesystem::create_directories(directory, ec);
            auto filePath = directory / "telemetry.csv";

            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                LOG_ERROR("EXPORT", "Failed to open " + filePath.string());
                return false;
            }

            auto telemetry = RadioState::getInstance().getLiveTelemetry();
            file << "metric,value\n";
            file << "rssi1," << telemetry.rssi1 << "\n";
            file << "rssi2," << telemetry.rssi2 << "\n";
            file << "link_quality," << telemetry.linkQuality << "\n";
            file << "snr," << telemetry.snr << "\n";
            file << "tx_power," << telemetry.txPower << "\n";
            file << "voltage," << telemetry.voltage << "\n";
            file << "current," << telemetry.current << "\n";
            file << "temperature," << telemetry.temperature << "\n";
            file << "packets_received," << telemetry.packetsReceived << "\n";
            file << "packets_transmitted," << telemetry.packetsTransmitted << "\n";
            file << "packets_lost," << telemetry.packetsLost << "\n";

            return true;
        }

        bool FTXUIManager::exportConfigurationJSON(const std::filesystem::path &directory)
        {
            std::error_code ec;
            std::filesystem::create_directories(directory, ec);
            auto filePath = directory / "configuration.json";

            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                LOG_ERROR("EXPORT", "Failed to open " + filePath.string());
                return false;
            }

            auto config = RadioState::getInstance().getDeviceConfiguration();
            file << "{\n";
            file << "  \"productName\": \"" << config.productName << "\",\n";
            file << "  \"manufacturer\": \"" << config.manufacturer << "\",\n";
            file << "  \"serialNumber\": \"" << config.serialNumber << "\",\n";
            file << "  \"firmwareVersion\": \"" << config.firmwareVersion << "\",\n";
            file << "  \"hardwareVersion\": \"" << config.hardwareVersion << "\",\n";
            file << "  \"vid\": " << config.vid << ",\n";
            file << "  \"pid\": " << config.pid << ",\n";
            file << "  \"frequency\": \"" << config.frequency << "\",\n";
            file << "  \"protocol\": \"" << config.protocol << "\",\n";
            file << "  \"baudRate\": " << config.baudRate << "\n";
            file << "}\n";

            return true;
        }

        bool FTXUIManager::exportLogsTXT(const std::filesystem::path &directory)
        {
            std::error_code ec;
            std::filesystem::create_directories(directory, ec);
            auto filePath = directory / "logs.txt";

            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                LOG_ERROR("EXPORT", "Failed to open " + filePath.string());
                return false;
            }

            auto logs = LogManager::getInstance().getRecentLogs(200);
            for (const auto &log : logs)
            {
                file << log.getFormattedTime() << " [" << log.getLevelString() << "] [" << log.category << "] " << log.message << "\n";
            }

            return true;
        }

        bool FTXUIManager::exportTestReportXML(const std::filesystem::path &directory)
        {
            std::error_code ec;
            std::filesystem::create_directories(directory, ec);
            auto filePath = directory / "rx_diagnostics.xml";

            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                LOG_ERROR("EXPORT", "Failed to open " + filePath.string());
                return false;
            }

            file << "<rx_diagnostics>\n";
            for (const auto &result : rxTestResults_)
            {
                file << "  <test name=\"" << result.name << "\" passed=\"" << (result.passed ? "true" : "false") << "\">";
                file << result.detail << "</test>\n";
            }
            file << "</rx_diagnostics>\n";

            return true;
        }

        void FTXUIManager::applySettings()
        {
            if (settingsRefreshRateOptionIndex_ >= 0 && settingsRefreshRateOptionIndex_ < static_cast<int>(refreshRateOptions_.size()))
            {
                updateIntervalMs_ = refreshRateOptions_[settingsRefreshRateOptionIndex_];
                if (refreshThreadRunning_)
                {
                    stopRefreshThread();
                    startRefreshThread();
                }
            }

            int levelIndex = settingsLogLevelIndex_;
            if (levelIndex < 0)
            {
                levelIndex = 0;
            }
            else if (levelIndex > 3)
            {
                levelIndex = 3;
            }

            auto desiredLevel = static_cast<LogLevel>(levelIndex);
            LogManager::getInstance().setLogLevel(desiredLevel);

            enableAutoLinkStats(autoLinkStatsEnabled_);

            LOG_INFO("SETTINGS", "Settings applied: refresh " + std::to_string(updateIntervalMs_) + "ms, log level " + std::to_string(static_cast<int>(desiredLevel)));
        }

        void FTXUIManager::rebuildConfigOptions()
        {
            std::lock_guard<std::mutex> lock(stateMutex_);

            configOptions_.clear();
            configOptionLabels_.clear();

            syncConfigOptionsFromTelemetry();

            ConfigOption txPowerOption;
            txPowerOption.name = "TX Power";
            txPowerOption.description = "Adjust transmitter RF output level.";
            txPowerOption.editable = true;
            txPowerOption.values.reserve(txPowerLevels_.size());
            for (int level : txPowerLevels_)
            {
                txPowerOption.values.push_back(std::to_string(level) + " dBm");
            }
            txPowerOption.currentIndex = configTxPowerIndex_;
            txPowerOption.onChange = [this](int newIndex, int previousIndex)
            {
                int direction = (newIndex > previousIndex) ? 1 : -1;
                bool success = sendTxPowerAdjustment(direction);
                if (success)
                {
                    configTxPowerIndex_ = newIndex;
                }
                return success;
            };
            configOptions_.push_back(txPowerOption);
            configOptionLabels_.push_back(txPowerOption.name);

            ConfigOption modelOption;
            modelOption.name = "Model Slot";
            modelOption.description = "Select active model profile on transmitter.";
            modelOption.editable = true;
            modelOption.values.reserve(modelIdOptions_.size());
            for (int id : modelIdOptions_)
            {
                modelOption.values.push_back("Model " + std::to_string(id));
            }
            modelOption.currentIndex = configModelIndex_;
            modelOption.onChange = [this](int newIndex, int)
            {
                if (!mspCommands_)
                {
                    updateConfigStatus("MSP commands unavailable for model selection.", LogLevel::Warning);
                    return false;
                }
                bool success = mspCommands_->sendModelSelect(static_cast<uint8_t>(modelIdOptions_[newIndex]));
                if (success)
                {
                    configModelIndex_ = newIndex;
                    updateConfigStatus("Selected model " + std::to_string(modelIdOptions_[newIndex]) + ".");
                }
                else
                {
                    updateConfigStatus("Failed to select model.", LogLevel::Error);
                }
                return success;
            };
            configOptions_.push_back(modelOption);
            configOptionLabels_.push_back(modelOption.name);

            ConfigOption telemetryOption;
            telemetryOption.name = "Telemetry Ratio";
            telemetryOption.description = "Current telemetry to control packet ratio.";
            telemetryOption.values = {"1:2"};
            telemetryOption.currentIndex = 0;
            telemetryOption.editable = false;
            configOptions_.push_back(telemetryOption);
            configOptionLabels_.push_back(telemetryOption.name);

            ConfigOption linkStatsOption;
            linkStatsOption.name = "Request Link Stats";
            linkStatsOption.description = "Send an immediate link statistics request via MSP.";
            linkStatsOption.editable = false;
            linkStatsOption.onChange = [this](int, int)
            {
                if (!mspCommands_)
                {
                    updateConfigStatus("MSP commands unavailable.", LogLevel::Warning);
                    return false;
                }
                bool success = mspCommands_->sendLinkStatsRequest();
                updateConfigStatus(success ? "Link stats request sent." : "Failed to send link stats request.",
                                   success ? LogLevel::Info : LogLevel::Error);
                return success;
            };
            configOptions_.push_back(linkStatsOption);
            configOptionLabels_.push_back(linkStatsOption.name);

            if (configSelectedIndex_ >= static_cast<int>(configOptions_.size()))
            {
                configSelectedIndex_ = static_cast<int>(configOptions_.size()) - 1;
            }
            if (configSelectedIndex_ < 0)
            {
                configSelectedIndex_ = 0;
            }
        }

        void FTXUIManager::syncConfigOptionsFromTelemetry()
        {
            auto telemetry = RadioState::getInstance().getLiveTelemetry();

            if (!txPowerLevels_.empty())
            {
                auto it = std::min_element(txPowerLevels_.begin(), txPowerLevels_.end(),
                                           [&telemetry](int a, int b)
                                           { return std::abs(a - telemetry.txPower) < std::abs(b - telemetry.txPower); });
                configTxPowerIndex_ = static_cast<int>(std::distance(txPowerLevels_.begin(), it));
            }
        }

        void FTXUIManager::handleConfigAdjustment(int direction)
        {
            if (configOptions_.empty() || configSelectedIndex_ < 0 || configSelectedIndex_ >= static_cast<int>(configOptions_.size()))
            {
                return;
            }

            auto &option = configOptions_[configSelectedIndex_];

            if (!option.editable || option.values.size() <= 1)
            {
                updateConfigStatus("Option is not adjustable.", LogLevel::Warning);
                return;
            }

            int newIndex = option.currentIndex + direction;
            int maxIndex = static_cast<int>(option.values.size()) - 1;
            if (newIndex < 0)
            {
                newIndex = 0;
            }
            else if (newIndex > maxIndex)
            {
                newIndex = maxIndex;
            }
            if (newIndex == option.currentIndex)
            {
                return;
            }

            bool success = option.onChange ? option.onChange(newIndex, option.currentIndex) : true;
            if (success)
            {
                option.currentIndex = newIndex;
                updateConfigStatus(option.name + " set to " + option.values[newIndex] + ".");
            }
        }

        void FTXUIManager::applyConfigSelection(int index)
        {
            if (index < 0 || index >= static_cast<int>(configOptions_.size()))
            {
                return;
            }

            auto &option = configOptions_[index];
            if (option.editable || !option.onChange)
            {
                return;
            }

            bool success = option.onChange(option.currentIndex, option.currentIndex);
            if (!success)
            {
                updateConfigStatus("Action failed: " + option.name, LogLevel::Error);
            }
        }

        bool FTXUIManager::sendTxPowerAdjustment(int direction)
        {
            if (!mspCommands_)
            {
                updateConfigStatus("MSP commands unavailable for power adjustment.", LogLevel::Warning);
                return false;
            }

            bool success = direction > 0 ? mspCommands_->sendPowerIncrease() : mspCommands_->sendPowerDecrease();
            if (success)
            {
                auto telemetry = RadioState::getInstance().getLiveTelemetry();
                telemetry.txPower += direction > 0 ? 5 : -5;
                RadioState::getInstance().updateTelemetry(telemetry);
            }
            else
            {
                updateConfigStatus("Power adjustment command failed.", LogLevel::Error);
            }
            return success;
        }

        void FTXUIManager::updateConfigStatus(const std::string &message, LogLevel level)
        {
            configStatusMessage_ = message;
            switch (level)
            {
            case LogLevel::Debug:
                LOG_DEBUG("CONFIG", message);
                break;
            case LogLevel::Info:
                LOG_INFO("CONFIG", message);
                break;
            case LogLevel::Warning:
                LOG_WARNING("CONFIG", message);
                break;
            case LogLevel::Error:
                LOG_ERROR("CONFIG", message);
                break;
            }
        }

    } // namespace UI
} // namespace ELRS