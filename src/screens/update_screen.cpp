#include "screens/update_screen.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>

namespace ELRS
{
    namespace UI
    {
        UpdateScreen::UpdateScreen()
            : ScreenBase(ScreenType::Update, "Update"), 
              currentState_(UpdateState::Idle),
              selectedFirmware_(0),
              updateProgress_(0),
              isUpdating_(false),
              operationStartTime_(std::chrono::steady_clock::now()),
              lastUpdate_(std::chrono::steady_clock::now())
        {
            // Initialize firmware info
            currentFirmware_ = {"3.0.1", "2024-09-15", "ESP32", "", 0, false};
            
            // Populate available firmware versions
            availableFirmware_ = {
                {"3.1.0", "2024-09-25", "ESP32", "http://releases.elrs.org/3.1.0/elrs-esp32.bin", 1024000, true},
                {"3.0.2", "2024-09-20", "ESP32", "http://releases.elrs.org/3.0.2/elrs-esp32.bin", 1020000, true},
                {"3.0.1", "2024-09-15", "ESP32", "http://releases.elrs.org/3.0.1/elrs-esp32.bin", 1018000, false},
                {"3.0.0", "2024-09-01", "ESP32", "http://releases.elrs.org/3.0.0/elrs-esp32.bin", 1015000, false}
            };
            
            latestFirmware_ = availableFirmware_[0];
        }

        bool UpdateScreen::initialize(const RenderContext &renderContext, const NavigationContext &navContext)
        {
            statusMessage_ = "Ready to check for updates";
            return true;
        }

        void UpdateScreen::onActivate()
        {
            isUpdating_ = false;
            currentState_ = UpdateState::Idle;
        }

        void UpdateScreen::onDeactivate()
        {
        }

        void UpdateScreen::update(std::chrono::milliseconds deltaTime)
        {
            if (!isUpdating_)
                return;

            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - lastUpdate_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

            if (ms.count() >= UPDATE_INTERVAL_MS)
            {
                updateProgress();
                markForRefresh();
                lastUpdate_ = now;
            }
        }

        void UpdateScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Firmware Update", Color::BrightWhite);

            // Render content sections
            renderVersionInfo(renderContext);
            renderFirmwareList(renderContext);
            renderUpdateProgress(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);

            if (isUpdating_)
            {
                std::cout << "ESC: Cancel Update | F9: Back | F12: Exit";
            }
            else
            {
                std::cout << "▲/▼: Select firmware  ENTER: Start update  C: Check version  R: Refresh  F9: Back | F12: Exit";
            }

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(Color::BrightCyan);
            std::cout << " Status: " << statusMessage_;

            resetColor();
            clearRefreshFlag();
        }

        bool UpdateScreen::handleKeyPress(FunctionKey key)
        {
            if (isUpdating_)
                return true; // Block input during update
                
            switch (key)
            {
                case FunctionKey::ArrowUp:
                    if (selectedFirmware_ > 0)
                    {
                        selectedFirmware_--;
                        markForRefresh();
                    }
                    return true;
                    
                case FunctionKey::ArrowDown:
                    if (selectedFirmware_ < static_cast<int>(availableFirmware_.size()) - 1)
                    {
                        selectedFirmware_++;
                        markForRefresh();
                    }
                    return true;
                    
                case FunctionKey::Enter:
                    if (selectedFirmware_ >= 0 && selectedFirmware_ < static_cast<int>(availableFirmware_.size()))
                    {
                        downloadFirmware();
                    }
                    return true;
                    
                case FunctionKey::C:
                case FunctionKey::c:
                    checkFirmwareVersion();
                    return true;
                    
                case FunctionKey::R:
                case FunctionKey::r:
                    // Refresh firmware list (simulate fetching)
                    statusMessage_ = "Refreshing firmware list...";
                    markForRefresh();
                    return true;
                    
                case FunctionKey::F9:
                    navigateToScreen(ScreenType::Main);
                    return true;
                    
                default:
                    return false;
            }
        }

        void UpdateScreen::cleanup()
        {
            isUpdating_ = false;
            currentState_ = UpdateState::Idle;
        }

        void UpdateScreen::checkFirmwareVersion()
        {
            currentState_ = UpdateState::CheckingVersion;
            isUpdating_ = true;
            operationStartTime_ = std::chrono::steady_clock::now();
            statusMessage_ = "Checking firmware version...";
            markForRefresh();
        }

