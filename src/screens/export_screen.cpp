#include "screens/export_screen.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <algorithm>

namespace ELRS
{
    namespace UI
    {
        ExportScreen::ExportScreen()
            : ScreenBase(ScreenType::Export, "Export"),
              currentState_(ExportState::Idle),
              selectedOption_(0),
              selectedFormat_(0),
              exportProgress_(0),
              isExporting_(false),
              totalFiles_(0),
              processedFiles_(0),
              useDateRange_(false),
              lastUpdate_(std::chrono::steady_clock::now())
        {
            // Initialize export options
            exportOptions_ = {
                {ExportType::TelemetryData, "Telemetry Data", "Export live telemetry data and statistics", 
                 {ExportFormat::CSV, ExportFormat::JSON}, true},
                {ExportType::LogFiles, "Log Files", "Export application and system logs", 
                 {ExportFormat::TXT, ExportFormat::JSON}, true},
                {ExportType::Configuration, "Configuration", "Export device and application settings", 
                 {ExportFormat::JSON, ExportFormat::XML}, true},
                {ExportType::Screenshots, "Screenshots", "Export screen captures and images", 
                 {ExportFormat::TXT}, false}, // Disabled - not applicable for console app
                {ExportType::All, "Complete Export", "Export all available data types", 
                 {ExportFormat::JSON}, true}
            };
            
            exportPath_ = std::filesystem::current_path().string() + "/exports";
            
            // Set default date range (last 24 hours)
            endDate_ = std::chrono::system_clock::now();
            startDate_ = endDate_ - std::chrono::hours(24);
        }

        bool ExportScreen::initialize(const RenderContext &renderContext, const NavigationContext &navContext)
        {
            logInfo("Initializing export screen");
            currentState_ = ExportState::Idle;
            isExporting_ = false;
            exportProgress_ = 0;
            statusMessage_ = "Ready to export data";
            
            // Create export directory if it doesn't exist
            try
            {
                std::filesystem::create_directories(exportPath_);
            }
            catch (const std::exception &e)
            {
                logError("Failed to create export directory: " + std::string(e.what()));
                statusMessage_ = "Failed to create export directory";
            }
            
            return true;
        }

        void ExportScreen::onActivate()
        {
            logInfo("Export screen activated");
            markForRefresh();
        }

        void ExportScreen::onDeactivate()
        {
            logDebug("Export screen deactivated");
            if (isExporting_)
            {
                logWarning("Export screen deactivated during export operation");
            }
        }

        void ExportScreen::update(std::chrono::milliseconds deltaTime)
        {
            if (!isExporting_)
                return;

            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - lastUpdate_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

            if (ms.count() >= EXPORT_UPDATE_INTERVAL_MS)
            {
                updateExportProgress();
                markForRefresh();
                lastUpdate_ = now;
            }
        }

        void ExportScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Data Export", Color::BrightWhite);

            // Render export content
            renderExportOptions(renderContext);
            renderExportSettings(renderContext);
            renderExportProgress(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);

            if (isExporting_)
            {
                std::cout << "Export in progress... | ESC: Cancel | F12: Exit";
            }
            else
            {
                std::cout << "UP/DOWN: Select Option | LEFT/RIGHT: Change Format | ENTER: Start Export | P: Set Path | ESC/F1: Return | F12: Exit";
            }

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(getStateColor(currentState_));

