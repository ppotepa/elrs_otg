#pragma once

#include "../screen_base.h"
#include <chrono>
#include <string>

namespace ELRS
{
    namespace UI
    {
        enum class BindState
        {
            Idle,
            Binding,
            Bound,
            Failed,
            Timeout
        };

        class BindScreen : public ScreenBase
        {
        public:
            BindScreen();
            virtual ~BindScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override;
            void onActivate() override;
            void onDeactivate() override;
            void update(std::chrono::milliseconds deltaTime) override;
            void render(const RenderContext &renderContext) override;
            bool handleKeyPress(FunctionKey key) override;
            void cleanup() override;

        private:
            void startBinding();
            void stopBinding();
            void updateBindingStatus();
            void renderBindStatus(const RenderContext &renderContext);
            void renderBindControls(const RenderContext &renderContext);
            void renderBindProgress(const RenderContext &renderContext);

            std::string generateBindPhrase();
            Color getStateColor(BindState state) const;
            std::string getStateText(BindState state) const;

            BindState currentState_;
            std::chrono::steady_clock::time_point bindStartTime_;
            std::chrono::steady_clock::time_point lastUpdate_;
            std::string bindPhrase_;
            int bindProgress_;
            int bindTimeout_;
            bool isBinding_;

            static constexpr int BIND_TIMEOUT_SECONDS = 30;
            static constexpr int BIND_UPDATE_INTERVAL_MS = 100;
        };
    }
}