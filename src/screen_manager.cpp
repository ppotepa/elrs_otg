#include "screen_manager.h"
#include "log_manager.h"
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#endif

namespace ELRS
{
    namespace UI
    {
        ScreenManager::ScreenManager()
            : currentScreen_(nullptr), running_(false), initialized_(false), lastUpdate_(std::chrono::steady_clock::now())
#ifdef _WIN32
              ,
              hConsole_(INVALID_HANDLE_VALUE), originalConsoleMode_(0)
#endif
        {
            // Set up navigation context
            navContext_.switchToScreen = [this](ScreenType screen)
            { switchToScreen(screen); };
            navContext_.exitApplication = [this]()
            { exit(); };
        }

        ScreenManager::~ScreenManager()
        {
            shutdown();
        }

        bool ScreenManager::initialize()
        {
            LOG_INFO("SCREEN_MGR", "Initializing screen manager");

            initializeTerminal();
            updateTerminalSize();

            // Create and initialize all screens
            std::vector<ScreenType> screenTypes = {
                ScreenType::Main, ScreenType::Logs, ScreenType::Config, ScreenType::Monitor,
                ScreenType::Graphs, ScreenType::TxTest, ScreenType::RxTest, ScreenType::Bind,
                ScreenType::Update, ScreenType::Export, ScreenType::Settings};

            for (ScreenType screenType : screenTypes)
            {
                auto screen = ScreenFactory::createScreen(screenType);
                if (!screen)
                {
                    LOG_ERROR("SCREEN_MGR", "Failed to create screen: " + ScreenFactory::getScreenName(screenType));
                    return false;
                }

                if (!screen->initialize(renderContext_, navContext_))
                {
                    LOG_ERROR("SCREEN_MGR", "Failed to initialize screen: " + ScreenFactory::getScreenName(screenType));
                    return false;
                }

                screens_[screenType] = std::move(screen);
            }

            // Set initial screen
            currentScreen_ = screens_[ScreenType::Main].get();
            navContext_.currentScreen = ScreenType::Main;
            navContext_.previousScreen = ScreenType::Main;

            if (currentScreen_)
            {
                currentScreen_->onActivate();
            }

            initialized_ = true;
            LOG_INFO("SCREEN_MGR", "Screen manager initialized successfully");
            return true;
        }

        void ScreenManager::run()
        {
            if (!initialized_)
            {
                LOG_ERROR("SCREEN_MGR", "Screen manager not initialized");
                return;
            }

            LOG_INFO("SCREEN_MGR", "Starting screen manager main loop");
            running_ = true;

            while (running_)
            {
                auto now = std::chrono::steady_clock::now();
                auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

                if (deltaTime.count() >= UPDATE_INTERVAL_MS)
                {
                    // Update current screen
                    if (currentScreen_)
                    {
                        currentScreen_->update(deltaTime);

                        // Only render if the screen needs refresh
                        if (currentScreen_->needsRefresh())
                        {
                            currentScreen_->render(renderContext_);
                            currentScreen_->clearRefreshFlag();
                        }
                    }

                    lastUpdate_ = now;
                }

                // Handle input (non-blocking)
                FunctionKey key = readKey();
                if (key != FunctionKey::Unknown)
                {
                    LOG_INFO("SCREEN_MGR", "Key pressed: " + std::to_string(static_cast<int>(key)));

                    // Let current screen handle the key first
                    bool handled = false;
                    if (currentScreen_)
                    {
                        handled = currentScreen_->handleKeyPress(key);
                        LOG_INFO("SCREEN_MGR", "Screen handled key: " + std::string(handled ? "true" : "false"));
                    }

                    // If not handled, process global keys
                    if (!handled)
                    {
                        LOG_INFO("SCREEN_MGR", "Processing global key");
                        handleGlobalKeys(key);
                    }
                }

                // Small sleep to prevent 100% CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            LOG_INFO("SCREEN_MGR", "Screen manager main loop ended");
        }

        void ScreenManager::shutdown()
        {
            if (!initialized_)
                return;

            LOG_INFO("SCREEN_MGR", "Shutting down screen manager");

            running_ = false;

            // Deactivate current screen
            if (currentScreen_)
            {
                currentScreen_->onDeactivate();
                currentScreen_ = nullptr;
            }

            // Cleanup all screens
            for (auto &pair : screens_)
            {
                if (pair.second)
                {
                    pair.second->cleanup();
                }
            }
            screens_.clear();

            cleanupTerminal();
            initialized_ = false;

            LOG_INFO("SCREEN_MGR", "Screen manager shutdown complete");
        }

        void ScreenManager::switchToScreen(ScreenType screenType)
        {
            auto it = screens_.find(screenType);
            if (it == screens_.end() || !it->second)
            {
                LOG_WARNING("SCREEN_MGR", "Attempted to switch to non-existent screen: " +
                                              ScreenFactory::getScreenName(screenType));
                return;
            }

            // Deactivate current screen
            if (currentScreen_)
            {
                currentScreen_->onDeactivate();
                screenHistory_.push(navContext_.currentScreen);
            }

            // Activate new screen
            navContext_.previousScreen = navContext_.currentScreen;
            navContext_.currentScreen = screenType;
            currentScreen_ = it->second.get();

            if (currentScreen_)
            {
                currentScreen_->onActivate();
            }

            LOG_INFO("SCREEN_MGR", "Switched to screen: " + ScreenFactory::getScreenName(screenType));
        }

        void ScreenManager::goBack()
        {
            if (!screenHistory_.empty())
            {
                ScreenType previousScreen = screenHistory_.top();
                screenHistory_.pop();
                switchToScreen(previousScreen);
            }
        }

        void ScreenManager::exit()
        {
            LOG_INFO("SCREEN_MGR", "Exit requested");
            running_ = false;
        }

        void ScreenManager::initializeTerminal()
        {
#ifdef _WIN32
            hConsole_ = GetStdHandle(STD_OUTPUT_HANDLE);
            renderContext_.consoleHandle = hConsole_;

            // Save original console state
            GetConsoleScreenBufferInfo(hConsole_, &originalConsoleInfo_);
            GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &originalConsoleMode_);

            // Set console mode for raw input
            DWORD newMode = originalConsoleMode_ & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), newMode);