            std::cout << " Export: " << getStateText(currentState_)
                      << " | Path: " << exportPath_
                      << " | " << statusMessage_;

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool ExportScreen::handleKeyPress(FunctionKey key)
        {
            if (isExporting_)
            {
                switch (key)
                {
                case FunctionKey::Escape:
                    // Cancel export
                    isExporting_ = false;
                    currentState_ = ExportState::Idle;
                    statusMessage_ = "Export cancelled by user";
                    logInfo("Export operation cancelled");
                    markForRefresh();
                    return true;
                case FunctionKey::F12:
                    exitApplication();
                    return true;
                default:
                    return true; // Consume all keys during export
                }
            }

            switch (key)
            {
            case FunctionKey::ArrowUp:
                if (selectedOption_ > 0)
                {
                    selectedOption_--;
                    selectedFormat_ = 0; // Reset format selection
                    markForRefresh();
                }
                return true;
            case FunctionKey::ArrowDown:
                if (selectedOption_ < static_cast<int>(exportOptions_.size()) - 1)
                {
                    selectedOption_++;
                    selectedFormat_ = 0; // Reset format selection
                    markForRefresh();
                }
                return true;
            case FunctionKey::ArrowLeft:
                if (selectedFormat_ > 0)
                {
                    selectedFormat_--;
                    markForRefresh();
                }
                return true;
            case FunctionKey::ArrowRight:
                if (selectedOption_ >= 0 && selectedOption_ < static_cast<int>(exportOptions_.size()))
                {
                    const auto &option = exportOptions_[selectedOption_];
                    if (selectedFormat_ < static_cast<int>(option.supportedFormats.size()) - 1)
                    {
                        selectedFormat_++;
                        markForRefresh();
                    }
                }
                return true;
            case FunctionKey::Enter:
                if (!isExporting_ && selectedOption_ >= 0 && 
                    selectedOption_ < static_cast<int>(exportOptions_.size()) &&
                    exportOptions_[selectedOption_].enabled)
                {
                    startExport();
                }
                return true;
            case FunctionKey::P:
            case FunctionKey::p:
                if (!isExporting_)
                {
                    // In a real implementation, we'd open a path selection dialog
                    logInfo("Path selection requested - using default path");
                    statusMessage_ = "Using default export path";
                    markForRefresh();
                }
                return true;
            case FunctionKey::D:
            case FunctionKey::d:
                if (!isExporting_)
                {
                    useDateRange_ = !useDateRange_;
                    statusMessage_ = useDateRange_ ? "Date range filtering enabled" : "Date range filtering disabled";
                    markForRefresh();
                }
                return true;
            case FunctionKey::Escape:
            case FunctionKey::F1:
                navigateToScreen(ScreenType::Main);
                return true;
            case FunctionKey::F12:
                exitApplication();
                return true;
            default:
                return false;
            }
        }

        void ExportScreen::cleanup()
        {
            logInfo("Cleaning up export screen");
            if (isExporting_)
            {
                logWarning("Cleanup called during export operation");
            }
        }

        void ExportScreen::startExport()
        {
            if (selectedOption_ < 0 || selectedOption_ >= static_cast<int>(exportOptions_.size()))
                return;

            const auto &option = exportOptions_[selectedOption_];
            if (!option.enabled || selectedFormat_ >= static_cast<int>(option.supportedFormats.size()))
                return;

            logInfo("Starting export: " + option.name);
            
            currentState_ = ExportState::Preparing;
            isExporting_ = true;
            exportStartTime_ = std::chrono::steady_clock::now();
            exportProgress_ = 0;
            processedFiles_ = 0;
            
            // Calculate total files based on export type
            switch (option.type)
            {
            case ExportType::TelemetryData:
                totalFiles_ = 1;
                break;
            case ExportType::LogFiles:
                totalFiles_ = 3; // App logs, system logs, error logs
                break;
            case ExportType::Configuration:
                totalFiles_ = 1;
                break;
            case ExportType::All:
                totalFiles_ = 5; // All types combined
                break;
            default:
                totalFiles_ = 1;
                break;
            }
            
            statusMessage_ = "Preparing export...";
            markForRefresh();
        }

        void ExportScreen::updateExportProgress()
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - exportStartTime_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            if (currentState_ == ExportState::Preparing)
            {
                // Preparation phase (1 second)
                exportProgress_ = ms.count() / 50 > 20 ? 20 : static_cast<int>(ms.count() / 50);
                
                if (exportProgress_ >= 20)
                {
                    currentState_ = ExportState::Exporting;
                    statusMessage_ = "Exporting data...";
                    performExport();
                }
            }
            else if (currentState_ == ExportState::Exporting)
            {
                // Export phase (based on processed files)
                exportProgress_ = 20 + ((processedFiles_ * 70) / totalFiles_);
                
                // Simulate file processing
                if (ms.count() > 2000 + (processedFiles_ * 1000)) // 1 second per file + 2 second prep
                {
                    processedFiles_++;
                    
                    if (processedFiles_ >= totalFiles_)
                    {
                        currentState_ = ExportState::Complete;
                        isExporting_ = false;
                        exportProgress_ = 100;
                        statusMessage_ = "Export completed successfully";
                        logInfo("Export operation completed");
                    }
                    else
                    {
                        statusMessage_ = "Processing file " + std::to_string(processedFiles_ + 1) + " of " + std::to_string(totalFiles_);
                    }
                }
            }
        }

