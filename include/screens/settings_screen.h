#pragma once

#include "../screen_base.h"
#include <chrono>
#include <string>
#include <vector>
#include <map>

namespace ELRS
{
    namespace UI
    {
        enum class SettingType
        {
            Boolean,
            Integer,
            Float,
            String,
            Selection
        };

        struct SettingOption
        {
            std::string category;
            std::string name;
            std::string description;
            SettingType type;
            std::string currentValue;
            std::string defaultValue;
            std::vector<std::string> options; // For selection type
            int minValue;                     // For integer/float
            int maxValue;                     // For integer/float
            bool modified;
        };

        class SettingsScreen : public ScreenBase
        {
        public:
            SettingsScreen();
            virtual ~SettingsScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override;
            void onActivate() override;
            void onDeactivate() override;
            void update(std::chrono::milliseconds deltaTime) override;
            void render(const RenderContext &renderContext) override;
            bool handleKeyPress(FunctionKey key) override;
            void cleanup() override;

        private:
            void initializeSettings();
            void loadSettings();
            void saveSettings();
            void resetToDefaults();

            void renderSettingsCategories(const RenderContext &renderContext);
            void renderSettingsList(const RenderContext &renderContext);
            void renderSettingDetails(const RenderContext &renderContext);

            void editCurrentSetting();
            void applySettingChange(const std::string &newValue);
            std::vector<SettingOption *> getCurrentCategorySettings();

            std::string formatSettingValue(const SettingOption &setting) const;
            Color getSettingColor(const SettingOption &setting) const;
            bool validateSettingValue(const SettingOption &setting, const std::string &value) const;

            std::vector<SettingOption> settings_;
            std::vector<std::string> categories_;

            int selectedCategory_;
            int selectedSetting_;
            bool inEditMode_;
            std::string editBuffer_;

            std::chrono::steady_clock::time_point lastUpdate_;
            bool settingsModified_;

            static constexpr int SETTINGS_UPDATE_INTERVAL_MS = 500;
        };
    }
}