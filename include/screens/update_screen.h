#pragma once

#include "../screen_base.h"
#include <chrono>
#include <string>
#include <vector>

namespace ELRS
{
    namespace UI
    {
        enum class UpdateState
        {
            Idle,
            CheckingVersion,
            DownloadingFirmware,
            FlashingFirmware,
            Complete,
            Failed
        };

        struct FirmwareInfo
        {
            std::string version;
            std::string buildDate;
            std::string target;
            std::string downloadUrl;
            size_t fileSize;
            bool isNewer;
        };

        class UpdateScreen : public ScreenBase
        {
        public:
            UpdateScreen();
            virtual ~UpdateScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override;
            void onActivate() override;
            void onDeactivate() override;
            void update(std::chrono::milliseconds deltaTime) override;
            void render(const RenderContext &renderContext) override;
            bool handleKeyPress(FunctionKey key) override;
            void cleanup() override;

        private:
            void checkFirmwareVersion();
            void downloadFirmware();
            void flashFirmware();
            void updateProgress();
            
            void renderVersionInfo(const RenderContext &renderContext);
            void renderUpdateProgress(const RenderContext &renderContext);
            void renderFirmwareList(const RenderContext &renderContext);
            
            Color getStateColor() const;
            std::string getStateText() const;
            void simulateVersionCheck();
            void simulateDownload();
            void simulateFlashing();

            UpdateState currentState_;
            std::chrono::steady_clock::time_point operationStartTime_;
            std::chrono::steady_clock::time_point lastUpdate_;
            
            FirmwareInfo currentFirmware_;
            FirmwareInfo latestFirmware_;
            std::vector<FirmwareInfo> availableFirmware_;
            
            int selectedFirmware_;
            int updateProgress_;
            bool isUpdating_;
            std::string statusMessage_;
            
            static constexpr int UPDATE_INTERVAL_MS = 50;
        };
    }
}