        void ExportScreen::performExport()
        {
            const auto &option = exportOptions_[selectedOption_];
            const auto format = option.supportedFormats[selectedFormat_];
            
            // Generate timestamp for filename
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            
            std::stringstream timestamp;
            timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
            
            std::string baseFilename = exportPath_ + "/elrs_export_" + timestamp.str();
            
            try
            {
                bool success = false;
                
                switch (option.type)
                {
                case ExportType::TelemetryData:
                    success = exportTelemetryData(baseFilename + "_telemetry", format);
                    break;
                case ExportType::LogFiles:
                    success = exportLogFiles(baseFilename + "_logs", format);
                    break;
                case ExportType::Configuration:
                    success = exportConfiguration(baseFilename + "_config", format);
                    break;
                case ExportType::All:
                    success = exportTelemetryData(baseFilename + "_telemetry", format) &&
                             exportLogFiles(baseFilename + "_logs", format) &&
                             exportConfiguration(baseFilename + "_config", format);
                    break;
                default:
                    success = false;
                    break;
                }
                
                if (!success)
                {
                    currentState_ = ExportState::Failed;
                    statusMessage_ = "Export failed - check permissions and disk space";
                    logError("Export operation failed");
                }
            }
            catch (const std::exception &e)
            {
                currentState_ = ExportState::Failed;
                statusMessage_ = "Export failed: " + std::string(e.what());
                logError("Export exception: " + std::string(e.what()));
            }
        }

        void ExportScreen::renderExportOptions(const RenderContext &renderContext)
        {
            int startY = 3;
            int centerX = renderContext.terminalWidth / 2;

            // Export options header
            setColor(Color::BrightWhite);
            printCentered(startY, "Export Options:", Color::BrightWhite);

            // List export options
            for (size_t i = 0; i < exportOptions_.size(); i++)
            {
                const auto &option = exportOptions_[i];
                int y = startY + 2 + static_cast<int>(i);
                
                moveCursor(centerX - 40, y);
                
                if (static_cast<int>(i) == selectedOption_)
                {
                    setColor(Color::BrightBlue);
                    std::cout << "► ";
                }
                else
                {
                    setColor(Color::White);
                    std::cout << "  ";
                }
                
                // Option name
                if (option.enabled)
                {
                    setColor(Color::BrightGreen);
                }
                else
                {
                    setColor(Color::DarkGray);
                }
                
                std::cout << std::left << std::setw(20) << option.name;
                
                // Description
                setColor(Color::White);
                std::cout << " - " << option.description;
                
                // Show selected format
                if (static_cast<int>(i) == selectedOption_ && !option.supportedFormats.empty())
                {
                    setColor(Color::BrightYellow);
                    std::cout << " [" << getFormatName(option.supportedFormats[selectedFormat_]) << "]";
                }
            }
        }

        void ExportScreen::renderExportSettings(const RenderContext &renderContext)
        {
            int startY = 11;
            int centerX = renderContext.terminalWidth / 2;

            // Settings box
            moveCursor(centerX - 35, startY);
            setColor(Color::BrightCyan);
            std::cout << "╭──────────────────────────────────────────────────────────────────────╮";
            
            moveCursor(centerX - 35, startY + 1);
            std::cout << "│                          Export Settings                             │";
            
            moveCursor(centerX - 35, startY + 2);
            std::cout << "├──────────────────────────────────────────────────────────────────────┤";

            // Export path
            moveCursor(centerX - 35, startY + 3);
            std::cout << "│ Export Path: ";
            setColor(Color::BrightWhite);
            std::string displayPath = exportPath_;
            if (displayPath.length() > 50)
            {
                displayPath = "..." + displayPath.substr(displayPath.length() - 47);
            }
            std::cout << std::left << std::setw(55) << displayPath;
            setColor(Color::BrightCyan);
            std::cout << "│";

            // Date range
            moveCursor(centerX - 35, startY + 4);
            std::cout << "│ Date Range:  ";
            if (useDateRange_)
            {
                setColor(Color::BrightGreen);
                auto start_time_t = std::chrono::system_clock::to_time_t(startDate_);
                auto end_time_t = std::chrono::system_clock::to_time_t(endDate_);
                auto start_tm = *std::localtime(&start_time_t);
                auto end_tm = *std::localtime(&end_time_t);
                
                std::stringstream range;
                range << std::put_time(&start_tm, "%Y-%m-%d") << " to " << std::put_time(&end_tm, "%Y-%m-%d");
                std::cout << std::left << std::setw(55) << range.str();
            }
            else
            {
                setColor(Color::BrightYellow);
                std::cout << std::left << std::setw(55) << "All available data";
            }
            setColor(Color::BrightCyan);
            std::cout << "│";

            // File format
            if (selectedOption_ >= 0 && selectedOption_ < static_cast<int>(exportOptions_.size()))
            {
                const auto &option = exportOptions_[selectedOption_];
                if (!option.supportedFormats.empty())
                {
                    moveCursor(centerX - 35, startY + 5);
                    std::cout << "│ Format:      ";
                    setColor(Color::BrightYellow);
                    std::cout << std::left << std::setw(55) << getFormatName(option.supportedFormats[selectedFormat_]);
                    setColor(Color::BrightCyan);
                    std::cout << "│";
                }
            }

            moveCursor(centerX - 35, startY + 6);
            std::cout << "╰──────────────────────────────────────────────────────────────────────╯";
        }