        void UpdateScreen::downloadFirmware()
        {
            if (selectedFirmware_ >= 0 && selectedFirmware_ < static_cast<int>(availableFirmware_.size()))
            {
                currentState_ = UpdateState::DownloadingFirmware;
                isUpdating_ = true;
                operationStartTime_ = std::chrono::steady_clock::now();
                updateProgress_ = 0;
                statusMessage_ = "Downloading firmware: " + availableFirmware_[selectedFirmware_].version;
                markForRefresh();
            }
        }

        void UpdateScreen::flashFirmware()
        {
            currentState_ = UpdateState::FlashingFirmware;
            operationStartTime_ = std::chrono::steady_clock::now();
            updateProgress_ = 0;
            statusMessage_ = "Flashing firmware...";
            markForRefresh();
        }

        void UpdateScreen::updateProgress()
        {
            switch (currentState_)
            {
                case UpdateState::CheckingVersion:
                    simulateVersionCheck();
                    break;
                case UpdateState::DownloadingFirmware:
                    simulateDownload();
                    break;
                case UpdateState::FlashingFirmware:
                    simulateFlashing();
                    break;
                default:
                    break;
            }
        }

        void UpdateScreen::renderVersionInfo(const RenderContext &renderContext)
        {
            int startY = 2;
            
            // Current firmware info
            moveCursor(2, startY);
            setColor(Color::BrightWhite);
            std::cout << "Current Firmware Information:";
            
            moveCursor(4, startY + 1);
            setColor(Color::White);
            std::cout << "Version: ";
            setColor(Color::BrightGreen);
            std::cout << currentFirmware_.version;
            
            moveCursor(4, startY + 2);
            setColor(Color::White);
            std::cout << "Build Date: ";
            setColor(Color::BrightBlue);
            std::cout << currentFirmware_.buildDate;
            
            moveCursor(4, startY + 3);
            setColor(Color::White);
            std::cout << "Target: ";
            setColor(Color::BrightCyan);
            std::cout << currentFirmware_.target;
            
            // Show comparison with latest
            if (!latestFirmware_.version.empty())
            {
                bool isUpToDate = currentFirmware_.version == latestFirmware_.version;
                moveCursor(4, startY + 4);
                setColor(Color::White);
                std::cout << "Status: ";
                setColor(isUpToDate ? Color::BrightGreen : Color::BrightYellow);
                std::cout << (isUpToDate ? "Up to date" : "Update available");
                
                if (!isUpToDate)
                {
                    moveCursor(4, startY + 5);
                    setColor(Color::White);
                    std::cout << "Latest: ";
                    setColor(Color::BrightCyan);
                    std::cout << latestFirmware_.version;
                }
            }
        }

        void UpdateScreen::renderFirmwareList(const RenderContext &renderContext)
        {
            int startY = 8;
            
            moveCursor(2, startY);
            setColor(Color::BrightWhite);
            std::cout << "Available Firmware Versions:";
            
            for (size_t i = 0; i < availableFirmware_.size(); ++i)
            {
                const auto& fw = availableFirmware_[i];
                bool isSelected = static_cast<int>(i) == selectedFirmware_;
                bool isCurrent = fw.version == currentFirmware_.version;
                
                int rowY = startY + 1 + static_cast<int>(i);
                
                // Selection indicator
                moveCursor(4, rowY);
                if (isSelected)
                {
                    setColor(Color::BrightYellow);
                    std::cout << "► ";
                }
                else
                {
                    setColor(Color::White);
                    std::cout << "  ";
                }
                
                // Version
                if (isCurrent)
                    setColor(Color::BrightGreen);
                else if (fw.isNewer)
                    setColor(Color::BrightCyan);
                else
                    setColor(Color::White);
                std::cout << std::left << std::setw(8) << fw.version;
                
                // Build date
                setColor(Color::DarkGray);
                std::cout << " " << std::left << std::setw(12) << fw.buildDate;
                
                // Target
                setColor(Color::BrightBlue);
                std::cout << " " << std::left << std::setw(8) << fw.target;
                
                // Size
                setColor(Color::DarkGray);
                std::stringstream sizeStr;
                sizeStr << std::fixed << std::setprecision(1) << (fw.fileSize / 1024.0) << " KB";
                std::cout << " " << std::left << std::setw(10) << sizeStr.str();
                
                // Status indicators
                if (isCurrent)
                {
                    setColor(Color::BrightGreen);
                    std::cout << " [CURRENT]";
                }
                else if (fw.isNewer)
                {
                    setColor(Color::BrightYellow);
                    std::cout << " [NEW]";
                }
            }
        }

