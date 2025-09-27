#include "screen_base.h"
// #include "screens/main_screen.h"      // Not implemented yet
// #include "screens/log_screen.h"       // Not implemented yet
// #include "screens/config_screen.h"    // Not implemented yet
// #include "screens/monitor_screen.h"   // Not implemented yet
// #include "screens/graphs_screen.h"    // Not implemented yet
// #include "screens/txtest_screen.h"    // Not implemented yet
// #include "screens/rxtest_screen.h"    // Not implemented yet
#include "screens/bind_screen.h"
#include "screens/update_screen.h"
#include "screens/export_screen.h"
#include "screens/settings_screen.h"
#include <iostream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace ELRS
{
    namespace UI
    {
        ScreenBase::ScreenBase(ScreenType screenType, const std::string &title)
            : screenType_(screenType), title_(title), isActive_(false), needsRefresh_(true)
        {
            logDebug("Screen created: " + title);
        }

        void ScreenBase::navigateToScreen(ScreenType screen)
        {
            if (navContext_.switchToScreen)
            {
                logInfo("Navigating from " + title_ + " to " + ScreenFactory::getScreenName(screen));
                navContext_.switchToScreen(screen);
            }
        }

        void ScreenBase::goBack()
        {
            navigateToScreen(navContext_.previousScreen);
        }

        void ScreenBase::exitApplication()
        {
            if (navContext_.exitApplication)
            {
                logInfo("Exit requested from " + title_);
                navContext_.exitApplication();
            }
        }

        void ScreenBase::moveCursor(int x, int y)
        {
#ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD coord;
            coord.X = static_cast<SHORT>(x);
            coord.Y = static_cast<SHORT>(y);
            SetConsoleCursorPosition(hConsole, coord);
#else
            std::cout << "\033[" << (y + 1) << ";" << (x + 1) << "H" << std::flush;
#endif
        }

        void ScreenBase::setColor(Color fg, Color bg)
        {
#ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            WORD attributes = static_cast<WORD>(fg) | (static_cast<WORD>(bg) << 4);
            SetConsoleTextAttribute(hConsole, attributes);
#else
            std::cout << "\033[" << (30 + static_cast<int>(fg)) << ";" << (40 + static_cast<int>(bg)) << "m" << std::flush;
#endif
        }

        void ScreenBase::resetColor()
        {
#ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
#else
            std::cout << "\033[0m" << std::flush;
#endif
        }

        void ScreenBase::clearScreen()
        {
#ifdef _WIN32
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD coordScreen = {0, 0};
            DWORD cCharsWritten;
            DWORD dwConSize;
            CONSOLE_SCREEN_BUFFER_INFO csbi;

            // Get console screen buffer info
            if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
                return;

            dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

            // Fill the entire screen with blanks
            if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ',
                                            dwConSize, coordScreen, &cCharsWritten))
                return;

            // Get the current text attribute
            if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
                                            dwConSize, coordScreen, &cCharsWritten))
                return;

            // Put the cursor at its home coordinates
            SetConsoleCursorPosition(hConsole, coordScreen);
#else
            std::cout << "\033[2J\033[H" << std::flush;