        void ExportScreen::renderExportProgress(const RenderContext &renderContext)
        {
            if (!isExporting_ && currentState_ != ExportState::Complete && currentState_ != ExportState::Failed)
                return;

            int startY = 19;
            int centerX = renderContext.terminalWidth / 2;
            int progressWidth = 60;

            // Progress section
            setColor(getStateColor(currentState_));
            printCentered(startY, getStateText(currentState_), getStateColor(currentState_));

            // Progress bar
            moveCursor(centerX - progressWidth/2, startY + 2);
            setColor(Color::BrightCyan);
            std::cout << "[";
            
            int filled = (exportProgress_ * (progressWidth - 2)) / 100;
            for (int i = 0; i < progressWidth - 2; i++)
            {
                if (i < filled)
                {
                    setColor(Color::BrightGreen);
                    std::cout << "█";
                }
                else
                {
                    setColor(Color::DarkGray);
                    std::cout << "░";
                }
            }
            
            setColor(Color::BrightCyan);
            std::cout << "] " << exportProgress_ << "%";

            // File progress
            if (isExporting_ && totalFiles_ > 0)
            {
                moveCursor(centerX - 15, startY + 4);
                setColor(Color::BrightWhite);
                std::cout << "Files: " << processedFiles_ << " / " << totalFiles_;
            }

            // Status message
            printCentered(startY + 6, statusMessage_, Color::BrightWhite);
        }

        bool ExportScreen::exportTelemetryData(const std::string &filename, ExportFormat format)
        {
            std::string fullPath = filename + getFormatExtension(format);
            
            try
            {
                std::ofstream file(fullPath);
                if (!file.is_open())
                    return false;

                const auto &radioState = getRadioState();
                const auto &telemetry = radioState.getLiveTelemetry();

                if (format == ExportFormat::CSV)
                {
                    file << "Timestamp,LinkQuality,SignalStrength,PacketsRX,PacketsTX\n";
                    
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    auto tm = *std::localtime(&time_t);
                    
                    file << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << ","
                         << telemetry.linkQuality << ","
                         << telemetry.rssi1 << ","
                         << telemetry.packetsReceived << ","
                         << telemetry.packetsTransmitted << "\n";
                }
                else if (format == ExportFormat::JSON)
                {
                    file << "{\n";
                    file << "  \"telemetry\": {\n";
                    file << "    \"linkQuality\": " << telemetry.linkQuality << ",\n";
                    file << "    \"rssi1\": " << telemetry.rssi1 << ",\n";
                    file << "    \"packetsReceived\": " << telemetry.packetsReceived << ",\n";
                    file << "    \"packetsTransmitted\": " << telemetry.packetsTransmitted << "\n";
                    file << "  }\n";
                    file << "}\n";
                }
                
                file.close();
                logInfo("Exported telemetry data to: " + fullPath);
                return true;
            }
            catch (const std::exception &e)
            {
                logError("Failed to export telemetry data: " + std::string(e.what()));
                return false;
            }
        }

        bool ExportScreen::exportLogFiles(const std::string &filename, ExportFormat format)
        {
            std::string fullPath = filename + getFormatExtension(format);
            
            try
            {
                std::ofstream file(fullPath);
                if (!file.is_open())
                    return false;

                const auto &logManager = getLogManager();
                auto logs = logManager.getRecentLogs(1000); // Get last 1000 entries

                if (format == ExportFormat::TXT)
                {
                    for (const auto &log : logs)
                    {
                        file << log.getFormattedTime() << " [" << log.getLevelString() << "] ["
                             << log.category << "] " << log.message << "\n";
                    }
                }
                else if (format == ExportFormat::JSON)
                {
                    file << "{\n";
                    file << "  \"logs\": [\n";
                    
                    for (size_t i = 0; i < logs.size(); i++)
                    {
                        const auto &log = logs[i];
                        file << "    {\n";
                        file << "      \"timestamp\": \"" << log.getFormattedTime() << "\",\n";
                        file << "      \"level\": \"" << log.getLevelString() << "\",\n";
                        file << "      \"category\": \"" << log.category << "\",\n";
                        file << "      \"message\": \"" << log.message << "\"\n";
                        file << "    }";
                        if (i < logs.size() - 1) file << ",";
                        file << "\n";
                    }
                    
                    file << "  ]\n";
                    file << "}\n";
                }
                
                file.close();
                logInfo("Exported log files to: " + fullPath);
                return true;
            }
            catch (const std::exception &e)
            {
                logError("Failed to export log files: " + std::string(e.what()));
                return false;
            }
        }

