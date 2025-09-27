#include "screens/main_screen.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace ELRS
{
    namespace UI
    {
        // LiveGraph Implementation
        LiveGraph::LiveGraph(const std::string &title, int maxPoints)
            : title_(title), maxPoints_(maxPoints), maxValue_(100.0), minValue_(0.0), autoScale_(true)
        {
        }

        void LiveGraph::addDataPoint(double value, Color color)
        {
            GraphData point;
            point.timestamp = std::chrono::steady_clock::now();
            point.value = value;
            point.color = color;

            dataPoints_.push_back(point);

            // Remove old points
            while (dataPoints_.size() > static_cast<size_t>(maxPoints_))
            {
                dataPoints_.pop_front();
            }

            if (autoScale_)
            {
                updateScale();
            }
        }

        void LiveGraph::updateScale()
        {
            if (dataPoints_.empty())
                return;

            auto minmax = std::minmax_element(dataPoints_.begin(), dataPoints_.end(),
                                              [](const GraphData &a, const GraphData &b)
                                              { return a.value < b.value; });

            minValue_ = minmax.first->value - 1.0;
            maxValue_ = minmax.second->value + 1.0;

            // Ensure we have some range
            if (std::abs(maxValue_ - minValue_) < 0.1)
            {
                maxValue_ = minValue_ + 10.0;
            }
        }

        void LiveGraph::render(int x, int y, int width, int height)
        {
            // This will be called by the main screen's renderSimpleGraph method
            // Implementation moved to MainScreen for now
        }

        // MainScreen Implementation
        MainScreen::MainScreen()
            : ScreenBase(ScreenType::Main, "Main"), lastGraphUpdate_(std::chrono::steady_clock::now())
        {
        }

        bool MainScreen::initialize(const RenderContext &renderContext,
                                    const NavigationContext &navContext)
        {
            logInfo("Initializing main screen");

            // Initialize graphs
            txGraph_ = std::make_unique<LiveGraph>("TX Power (dBm)", 200);
            rxGraph_ = std::make_unique<LiveGraph>("RX Signal (dBm)", 200);
            linkQualityGraph_ = std::make_unique<LiveGraph>("Link Quality (%)", 200);

            // Set appropriate ranges
            txGraph_->setMinValue(-30.0);
            txGraph_->setMaxValue(30.0);

            rxGraph_->setMinValue(-120.0);
            rxGraph_->setMaxValue(-30.0);

            linkQualityGraph_->setMinValue(0.0);
            linkQualityGraph_->setMaxValue(100.0);

            return true;
        }

        void MainScreen::onActivate()
        {
            logInfo("Main screen activated");
            markForRefresh();
        }

        void MainScreen::onDeactivate()
        {
            logDebug("Main screen deactivated");
        }

        void MainScreen::update(std::chrono::milliseconds deltaTime)
        {
            // Check if telemetry data has changed since last update
            const auto &radioState = getRadioState();
            const auto &telemetry = radioState.getLiveTelemetry();

            if (telemetry.lastUpdate > lastGraphUpdate_)
            {
                updateGraphData();
                markForRefresh();
                lastGraphUpdate_ = telemetry.lastUpdate;
            }
        }

        void MainScreen::render(const RenderContext &renderContext)
        {
            clearScreen();

            // Render header
            printCentered(0, "ELRS OTG Monitor v1.0 - ExpressLRS 2.4GHz Radio Interface", Color::BrightWhite);

            // Render main content
            renderDeviceInfo(renderContext);
            renderConnectionStats(renderContext);
            renderGraphs(renderContext);

            // Render footer with function keys
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);
            std::cout << "F1:Device F2:Graphs F3:Config F4:Monitor F5:TX Test F6:RX Test F7:Bind F8:Update F9:Log F10:Export F11:Settings F12:Exit";

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(Color::BrightGreen);

            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();
            auto deviceConfig = radioState.getDeviceConfiguration();

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);

            std::stringstream statusBar;
            statusBar << " Connected | Link Quality: " << telemetry.linkQuality << "% | "
                      << std::put_time(&tm, "%H:%M:%S");

            std::cout << statusBar.str();

            resetColor();
            std::cout.flush(); // Ensure all output is written
            clearRefreshFlag();
        }

        bool MainScreen::handleKeyPress(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::F1:
                // Already on main screen
                return true;
            case FunctionKey::F2:
                navigateToScreen(ScreenType::Graphs);
                return true;
            case FunctionKey::F3:
                navigateToScreen(ScreenType::Config);
                return true;
            case FunctionKey::F4:
                navigateToScreen(ScreenType::Monitor);
                return true;
            case FunctionKey::F5:
                navigateToScreen(ScreenType::TxTest);
                return true;
            case FunctionKey::F6:
                navigateToScreen(ScreenType::RxTest);
                return true;
            case FunctionKey::F7:
                navigateToScreen(ScreenType::Bind);
                return true;
            case FunctionKey::F8:
                navigateToScreen(ScreenType::Update);
                return true;
            case FunctionKey::F9:
                logInfo("F9 pressed - navigating to log screen");
                navigateToScreen(ScreenType::Logs);
                return true;
            case FunctionKey::F10:
                navigateToScreen(ScreenType::Export);
                return true;
            case FunctionKey::F11:
                navigateToScreen(ScreenType::Settings);
                return true;
            case FunctionKey::F12:
            case FunctionKey::Escape:
                exitApplication();
                return true;
            default:
                return false;
            }
        }

        void MainScreen::cleanup()
        {
            logInfo("Cleaning up main screen");
            txGraph_.reset();
            rxGraph_.reset();
            linkQualityGraph_.reset();
        }

        void MainScreen::renderDeviceInfo(const RenderContext &context)
        {
            auto &radioState = getRadioState();
            auto deviceConfig = radioState.getDeviceConfiguration();

            int startX = 2;
            int startY = 3;
            int boxWidth = 47;
            int boxHeight = 7;

            setColor(Color::White);
            drawBox(startX, startY, boxWidth, boxHeight, "Device Information");

            // Device info content
            moveCursor(startX + 2, startY + 1);
            std::cout << "Product    : " << deviceConfig.productName;

            moveCursor(startX + 2, startY + 2);
            std::cout << "Manufacturer: " << deviceConfig.manufacturer;

            moveCursor(startX + 2, startY + 3);
            std::cout << "Serial     : " << deviceConfig.serialNumber;

            moveCursor(startX + 2, startY + 4);
            std::cout << "VID:PID    : " << deviceConfig.vid << ":" << deviceConfig.pid;

            moveCursor(startX + 2, startY + 5);
            std::cout << "Status     : ";
            setColor(radioState.getConnectionStatus() == ConnectionStatus::Connected ? Color::BrightGreen : Color::BrightRed);
            std::cout << (radioState.getConnectionStatus() == ConnectionStatus::Connected ? "Connected" : "Disconnected");

            resetColor();
        }

        void MainScreen::renderConnectionStats(const RenderContext &context)
        {
            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();

            int startX = 51;
            int startY = 3;
            int boxWidth = 47;
            int boxHeight = 7;

            setColor(Color::White);
            drawBox(startX, startY, boxWidth, boxHeight, "Connection Statistics");

            // Connection stats content
            moveCursor(startX + 2, startY + 1);
            std::cout << "Packets RX : " << std::setw(25) << telemetry.packetsReceived;

            moveCursor(startX + 2, startY + 2);
            std::cout << "Packets TX : " << std::setw(25) << telemetry.packetsTransmitted;

            moveCursor(startX + 2, startY + 3);
            std::cout << "Link Quality: " << std::setw(24) << telemetry.linkQuality << "%";

            moveCursor(startX + 2, startY + 4);
            std::cout << "Signal     : " << std::setw(20) << telemetry.rssi1 << "dBm";

            moveCursor(startX + 2, startY + 5);
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            std::cout << "Last Update: " << std::put_time(&tm, "%H:%M:%S");

            resetColor();
        }

        void MainScreen::renderGraphs(const RenderContext &context)
        {
            int graphY = 11;
            int graphHeight = 8;
            int leftGraphWidth = 56;
            int rightGraphWidth = 56;

            // TX Power graph
            setColor(Color::White);
            drawBox(0, graphY, leftGraphWidth, graphHeight, "TX Power (dBm)");
            renderSimpleGraph(1, graphY + 1, leftGraphWidth - 2, graphHeight - 2, txGraph_.get());

            // RX Signal graph
            drawBox(leftGraphWidth, graphY, rightGraphWidth, graphHeight, "RX Signal (dBm)");
            renderSimpleGraph(leftGraphWidth + 1, graphY + 1, rightGraphWidth - 2, graphHeight - 2, rxGraph_.get());

            resetColor();
        }

        void MainScreen::renderSimpleGraph(int x, int y, int width, int height, LiveGraph *graph)
        {
            if (!graph)
                return;

            auto &radioState = getRadioState();
            std::vector<int> history;
            Color graphColor = Color::White;

            if (graph == txGraph_.get())
            {
                history = radioState.getTxPowerHistory(width);
                graphColor = Color::BrightGreen;
            }
            else if (graph == rxGraph_.get())
            {
                history = radioState.getRSSIHistory(width);
                graphColor = Color::BrightRed;
            }
            else if (graph == linkQualityGraph_.get())
            {
                history = radioState.getLinkQualityHistory(width);
                graphColor = Color::BrightBlue;
            }

            // Render graph area with real data
            for (int row = 0; row < height; ++row)
            {
                moveCursor(x, y + row);

                for (int col = 0; col < width; ++col)
                {
                    bool showPoint = false;

                    if (!history.empty() && col < static_cast<int>(history.size()))
                    {
                        int value = history[col];

                        // Normalize value based on graph type
                        double normalizedValue = 0.0;
                        if (graph == txGraph_.get())
                        {
                            // TX Power: -30 to +30 dBm
                            normalizedValue = (value + 30.0) / 60.0;
                        }
                        else if (graph == rxGraph_.get())
                        {
                            // RX Signal: -120 to -30 dBm
                            normalizedValue = (value + 120.0) / 90.0;
                        }
                        else if (graph == linkQualityGraph_.get())
                        {
                            // Link Quality: 0 to 100%
                            normalizedValue = value / 100.0;
                        }

                        normalizedValue = (std::max)(0.0, (std::min)(1.0, normalizedValue));
                        int targetRow = height - 1 - static_cast<int>(normalizedValue * (height - 1));

                        if (std::abs(row - targetRow) <= 1)
                        {
                            showPoint = true;
                        }
                    }

                    if (showPoint)
                    {
                        setColor(graphColor);
                        std::cout << "*";
                        resetColor();
                    }
                    else
                    {
                        std::cout << " ";
                    }
                }
            }
        }

        void MainScreen::updateGraphData()
        {
            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();

            // Add current data points to graphs
            if (txGraph_)
            {
                txGraph_->addTxDataPoint(telemetry.txPower);
            }

            if (rxGraph_)
            {
                rxGraph_->addRxDataPoint(telemetry.rssi1);
            }

            if (linkQualityGraph_)
            {
                linkQualityGraph_->addDataPoint(telemetry.linkQuality, Color::BrightBlue);
            }
        }

    } // namespace UI
} // namespace ELRS