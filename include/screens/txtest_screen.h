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
         * TX Test Screen - Transmitter testing and diagnostics
         */
        class TxTestScreen : public ScreenBase
        {
        public:
            TxTestScreen();
            virtual ~TxTestScreen() = default;

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
            enum class TestMode
            {
                Idle,
                ContinuousWave,
                ModulatedSignal,
                PowerSweep,
                FrequencySweep
            };

            struct TestResult
            {
                std::string testName;
                std::string result;
                std::string status;
                int progress;
                bool passed;
                std::string details;
            };

            TestMode currentMode_;
            std::vector<TestResult> testResults_;
            std::chrono::steady_clock::time_point testStartTime_;
            std::chrono::steady_clock::time_point lastUpdate_;
            bool isRunning_;
            int selectedTest_;

            void startTest(TestMode mode);
            void stopTest();
            void updateTestResults();
            void renderTestControls(const RenderContext &context);
            void renderTestResults(const RenderContext &context);
            void renderTestStatus(const RenderContext &context);

            static constexpr int TEST_UPDATE_INTERVAL_MS = 500; // 2 Hz
        };
    }
}