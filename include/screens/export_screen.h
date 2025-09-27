#pragma once

#include "../screen_base.h"
#include <chrono>
#include <string>
#include <vector>
#include <fstream>

namespace ELRS
{
    namespace UI
    {
        enum class ExportType
        {
            TelemetryData,
            LogFiles,
            Configuration,
            Screenshots,
            All
        };

        enum class ExportFormat
        {
            CSV,
            JSON,
            TXT,
            XML
        };

        enum class ExportState
        {
            Idle,
            Preparing,
            Exporting,
            Complete,
            Failed
        };

        struct ExportOption
        {
            ExportType type;
            std::string name;
            std::string description;
            std::vector<ExportFormat> supportedFormats;
            bool enabled;
        };

        class ExportScreen : public ScreenBase
        {
        public:
            ExportScreen();
            virtual ~ExportScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override;
            void onActivate() override;
            void onDeactivate() override;
            void update(std::chrono::milliseconds deltaTime) override;
            void render(const RenderContext &renderContext) override;
            bool handleKeyPress(FunctionKey key) override;
            void cleanup() override;

        private:
            void startExport();
            void updateExportProgress();
            void performExport();

            void renderExportOptions(const RenderContext &renderContext);
            void renderExportSettings(const RenderContext &renderContext);
            void renderExportProgress(const RenderContext &renderContext);

            bool exportTelemetryData(const std::string &filename, ExportFormat format);
            bool exportLogFiles(const std::string &filename, ExportFormat format);
            bool exportConfiguration(const std::string &filename, ExportFormat format);

            std::string getExportPath() const;
            std::string getFormatExtension(ExportFormat format) const;
            std::string getFormatName(ExportFormat format) const;
            Color getStateColor(ExportState state) const;
            std::string getStateText(ExportState state) const;

            std::vector<ExportOption> exportOptions_;
            ExportState currentState_;

            int selectedOption_;
            int selectedFormat_;
            std::string exportPath_;
            std::string statusMessage_;

            std::chrono::steady_clock::time_point exportStartTime_;
            std::chrono::steady_clock::time_point lastUpdate_;

            int exportProgress_;
            bool isExporting_;
            int totalFiles_;
            int processedFiles_;

            // Date range selection
            std::chrono::system_clock::time_point startDate_;
            std::chrono::system_clock::time_point endDate_;
            bool useDateRange_;

            static constexpr int EXPORT_UPDATE_INTERVAL_MS = 100;
        };
    }
}