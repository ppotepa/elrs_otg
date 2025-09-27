#include "screens/graphs_screen.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace ELRS
{
    namespace UI
    {
        GraphsScreen::GraphsScreen()
            : ScreenBase(ScreenType::Graphs, "Graphs"), selectedGraph_(0), lastUpdate_(std::chrono::steady_clock::now())
        {
        }

        bool GraphsScreen::initialize(const RenderContext &renderContext,
                                      const NavigationContext &navContext)
        {
            logInfo("Initializing graphs screen");
            initializeGraphs();
            return true;
        }

        void GraphsScreen::onActivate()
        {
            logInfo("Graphs screen activated");
            markForRefresh();
        }

        void GraphsScreen::onDeactivate()
        {
            logDebug("Graphs screen deactivated");
        }

        void GraphsScreen::update(std::chrono::milliseconds deltaTime)
        {
            // Check if telemetry data has changed since last update
            const auto &radioState = getRadioState();
            const auto &telemetry = radioState.getLiveTelemetry();

            if (telemetry.lastUpdate > lastUpdate_)
            {
                updateGraphData();
                markForRefresh();
                lastUpdate_ = telemetry.lastUpdate;
            }
        }

        void GraphsScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Historical Graphs", Color::BrightWhite);

            // Render graphs
            renderGraphs(renderContext);
            renderGraphDetails(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);
            std::cout << "LEFT/RIGHT: Select Graph | C: Clear Data | ESC/F1: Return | F12: Exit";

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(Color::BrightGreen);

            if (selectedGraph_ >= 0 && selectedGraph_ < static_cast<int>(graphs_.size()))
            {
                const auto &graph = graphs_[selectedGraph_];
                std::cout << " Graphs | Selected: " << graph->title
                          << " | Points: " << graph->values.size() << "/" << MAX_GRAPH_POINTS;
            }

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool GraphsScreen::handleKeyPress(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::ArrowLeft:
                if (selectedGraph_ > 0)
                {
                    selectedGraph_--;
                    markForRefresh();
                }
                return true;
            case FunctionKey::ArrowRight:
                if (selectedGraph_ < static_cast<int>(graphs_.size()) - 1)
                {
                    selectedGraph_++;
                    markForRefresh();
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

        void GraphsScreen::cleanup()
        {
            logInfo("Cleaning up graphs screen");
        }

        void GraphsScreen::initializeGraphs()
        {
            graphs_.clear();

            // RSSI Graph
            auto rssiGraph = std::make_unique<GraphData>();
            rssiGraph->title = "RSSI (dBm)";
            rssiGraph->unit = "dBm";
            rssiGraph->minValue = -120.0;
            rssiGraph->maxValue = -30.0;
            rssiGraph->color = Color::BrightGreen;
            rssiGraph->autoScale = false;
            graphs_.push_back(std::move(rssiGraph));

            // Link Quality Graph
            auto lqGraph = std::make_unique<GraphData>();
            lqGraph->title = "Link Quality (%)";
            lqGraph->unit = "%";
            lqGraph->minValue = 0.0;
            lqGraph->maxValue = 100.0;
            lqGraph->color = Color::BrightBlue;
            lqGraph->autoScale = false;
            graphs_.push_back(std::move(lqGraph));

            // TX Power Graph
            auto txGraph = std::make_unique<GraphData>();
            txGraph->title = "TX Power (dBm)";
            txGraph->unit = "dBm";
            txGraph->minValue = -30.0;
            txGraph->maxValue = 30.0;
            txGraph->color = Color::BrightRed;
            txGraph->autoScale = false;
            graphs_.push_back(std::move(txGraph));

            // Temperature Graph
            auto tempGraph = std::make_unique<GraphData>();
            tempGraph->title = "Temperature (°C)";
            tempGraph->unit = "°C";
            tempGraph->minValue = 0.0;
            tempGraph->maxValue = 100.0;
            tempGraph->color = Color::BrightYellow;
            tempGraph->autoScale = false;
            graphs_.push_back(std::move(tempGraph));

            // SNR Graph
            auto snrGraph = std::make_unique<GraphData>();
            snrGraph->title = "SNR (dB)";
            snrGraph->unit = "dB";
            snrGraph->minValue = -20.0;
            snrGraph->maxValue = 20.0;
            snrGraph->color = Color::BrightMagenta;
            snrGraph->autoScale = false;
            graphs_.push_back(std::move(snrGraph));
        }

        void GraphsScreen::updateGraphData()
        {
            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();

            if (graphs_.size() >= 5)
            {
                // Update RSSI
                graphs_[0]->values.push_back(telemetry.rssi1);
                if (graphs_[0]->values.size() > MAX_GRAPH_POINTS)
                {
                    graphs_[0]->values.pop_front();
                }

                // Update Link Quality
                graphs_[1]->values.push_back(telemetry.linkQuality);
                if (graphs_[1]->values.size() > MAX_GRAPH_POINTS)
                {
                    graphs_[1]->values.pop_front();
                }

                // Update TX Power
                graphs_[2]->values.push_back(telemetry.txPower);
                if (graphs_[2]->values.size() > MAX_GRAPH_POINTS)
                {
                    graphs_[2]->values.pop_front();
                }

                // Update Temperature
                graphs_[3]->values.push_back(telemetry.temperature);
                if (graphs_[3]->values.size() > MAX_GRAPH_POINTS)
                {
                    graphs_[3]->values.pop_front();
                }

                // Update SNR
                graphs_[4]->values.push_back(telemetry.snr);
                if (graphs_[4]->values.size() > MAX_GRAPH_POINTS)
                {
                    graphs_[4]->values.pop_front();
                }
            }
        }

        void GraphsScreen::renderGraphs(const RenderContext &context)
        {
            int startY = 3;
            int graphsPerRow = 2;
            int graphWidth = (context.terminalWidth - 6) / graphsPerRow;
            int graphHeight = 8;

            for (int i = 0; i < static_cast<int>(graphs_.size()); ++i)
            {
                int row = i / graphsPerRow;
                int col = i % graphsPerRow;
                int graphX = 2 + (col * (graphWidth + 2));
                int graphY = startY + (row * (graphHeight + 2));

                const auto &graph = graphs_[i];

                // Highlight selected graph
                Color borderColor = (i == selectedGraph_) ? Color::BrightWhite : Color::White;
                setColor(borderColor);
                drawBox(graphX, graphY, graphWidth, graphHeight, graph->title);

                // Render mini graph
                renderMiniGraph(graphX + 1, graphY + 1, graphWidth - 2, graphHeight - 2, *graph);
            }
        }

        void GraphsScreen::renderGraphDetails(const RenderContext &context)
        {
            if (selectedGraph_ >= 0 && selectedGraph_ < static_cast<int>(graphs_.size()))
            {
                const auto &graph = graphs_[selectedGraph_];
                int detailY = context.terminalHeight - 10;
                int detailWidth = context.terminalWidth - 4;

                setColor(Color::BrightCyan);
                drawBox(2, detailY, detailWidth, 6, "Graph Details");

                moveCursor(4, detailY + 1);
                setColor(Color::BrightWhite);
                std::cout << "Graph: " << graph->title;

                if (!graph->values.empty())
                {
                    double current = graph->values.back();
                    auto minmax = std::minmax_element(graph->values.begin(), graph->values.end());

                    moveCursor(4, detailY + 2);
                    setColor(graph->color);
                    std::cout << "Current: " << std::fixed << std::setprecision(2) << current << " " << graph->unit;

                    moveCursor(4, detailY + 3);
                    setColor(Color::BrightGreen);
                    std::cout << "Min: " << std::fixed << std::setprecision(2) << *minmax.first << " " << graph->unit;

                    moveCursor(30, detailY + 3);
                    setColor(Color::BrightRed);
                    std::cout << "Max: " << std::fixed << std::setprecision(2) << *minmax.second << " " << graph->unit;

                    moveCursor(4, detailY + 4);
                    setColor(Color::BrightYellow);
                    double avg = 0.0;
                    for (double val : graph->values)
                        avg += val;
                    avg /= graph->values.size();
                    std::cout << "Average: " << std::fixed << std::setprecision(2) << avg << " " << graph->unit;
                }
                else
                {
                    moveCursor(4, detailY + 2);
                    setColor(Color::BrightBlack);
                    std::cout << "No data available";
                }
            }
        }

        void GraphsScreen::renderMiniGraph(int x, int y, int width, int height, const GraphData &graph)
        {
            if (graph.values.empty())
                return;

            // Calculate scale
            double minVal = graph.minValue;
            double maxVal = graph.maxValue;

            if (graph.autoScale && !graph.values.empty())
            {
                auto minmax = std::minmax_element(graph.values.begin(), graph.values.end());
                minVal = *minmax.first;
                maxVal = *minmax.second;

                // Add some padding
                double range = maxVal - minVal;
                if (range < 0.1)
                    range = 0.1;
                minVal -= range * 0.1;
                maxVal += range * 0.1;
            }

            // Plot points
            setColor(graph.color);
            size_t dataPoints = (std::min)(static_cast<size_t>(width - 2), graph.values.size());

            for (size_t i = 0; i < dataPoints; ++i)
            {
                size_t dataIndex = graph.values.size() - dataPoints + i;
                double value = graph.values[dataIndex];

                // Scale to graph height
                int plotY = y + height - 3 - static_cast<int>((value - minVal) / (maxVal - minVal) * (height - 4));
                plotY = (std::max)(y + 1, (std::min)(y + height - 2, plotY));

                int plotX = x + 1 + static_cast<int>(i);

                moveCursor(plotX, plotY);
                std::cout << "●";
            }

            // Y-axis labels
            setColor(Color::BrightBlack);
            moveCursor(x + width - 8, y + 1);
            std::cout << std::fixed << std::setprecision(0) << maxVal;

            moveCursor(x + width - 8, y + height - 2);
            std::cout << std::fixed << std::setprecision(0) << minVal;
        }
    }
}
