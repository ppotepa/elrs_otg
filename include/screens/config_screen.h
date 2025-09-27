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
         * Configuration Screen - Device settings and parameters
         */
        class ConfigScreen : public ScreenBase
        {
        public:
            ConfigScreen();
            virtual ~ConfigScreen() = default;

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
            struct ConfigOption
            {
                std::string name;
                std::string value;
                std::string description;
                bool isEditable;
            };

            std::vector<ConfigOption> configOptions_;
            int selectedOption_;
            bool inEditMode_;
            std::string editBuffer_;
            std::chrono::steady_clock::time_point lastUpdate_;

            void initializeConfigOptions();
            void renderConfigList(const RenderContext &context);
            void renderConfigDetails(const RenderContext &context);
            void saveConfiguration();
            void loadConfiguration();

            static constexpr int CONFIG_UPDATE_INTERVAL_MS = 1000; // 1 Hz
        };
    }
}