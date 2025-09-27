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
         * RX Test Screen - Receiver testing and diagnostics
         */
        class RxTestScreen : public ScreenBase
        {
        public:
            RxTestScreen();
            virtual ~RxTestScreen() = default;

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
            struct ChannelTest
            {
                int channel;
                std::string name;
                int value;
                int minValue;
                int maxValue;
                bool isActive;
            };

            std::vector<ChannelTest> channels_;
            std::chrono::steady_clock::time_point lastUpdate_;
            bool isRecording_;
            int selectedChannel_;

            void initializeChannels();
            void updateChannelData();
            void renderChannelGrid(const RenderContext &context);
            void renderChannelDetails(const RenderContext &context);

            static constexpr int RX_UPDATE_INTERVAL_MS = 50; // 20 Hz
        };
    }
}