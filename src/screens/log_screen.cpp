#include "screens/log_screen.h"
#include <iostream>
#include <iomanip>

namespace ELRS
{
    namespace UI
    {
        LogScreen::LogScreen()
            : ScreenBase(ScreenType::Logs, "Logs"), scrollOffset_(0), maxLogEntries_(100), visibleLogCount_(0), lastLogCount_(0), lastLogUpdate_(std::chrono::steady_clock::now())
        {
        }

        bool LogScreen::initialize(const RenderContext &renderContext,
                                   const NavigationContext &navContext)
        {
            logInfo("Initializing log screen");

            // Calculate visible log count based on terminal size
            visibleLogCount_ = renderContext.contentHeight - 4; // Account for headers and borders

            return true;
        }

        void LogScreen::onActivate()
        {
            logInfo("Log screen activated - should be visible now");
            scrollOffset_ = 0; // Reset scroll when activating
            markForRefresh();
        }

        void LogScreen::onDeactivate()
        {
            logDebug("Log screen deactivated");
        }

        void LogScreen::update(std::chrono::milliseconds deltaTime)
        {
            // Check if there are new log messages
            const auto &logManager = getLogManager();
            auto currentLogCount = logManager.getLogCount();

            // If log count changed, we have new messages
            if (currentLogCount != lastLogCount_)
            {
                lastLogCount_ = currentLogCount;
                markForRefresh();

                // Auto-scroll to bottom when new messages arrive (if not manually scrolled)
                if (scrollOffset_ == 0) // At bottom
                {
                    auto messages = logManager.getRecentLogs(maxLogEntries_);
                    if (messages.size() > static_cast<size_t>(visibleLogCount_))
                    {
                        scrollOffset_ = 0; // Stay at bottom
                    }
                }
            }
        }

        void LogScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Render header
            printCentered(0, "ELRS OTG Monitor v1.0 - System Logs", Color::BrightWhite);

            // Render log content
            renderLogEntries(renderContext);

            // Render footer with navigation instructions
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);
            std::cout << "Navigation: UP/DOWN to scroll, ESC or F1 to return to main screen, F12 to exit";

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(Color::BrightYellow);

            auto logs = getLogManager().getRecentLogs(maxLogEntries_);
            std::cout << " Log Viewer | Total Entries: " << logs.size()
                      << " | Scroll Offset: " << scrollOffset_;

