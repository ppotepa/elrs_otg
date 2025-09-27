#pragma once

#include "../screen_base.h"
#include <memory>
#include <vector>
#include <deque>

namespace ELRS
{
    namespace UI
    {
        /**
         * Graphs Screen - Historical telemetry visualization
         */
        class GraphsScreen : public ScreenBase
        {
        public:
            GraphsScreen();
            virtual ~GraphsScreen() = default;

            // ScreenBase interface
            bool initialize(const RenderContext &renderContext,
                            const NavigationContext &navContext) override;
            void onActivate() override;
            void onDeactivate() override;
            void update(std::chrono::milliseconds deltaTime) override;
            void render(const RenderContext &renderContext) override;
            bool handleKeyPress(FunctionKey key) override;
            void cleanup() override;

        private:
            struct GraphData
            {
                std::string title;
                std::string unit;
                std::deque<double> values;
                double minValue;
                double maxValue;
                Color color;
                bool autoScale;
            };

            std::vector<std::unique_ptr<GraphData>> graphs_;
            int selectedGraph_;
            std::chrono::steady_clock::time_point lastUpdate_;

            void initializeGraphs();
            void updateGraphData();
            void renderGraphs(const RenderContext &context);
            void renderGraphDetails(const RenderContext &context);
            void renderMiniGraph(int x, int y, int width, int height, const GraphData &graph);

            static constexpr int GRAPH_UPDATE_INTERVAL_MS = 200; // 5 Hz
            static constexpr size_t MAX_GRAPH_POINTS = 100;
        };
    }
}