            // Enable virtual terminal processing for colors
            DWORD outputMode;
            GetConsoleMode(hConsole_, &outputMode);
            outputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole_, outputMode);

            // Hide cursor
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hConsole_, &cursorInfo);
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(hConsole_, &cursorInfo);
#else
            // Save original terminal settings
            tcgetattr(STDIN_FILENO, &originalTermios_);

            // Set raw mode
            struct termios raw = originalTermios_;
            raw.c_lflag &= ~(ECHO | ICANON);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

            // Hide cursor
            std::cout << "\033[?25l";
#endif
        }

        void ScreenManager::cleanupTerminal()
        {
#ifdef _WIN32
            // Restore original console settings
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), originalConsoleMode_);
            SetConsoleTextAttribute(hConsole_, originalConsoleInfo_.wAttributes);

            // Show cursor
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hConsole_, &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(hConsole_, &cursorInfo);
#else
            // Restore original terminal settings
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios_);

            // Show cursor
            std::cout << "\033[?25h";
#endif

            // Clear screen and reset cursor
            std::cout << "\033[2J\033[H";
        }

        void ScreenManager::updateTerminalSize()
        {
#ifdef _WIN32
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole_, &csbi);
            renderContext_.terminalWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            renderContext_.terminalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
            struct winsize ws;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
            {
                renderContext_.terminalWidth = ws.ws_col;
                renderContext_.terminalHeight = ws.ws_row;
            }