            resetColor();
            clearRefreshFlag();
        }

        bool LogScreen::handleKeyPress(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::Escape:
            case FunctionKey::F1:
                navigateToScreen(ScreenType::Main);
                return true;

            case FunctionKey::F12:
                exitApplication();
                return true;

            case FunctionKey::ArrowUp:
            case FunctionKey::PageUp:
            {
                auto logs = getLogManager().getRecentLogs(maxLogEntries_);
                int maxScroll = static_cast<int>(logs.size()) - visibleLogCount_;
                if (scrollOffset_ < maxScroll)
                {
                    scrollOffset_ += (key == FunctionKey::PageUp) ? 10 : 1;
                    if (scrollOffset_ > maxScroll)
                        scrollOffset_ = maxScroll;
                    markForRefresh();
                }
            }
                return true;

            case FunctionKey::ArrowDown:
            case FunctionKey::PageDown:
                if (scrollOffset_ > 0)
                {
                    scrollOffset_ -= (key == FunctionKey::PageDown) ? 10 : 1;
                    if (scrollOffset_ < 0)
                        scrollOffset_ = 0;
                    markForRefresh();
                }
                return true;

            case FunctionKey::Home:
                scrollOffset_ = 0;
                markForRefresh();
                return true;

            case FunctionKey::End:
            {
                auto logs = getLogManager().getRecentLogs(maxLogEntries_);
                int maxScroll = static_cast<int>(logs.size()) - visibleLogCount_;
                scrollOffset_ = (std::max)(0, maxScroll);
                markForRefresh();
            }
                return true;

            default:
                return false;
            }
        }

        void LogScreen::cleanup()
        {
            logInfo("Cleaning up log screen");
        }

        void LogScreen::renderLogEntries(const RenderContext &context)
        {
            // Get recent logs
            auto logs = getLogManager().getRecentLogs(maxLogEntries_);

            int contentStartY = 3;
            int contentWidth = context.terminalWidth - 4;

            // Render log screen header box
            setColor(Color::BrightYellow);
            drawBox(2, contentStartY, contentWidth, visibleLogCount_ + 4, "System Logs");

            // Instructions header
            moveCursor(4, contentStartY + 1);
            setColor(Color::BrightBlue);
            std::cout << "Navigation: UP/DOWN to scroll, PgUp/PgDn for fast scroll, Home/End to jump";

            // Separator
            moveCursor(2, contentStartY + 2);
            setColor(Color::White);
            std::cout << "+";
            for (int i = 1; i < contentWidth - 1; ++i)
                std::cout << "-";
            std::cout << "+";

            // Calculate visible log range
            int totalLogs = static_cast<int>(logs.size());
            int startIndex = (std::max)(0, totalLogs - visibleLogCount_ - scrollOffset_);
            int endIndex = (std::min)(totalLogs, startIndex + visibleLogCount_);

            // Render log entries
            for (int i = 0; i < visibleLogCount_; ++i)
            {
                int logIndex = startIndex + i;
                int displayY = contentStartY + 3 + i;

                moveCursor(2, displayY);

                if (logIndex < endIndex && logIndex >= 0 && logIndex < totalLogs)
                {
                    const auto &log = logs[logIndex];

                    // Set color based on log level
                    switch (log.level)
                    {
                    case LogLevel::Error:
                        setColor(Color::BrightRed);
                        break;
                    case LogLevel::Warning:
                        setColor(Color::BrightYellow);
                        break;
                    case LogLevel::Info:
                        setColor(Color::BrightGreen);
                        break;
                    case LogLevel::Debug:
                        setColor(Color::Cyan);
                        break;
                    }

                    // Format: | [HH:MM:SS] [LEVEL] [CATEGORY] Message
                    std::string logLine = "| " + log.getFormattedTime() + " [" +
                                          log.getLevelString() + "] [" + log.category + "] " +
                                          log.message;

                    // Truncate if too long
                    if (logLine.length() > static_cast<size_t>(contentWidth - 1))
                    {
                        logLine = logLine.substr(0, contentWidth - 4) + "...";
                    }

                    std::cout << logLine;

                    // Fill remaining space
                    int remaining = contentWidth - static_cast<int>(logLine.length());
                    for (int j = 0; j < remaining; ++j)
                        std::cout << " ";
                    std::cout << "|";
                }
                else
                {
                    // Empty line
                    setColor(Color::White);
                    std::cout << "|";
                    for (int j = 1; j < contentWidth - 1; ++j)
                        std::cout << " ";
                    std::cout << "|";
                }
            }

            // Bottom border
            moveCursor(2, contentStartY + 3 + visibleLogCount_);
            setColor(Color::White);
            std::cout << "+";
            for (int i = 1; i < contentWidth - 1; ++i)
                std::cout << "-";
            std::cout << "+";

            // Scroll indicators
            if (totalLogs > visibleLogCount_)
            {
                if (scrollOffset_ > 0)
                {
                    moveCursor(contentWidth - 20, contentStartY + 3 + visibleLogCount_);
                    setColor(Color::BrightBlue);
                    std::cout << " [" << scrollOffset_ << " more below] ";
                }

                int hiddenAbove = totalLogs - endIndex;
                if (hiddenAbove > 0)
                {
                    moveCursor(contentWidth - 40, contentStartY + 3 + visibleLogCount_);
                    setColor(Color::BrightBlue);
                    std::cout << " [" << hiddenAbove << " more above] ";
                }
            }

            resetColor();
        }

        void LogScreen::updateScrollLimits(int totalLogs, int visibleLogs)
        {
            int maxScroll = totalLogs - visibleLogs;
            if (scrollOffset_ > maxScroll)
            {
                scrollOffset_ = (std::max)(0, maxScroll);
            }
        }

    } // namespace UI
} // namespace ELRS