        void UpdateScreen::renderUpdateProgress(const RenderContext &renderContext)
        {
            if (!isUpdating_)
                return;
                
            int startY = 8 + static_cast<int>(availableFirmware_.size()) + 2;
            
            // State and progress
            moveCursor(2, startY);
            setColor(Color::BrightWhite);
            std::cout << "Update Progress:";
            
            moveCursor(4, startY + 1);
            setColor(Color::White);
            std::cout << "State: ";
            setColor(getStateColor());
            std::cout << getStateText();
            
            // Progress bar
            if (updateProgress_ > 0)
            {
                moveCursor(4, startY + 2);
                setColor(Color::White);
                std::cout << "Progress: [";
                
                int barWidth = 30;
                int filled = (updateProgress_ * barWidth) / 100;
                
                setColor(Color::BrightGreen);
                for (int i = 0; i < filled; ++i)
                    std::cout << "█";
                    
                setColor(Color::DarkGray);
                for (int i = filled; i < barWidth; ++i)
                    std::cout << "░";
                    
                setColor(Color::White);
                std::cout << "] " << updateProgress_ << "%";
            }
            
            // Status message
            if (!statusMessage_.empty())
            {
                moveCursor(4, startY + 3);
                setColor(Color::BrightCyan);
                std::cout << "Status: " << statusMessage_;
            }
        }

        void UpdateScreen::simulateVersionCheck()
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - operationStartTime_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            updateProgress_ = ms.count() / 20 > 100 ? 100 : static_cast<int>(ms.count() / 20); // 2 seconds total
            
            if (updateProgress_ >= 100)
            {
                currentState_ = UpdateState::Idle;
                isUpdating_ = false;
                statusMessage_ = "Version check complete";
                updateProgress_ = 0;
            }
        }

        void UpdateScreen::simulateDownload()
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - operationStartTime_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            updateProgress_ = ms.count() / 100 > 100 ? 100 : static_cast<int>(ms.count() / 100); // 10 seconds total
            
            if (updateProgress_ >= 100)
            {
                flashFirmware(); // Automatically proceed to flashing
            }
            else
            {
                // Update download status
                const auto& firmware = availableFirmware_[selectedFirmware_];
                int downloaded = (firmware.fileSize * updateProgress_) / 100;
                statusMessage_ = "Downloaded " + std::to_string(downloaded / 1024) + " KB / " + 
                               std::to_string(firmware.fileSize / 1024) + " KB";
            }
        }

        void UpdateScreen::simulateFlashing()
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - operationStartTime_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            updateProgress_ = ms.count() / 80 > 100 ? 100 : static_cast<int>(ms.count() / 80); // 8 seconds total
            
            if (updateProgress_ >= 100)
            {
                currentState_ = UpdateState::Complete;
                isUpdating_ = false;
                statusMessage_ = "Firmware update completed successfully!";
                
                // Update current firmware info
                if (selectedFirmware_ >= 0 && selectedFirmware_ < static_cast<int>(availableFirmware_.size()))
                {
                    currentFirmware_ = availableFirmware_[selectedFirmware_];
                    currentFirmware_.isNewer = false; // No longer newer since we just installed it
                }
                
                updateProgress_ = 0;
            }
            else
            {
                statusMessage_ = "Flashing firmware... " + std::to_string(updateProgress_) + "%";
            }
        }

        Color UpdateScreen::getStateColor() const
        {
            switch (currentState_)
            {
                case UpdateState::Idle:
                    return Color::White;
                case UpdateState::CheckingVersion:
                    return Color::BrightYellow;
                case UpdateState::DownloadingFirmware:
                    return Color::BrightCyan;
                case UpdateState::FlashingFirmware:
                    return Color::BrightMagenta;
                case UpdateState::Complete:
                    return Color::BrightGreen;
                case UpdateState::Failed:
                    return Color::BrightRed;
                default:
                    return Color::White;
            }
        }

        std::string UpdateScreen::getStateText() const
        {
            switch (currentState_)
            {
                case UpdateState::Idle:
                    return "Ready";
                case UpdateState::CheckingVersion:
                    return "Checking Version";
                case UpdateState::DownloadingFirmware:
                    return "Downloading";
                case UpdateState::FlashingFirmware:
                    return "Flashing";
                case UpdateState::Complete:
                    return "Complete";
                case UpdateState::Failed:
                    return "Failed";
                default:
                    return "Unknown";
            }
        }

        constexpr int UpdateScreen::UPDATE_INTERVAL_MS;
    }
}