#endif
        }

        void ScreenBase::clearLine()
        {
#ifdef _WIN32
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(renderContext_.consoleHandle, &csbi);

            DWORD dwConSize = renderContext_.terminalWidth - csbi.dwCursorPosition.X;
            DWORD cCharsWritten;

            FillConsoleOutputCharacter(renderContext_.consoleHandle, (TCHAR)' ',
                                       dwConSize, csbi.dwCursorPosition, &cCharsWritten);

            FillConsoleOutputAttribute(renderContext_.consoleHandle,
                                       csbi.wAttributes,
                                       dwConSize, csbi.dwCursorPosition, &cCharsWritten);
#else
            std::cout << "\033[K";
#endif
        }

        void ScreenBase::hideCursor()
        {
#ifdef _WIN32
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(renderContext_.consoleHandle, &cursorInfo);
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(renderContext_.consoleHandle, &cursorInfo);
#else
            std::cout << "\033[?25l";
#endif
        }

        void ScreenBase::showCursor()
        {
#ifdef _WIN32
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(renderContext_.consoleHandle, &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(renderContext_.consoleHandle, &cursorInfo);
#else
            std::cout << "\033[?25h";
#endif
        }

        void ScreenBase::drawBox(int x, int y, int width, int height, const std::string &title)
        {
            // Draw top border
            moveCursor(x, y);
            std::cout << "+";
            for (int i = 1; i < width - 1; ++i)
                std::cout << "-";
            std::cout << "+";

            // Draw title if provided
            if (!title.empty() && title.length() + 4 < static_cast<size_t>(width))
            {
                moveCursor(x + 2, y);
                std::cout << " " << title << " ";
            }

            // Draw sides
            for (int row = 1; row < height - 1; ++row)
            {
                moveCursor(x, y + row);
                std::cout << "|";
                moveCursor(x + width - 1, y + row);
                std::cout << "|";
            }

            // Draw bottom border
            moveCursor(x, y + height - 1);
            std::cout << "+";
            for (int i = 1; i < width - 1; ++i)
                std::cout << "-";
            std::cout << "+";
        }

        void ScreenBase::drawHorizontalLine(int x, int y, int length, char ch)
        {
            moveCursor(x, y);
            for (int i = 0; i < length; ++i)
                std::cout << ch;
        }

        void ScreenBase::drawVerticalLine(int x, int y, int length, char ch)
        {
            for (int i = 0; i < length; ++i)
            {
                moveCursor(x, y + i);
                std::cout << ch;
            }
        }

        void ScreenBase::printCentered(int y, const std::string &text, Color color)
        {
            setColor(color);
            int x = (renderContext_.terminalWidth - static_cast<int>(text.length())) / 2;
            moveCursor(x, y);
            std::cout << text;
            resetColor();
        }

        void ScreenBase::printAt(int x, int y, const std::string &text, Color color)
        {
            setColor(color);
            moveCursor(x, y);
            std::cout << text;
            resetColor();
        }

        void ScreenBase::logInfo(const std::string &message)
        {
            getLogManager().info(title_, message);
        }

        void ScreenBase::logWarning(const std::string &message)
        {
            getLogManager().warning(title_, message);
        }

        void ScreenBase::logError(const std::string &message)
        {
            getLogManager().error(title_, message);
        }

        void ScreenBase::logDebug(const std::string &message)
        {
            getLogManager().debug(title_, message);
        }

        // ScreenFactory Implementation
        std::unique_ptr<ScreenBase> ScreenFactory::createScreen(ScreenType screenType)
        {
            switch (screenType)
            {
            case ScreenType::Main:
                // return std::make_unique<MainScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::Logs:
                // return std::make_unique<LogScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::Config:
                // return std::make_unique<ConfigScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::Monitor:
                // return std::make_unique<MonitorScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::Graphs:
                // return std::make_unique<GraphsScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::TxTest:
                // return std::make_unique<TxTestScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::RxTest:
                // return std::make_unique<RxTestScreen>(); // Not implemented yet
                return nullptr;
            case ScreenType::Bind:
                return std::make_unique<BindScreen>();
            case ScreenType::Update:
                return std::make_unique<UpdateScreen>();
            case ScreenType::Export:
                return std::make_unique<ExportScreen>();
            case ScreenType::Settings:
                return std::make_unique<SettingsScreen>();
            default:
                break;
            }

            return nullptr; // Screen type not implemented yet
        }

        std::string ScreenFactory::getScreenName(ScreenType screenType)
        {
            switch (screenType)
            {
            case ScreenType::Main:
                return "Main";
            case ScreenType::Logs:
                return "Logs";
            case ScreenType::Config:
                return "Config";
            case ScreenType::Monitor:
                return "Monitor";
            case ScreenType::Graphs:
                return "Graphs";
            case ScreenType::TxTest:
                return "TX Test";
            case ScreenType::RxTest:
                return "RX Test";
            case ScreenType::Bind:
                return "Bind";
            case ScreenType::Update:
                return "Update";
            case ScreenType::Export:
                return "Export";
            case ScreenType::Settings:
                return "Settings";
            default:
                return "Unknown";
            }
        }

    } // namespace UI
} // namespace ELRS