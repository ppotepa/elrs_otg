#pragma once

#include "../screen_base.h"

namespace ELRS
{
    namespace UI
    {
        /**
         * Log screen for viewing system logs
         */
        class LogScreen : public ScreenBase
        {
        public:
            LogScreen();
            virtual ~LogScreen() = default;

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
            void renderLogEntries(const RenderContext &context);
            void updateScrollLimits(int totalLogs, int visibleLogs);

            int scrollOffset_;
            int maxLogEntries_;
            int visibleLogCount_;
            size_t lastLogCount_;

            // Auto-refresh timing
            std::chrono::steady_clock::time_point lastLogUpdate_;
            static constexpr int LOG_UPDATE_INTERVAL_MS = 500; // 2 Hz
        };

    } // namespace UI
} // namespace ELRS