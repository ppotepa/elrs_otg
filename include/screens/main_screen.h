#pragma once

#include "../screen_base.h"
#include <memory>
#include <vector>
#include <deque>
#include <chrono>

namespace ELRS
{
    namespace UI
    {
        /**
         * Data point for live graphs
         */
        struct GraphData
        {
            std::chrono::steady_clock::time_point timestamp;
            double value;
            Color color;
        };

        /**
         * Live graph widget for TX/RX data
         */
        class LiveGraph
        {
        public:
            LiveGraph(const std::string &title, int maxPoints = 100);

            void addDataPoint(double value, Color color = Color::Green);
            void addTxDataPoint(double value) { addDataPoint(value, Color::BrightGreen); }
            void addRxDataPoint(double value) { addDataPoint(value, Color::BrightRed); }

            void render(int x, int y, int width, int height);
            void setMinValue(double min)
            {
                minValue_ = min;
                autoScale_ = false;
            }
            void setMaxValue(double max)
            {
                maxValue_ = max;
                autoScale_ = false;
            }
            void setAutoScale(bool enabled) { autoScale_ = enabled; }

            const std::string &getTitle() const { return title_; }

        private:
            std::string title_;
            std::deque<GraphData> dataPoints_;
            int maxPoints_;
            double maxValue_;
            double minValue_;
            bool autoScale_;

            void updateScale();
        };

        /**
         * Main screen showing device information and live graphs
         */
        class MainScreen : public ScreenBase
        {
        public:
            MainScreen();
            virtual ~MainScreen() = default;

            // ScreenBase implementation
            bool initialize(const RenderContext &renderContext,
                            const NavigationContext &navContext) override;
            void onActivate() override;
            void onDeactivate() override;
            void update(std::chrono::milliseconds deltaTime) override;
            void render(const RenderContext &renderContext) override;
            bool handleKeyPress(FunctionKey key) override;
            void cleanup() override;

        private:
            void renderDeviceInfo(const RenderContext &context);
            void renderConnectionStats(const RenderContext &context);
            void renderGraphs(const RenderContext &context);
            void renderSimpleGraph(int x, int y, int width, int height, LiveGraph *graph);
            void updateGraphData();

            // Graph widgets
            std::unique_ptr<LiveGraph> txGraph_;
            std::unique_ptr<LiveGraph> rxGraph_;
            std::unique_ptr<LiveGraph> linkQualityGraph_;

            // Update timing
            std::chrono::steady_clock::time_point lastGraphUpdate_;
            static constexpr int GRAPH_UPDATE_INTERVAL_MS = 200; // 5 Hz
        };

    } // namespace UI
} // namespace ELRS