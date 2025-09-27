#pragma once

#include "../screen_base.h"
#include <memory>
#include <vector>
#include <string>

namespace ELRS
{
    namespace UI
    {
        /**
         * Monitor Screen - Real-time telemetry monitoring
         */
        class MonitorScreen : public ScreenBase
        {
        public:
            MonitorScreen();
            virtual ~MonitorScreen() = default;

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
            struct MonitorValue
            {
                std::string name;
                std::string value;
                std::string unit;
                Color color;
                bool isAlert;
            };

            std::vector<MonitorValue> monitorValues_;
            std::chrono::steady_clock::time_point lastUpdate_;
            bool isPaused_;

            void updateMonitorValues();
            void renderMonitorGrid(const RenderContext &context);
            void renderAlerts(const RenderContext &context);

            static constexpr int MONITOR_UPDATE_INTERVAL_MS = 100; // 10 Hz
        };
    }
}