        bool ExportScreen::exportConfiguration(const std::string &filename, ExportFormat format)
        {
            std::string fullPath = filename + getFormatExtension(format);
            
            try
            {
                std::ofstream file(fullPath);
                if (!file.is_open())
                    return false;

                const auto &radioState = getRadioState();
                const auto &deviceConfig = radioState.getDeviceConfiguration();

                if (format == ExportFormat::JSON)
                {
                    file << "{\n";
                    file << "  \"device\": {\n";
                    file << "    \"productName\": \"" << deviceConfig.productName << "\",\n";
                    file << "    \"manufacturer\": \"" << deviceConfig.manufacturer << "\",\n";
                    file << "    \"serialNumber\": \"" << deviceConfig.serialNumber << "\",\n";
                    file << "    \"vid\": " << deviceConfig.vid << ",\n";
                    file << "    \"pid\": " << deviceConfig.pid << "\n";
                    file << "  },\n";
                    file << "  \"exportInfo\": {\n";
                    auto now = std::chrono::system_clock::now().time_since_epoch();
                    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now);
                    file << "    \"exportDate\": \"" << seconds.count() << "\",\n";
                    file << "    \"version\": \"1.0\"\n";
                    file << "  }\n";
                    file << "}\n";
                }
                else if (format == ExportFormat::XML)
                {
                    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                    file << "<configuration>\n";
                    file << "  <device>\n";
                    file << "    <productName>" << deviceConfig.productName << "</productName>\n";
                    file << "    <manufacturer>" << deviceConfig.manufacturer << "</manufacturer>\n";
                    file << "    <serialNumber>" << deviceConfig.serialNumber << "</serialNumber>\n";
                    file << "    <vid>" << deviceConfig.vid << "</vid>\n";
                    file << "    <pid>" << deviceConfig.pid << "</pid>\n";
                    file << "  </device>\n";
                    file << "</configuration>\n";
                }
                
                file.close();
                logInfo("Exported configuration to: " + fullPath);
                return true;
            }
            catch (const std::exception &e)
            {
                logError("Failed to export configuration: " + std::string(e.what()));
                return false;
            }
        }

        std::string ExportScreen::getExportPath() const
        {
            return exportPath_;
        }

        std::string ExportScreen::getFormatExtension(ExportFormat format) const
        {
            switch (format)
            {
            case ExportFormat::CSV: return ".csv";
            case ExportFormat::JSON: return ".json";
            case ExportFormat::TXT: return ".txt";
            case ExportFormat::XML: return ".xml";
            default: return ".txt";
            }
        }

        std::string ExportScreen::getFormatName(ExportFormat format) const
        {
            switch (format)
            {
            case ExportFormat::CSV: return "CSV (Comma Separated Values)";
            case ExportFormat::JSON: return "JSON (JavaScript Object Notation)";
            case ExportFormat::TXT: return "TXT (Plain Text)";
            case ExportFormat::XML: return "XML (Extensible Markup Language)";
            default: return "Unknown";
            }
        }

        Color ExportScreen::getStateColor(ExportState state) const
        {
            switch (state)
            {
            case ExportState::Idle: return Color::BrightBlue;
            case ExportState::Preparing: return Color::BrightYellow;
            case ExportState::Exporting: return Color::BrightCyan;
            case ExportState::Complete: return Color::BrightGreen;
            case ExportState::Failed: return Color::BrightRed;
            default: return Color::White;
            }
        }

        std::string ExportScreen::getStateText(ExportState state) const
        {
            switch (state)
            {
            case ExportState::Idle: return "Ready to Export";
            case ExportState::Preparing: return "Preparing Export";
            case ExportState::Exporting: return "Exporting Data";
            case ExportState::Complete: return "Export Complete";
            case ExportState::Failed: return "Export Failed";
            default: return "Unknown";
            }
        }
    }
}