#endif

            // Update content area
            renderContext_.contentStartX = 0;
            renderContext_.contentStartY = 3;
            renderContext_.contentWidth = renderContext_.terminalWidth;
            renderContext_.contentHeight = renderContext_.terminalHeight - 5; // Leave space for header and footer
        }

        FunctionKey ScreenManager::readKey()
        {
#ifdef _WIN32
            if (!_kbhit())
                return FunctionKey::Unknown;

            int ch = _getch();
            std::cout << "\n[DEBUG] Raw key code: " << ch << std::endl;

            if (ch == 0 || ch == 224) // Extended key
            {
                ch = _getch();
                switch (ch)
                {
                case 59:
                    return FunctionKey::F1;
                case 60:
                    return FunctionKey::F2;
                case 61:
                    return FunctionKey::F3;
                case 62:
                    return FunctionKey::F4;
                case 63:
                    return FunctionKey::F5;
                case 64:
                    return FunctionKey::F6;
                case 65:
                    return FunctionKey::F7;
                case 66:
                    return FunctionKey::F8;
                case 67:
                    return FunctionKey::F9;
                case 68:
                    return FunctionKey::F10;
                case 133:
                    return FunctionKey::F11;
                case 134:
                    return FunctionKey::F12;
                case 72:
                    return FunctionKey::ArrowUp;
                case 80:
                    return FunctionKey::ArrowDown;
                case 75:
                    return FunctionKey::ArrowLeft;
                case 77:
                    return FunctionKey::ArrowRight;
                case 73:
                    return FunctionKey::PageUp;
                case 81:
                    return FunctionKey::PageDown;
                case 71:
                    return FunctionKey::Home;
                case 79:
                    return FunctionKey::End;
                default:
                    return FunctionKey::Unknown;
                }
            }
            else
            {
                switch (ch)
                {
                case 27:
                    return FunctionKey::Escape;
                case 13:
                    return FunctionKey::Enter;
                case 32:
                    return FunctionKey::Space;
                case 9:
                    return FunctionKey::Tab;
                case 8:
                    return FunctionKey::Backspace;
                default:
                    return FunctionKey::Unknown;
                }
            }
#else
            fd_set readfds;
            struct timeval timeout;

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) <= 0)
                return FunctionKey::Unknown;

            char buffer[8];
            int bytes = read(STDIN_FILENO, buffer, sizeof(buffer));

            if (bytes <= 0)
                return FunctionKey::Unknown;

            if (bytes == 1)
            {
                switch (buffer[0])
                {
                case 27:
                    return FunctionKey::Escape;
                case 13:
                    return FunctionKey::Enter;
                case 32:
                    return FunctionKey::Space;
                case 9:
                    return FunctionKey::Tab;
                case 127:
                    return FunctionKey::Backspace;
                default:
                    return FunctionKey::Unknown;
                }
            }
            else if (bytes >= 3 && buffer[0] == 27 && buffer[1] == '[')
            {
                switch (buffer[2])
                {
                case 'A':
                    return FunctionKey::ArrowUp;
                case 'B':
                    return FunctionKey::ArrowDown;
                case 'C':
                    return FunctionKey::ArrowRight;
                case 'D':
                    return FunctionKey::ArrowLeft;
                case 'H':
                    return FunctionKey::Home;
                case 'F':
                    return FunctionKey::End;
                default:
                    if (bytes >= 4)
                    {
                        if (buffer[3] == '~')
                        {
                            switch (buffer[2])
                            {
                            case '5':
                                return FunctionKey::PageUp;
                            case '6':
                                return FunctionKey::PageDown;
                            default:
                                return FunctionKey::Unknown;
                            }
                        }
                    }
                    return FunctionKey::Unknown;
                }
            }
            else if (bytes >= 4 && buffer[0] == 27 && buffer[1] == 'O')
            {
                switch (buffer[2])
                {
                case 'P':
                    return FunctionKey::F1;
                case 'Q':
                    return FunctionKey::F2;
                case 'R':
                    return FunctionKey::F3;
                case 'S':
                    return FunctionKey::F4;
                default:
                    return FunctionKey::Unknown;
                }
            }

            return FunctionKey::Unknown;
#endif
        }

        void ScreenManager::handleGlobalKeys(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::F1:
                switchToScreen(ScreenType::Main);
                break;
            case FunctionKey::F2:
                switchToScreen(ScreenType::Graphs);
                break;
            case FunctionKey::F3:
                switchToScreen(ScreenType::Config);
                break;
            case FunctionKey::F4:
                switchToScreen(ScreenType::Monitor);
                break;
            case FunctionKey::F5:
                switchToScreen(ScreenType::TxTest);
                break;
            case FunctionKey::F6:
                switchToScreen(ScreenType::RxTest);
                break;
            case FunctionKey::F7:
                switchToScreen(ScreenType::Bind);
                break;
            case FunctionKey::F8:
                switchToScreen(ScreenType::Update);
                break;
            case FunctionKey::F9:
                switchToScreen(ScreenType::Logs);
                break;
            case FunctionKey::F10:
                switchToScreen(ScreenType::Export);
                break;
            case FunctionKey::F11:
                switchToScreen(ScreenType::Settings);
                break;
            case FunctionKey::F12:
                exit();
                break;
            default:
                // Key not handled globally
                break;
            }
        }

    } // namespace UI
} // namespace ELRS