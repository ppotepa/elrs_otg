#include "tui_manager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>
#undef min
#undef max
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

        // LiveGraph Implementation
        LiveGraph::LiveGraph(const std::string &title, int maxPoints)
            : title_(title), maxPoints_(maxPoints), maxValue_(100.0), minValue_(0.0), autoScale_(true)
        {
        }

        void LiveGraph::addDataPoint(double value, Color color)
        {
            GraphData point;
            point.timestamp = std::chrono::steady_clock::now();
            point.value = value;
            point.color = color;

            dataPoints_.push_back(point);

            // Remove old points
            while (dataPoints_.size() > static_cast<size_t>(maxPoints_))
            {
                dataPoints_.pop_front();
            }

            if (autoScale_)
            {
                updateMinMax();
            }
        }

        void LiveGraph::updateMinMax()
        {
            if (dataPoints_.empty())
                return;

            auto minMax = std::minmax_element(dataPoints_.begin(), dataPoints_.end(),
                                              [](const GraphData &a, const GraphData &b)
                                              { return a.value < b.value; });

            minValue_ = minMax.first->value - 5.0; // Add some padding
            maxValue_ = minMax.second->value + 5.0;

            if (maxValue_ - minValue_ < 10.0)
            {
                double center = (maxValue_ + minValue_) / 2.0;
                minValue_ = center - 5.0;
                maxValue_ = center + 5.0;
            }
        }

        char LiveGraph::getGraphChar(double value, double min, double max, int height)
        {
            if (max <= min)
                return ' ';

            double normalized = (value - min) / (max - min);
            int level = static_cast<int>(normalized * (height - 1));
            level = (std::max)(0, (std::min)(height - 1, level));

            // Use different characters for different levels (ASCII compatible)
            const char chars[] = " .:-=+*#@";
            int charIndex = static_cast<int>((normalized * 8));
            charIndex = (std::max)(0, (std::min)(8, charIndex));

            return chars[charIndex];
        }

        void LiveGraph::render(int x, int y, int width, int height)
        {
            // This will be implemented with platform-specific drawing
            // For now, we'll use a simple ASCII representation
        }

        void LiveGraph::clear()
        {
            dataPoints_.clear();
        }

        // TuiManager Implementation
        TuiManager::TuiManager()
            : running_(false), initialized_(false), terminalWidth_(80), terminalHeight_(25), currentScreen_(Screen::Main), radio_state_(&RadioState::getInstance()), connected_to_state_(false), logScrollOffset_(0), maxLogEntries_(100), lastUpdate_(std::chrono::steady_clock::now())
        {
            // Initialize graphs
            txGraph_ = std::make_unique<LiveGraph>("TX Power (dBm)", 200);
            rxGraph_ = std::make_unique<LiveGraph>("RX Signal (dBm)", 200);
            linkQualityGraph_ = std::make_unique<LiveGraph>("Link Quality (%)", 200);

            // Set appropriate ranges
            txGraph_->setMinValue(-30.0);
            txGraph_->setMaxValue(30.0);

            rxGraph_->setMinValue(-120.0);
            rxGraph_->setMaxValue(-30.0);

            linkQualityGraph_->setMinValue(0.0);
            linkQualityGraph_->setMaxValue(100.0);
        }

        TuiManager::~TuiManager()
        {
            cleanup();
        }

        bool TuiManager::initialize()
        {
            if (initialized_)
                return true;

            if (!initializeTerminal())
            {
                return false;
            }

            getTerminalSize(terminalWidth_, terminalHeight_);

            // Initialize default menu items
            addMenuItem(MenuItem(FunctionKey::F1, "Device", "Device Information", [this]()
                                 {
                                     // Show device details
                                 }));

            addMenuItem(MenuItem(FunctionKey::F2, "Graphs", "Live Data Graphs", [this]()
                                 {
                                     // Focus on graphs
                                 }));

            addMenuItem(MenuItem(FunctionKey::F3, "Config", "Configuration", [this]()
                                 {
                                     // Configuration menu
                                 }));

            addMenuItem(MenuItem(FunctionKey::F4, "Monitor", "Signal Monitor", [this]()
                                 {
                                     // Signal monitoring
                                 }));

            addMenuItem(MenuItem(FunctionKey::F5, "TX Test", "Transmitter Test", [this]()
                                 {
                                     // TX testing
                                 }));

            addMenuItem(MenuItem(FunctionKey::F6, "RX Test", "Receiver Test", [this]()
                                 {
                                     // RX testing
                                 }));

            addMenuItem(MenuItem(FunctionKey::F7, "Binding", "Bind Mode", [this]()
                                 {
                                     // Binding mode
                                 }));

            addMenuItem(MenuItem(FunctionKey::F8, "Update", "Firmware Update", [this]()
                                 {
                                     // Firmware update
                                 }));

            addMenuItem(MenuItem(FunctionKey::F9, "Log", "Event Log", [this]()
                                 { switchToScreen(Screen::Logs); }));

            addMenuItem(MenuItem(FunctionKey::F10, "Export", "Export Data", [this]()
                                 {
                                     // Data export
                                 }));

            addMenuItem(MenuItem(FunctionKey::F11, "Settings", "System Settings", [this]()
                                 {
                                     // System settings
                                 }));

            addMenuItem(MenuItem(FunctionKey::F12, "Exit", "Exit Application", [this]()
                                 { stop(); }));

            // Connect to RadioState for automatic updates
            connectToRadioState();

            initialized_ = true;
            return true;
        }

        void TuiManager::cleanup()
        {
            if (initialized_)
            {
                disconnectFromRadioState();
                restoreTerminal();
                initialized_ = false;
            }
        }

        void TuiManager::run()
        {
            if (!initialized_)
            {
                if (!initialize())
                {
                    std::cerr << "Failed to initialize TUI" << std::endl;
                    return;
                }
            }

            running_ = true;
            hideCursor();
            clearScreen();

            while (running_)
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

                if (elapsed.count() >= UPDATE_INTERVAL_MS)
                {
                    refreshScreen();
                    lastUpdate_ = now;
                }

                // Check for key presses (non-blocking)
                FunctionKey key = getKeyPress();
                if (key != FunctionKey::Unknown)
                {
                    if (!handleKeyPress(key))
                    {
                        break; // Exit requested
                    }
                }

                // Small sleep to prevent excessive CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            showCursor();
            clearScreen();
            moveCursor(0, 0);
        }

        void TuiManager::stop()
        {
            running_ = false;
        }

        void TuiManager::connectToRadioState()
        {
            if (!connected_to_state_)
            {
                radio_state_->subscribeToChanges([this]()
                                                 {
                                                     // This callback is triggered whenever RadioState changes
                                                     // The TUI will automatically refresh on the next update cycle
                                                 });
                connected_to_state_ = true;
            }
        }

        void TuiManager::disconnectFromRadioState()
        {
            if (connected_to_state_)
            {
                radio_state_->unsubscribeFromChanges();
                connected_to_state_ = false;
            }
        }

        void TuiManager::addTxData(double value)
        {
            txGraph_->addTxDataPoint(value);
        }

        void TuiManager::addRxData(double value)
        {
            rxGraph_->addRxDataPoint(value);
        }

        void TuiManager::addMenuItem(const MenuItem &item)
        {
            menuItems_[item.key] = item;
        }

        void TuiManager::removeMenuItem(FunctionKey key)
        {
            menuItems_.erase(key);
        }

        void TuiManager::enableMenuItem(FunctionKey key, bool enabled)
        {
            auto it = menuItems_.find(key);
            if (it != menuItems_.end())
            {
                it->second.enabled = enabled;
            }
        }

        void TuiManager::clearScreen()
        {
#ifdef _WIN32
            COORD coord = {0, 0};
            DWORD written;
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole_, &csbi);
            FillConsoleOutputCharacterA(hConsole_, ' ', csbi.dwSize.X * csbi.dwSize.Y, coord, &written);
            FillConsoleOutputAttribute(hConsole_, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &written);
            SetConsoleCursorPosition(hConsole_, coord);
#else
            std::cout << "\033[2J\033[H";
            std::cout.flush();
#endif
        }

        void TuiManager::refreshScreen()
        {
            moveCursor(0, 0);
            renderHeader();

            switch (currentScreen_)
            {
            case Screen::Main:
                renderDeviceInfo();
                renderGraphs();
                break;
            case Screen::Logs:
                renderLogScreen();
                break;
            default:
                renderDeviceInfo();
                renderGraphs();
                break;
            }

            renderFooter();
            renderStatusBar();

#ifdef _WIN32
            // Force update on Windows
            std::cout.flush();
#else
            std::cout.flush();
#endif
        }

        bool TuiManager::initializeTerminal()
        {
#ifdef _WIN32
            hConsole_ = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hConsole_ == INVALID_HANDLE_VALUE)
                return false;

            // Save original console state
            GetConsoleScreenBufferInfo(hConsole_, &originalConsoleInfo_);
            GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &originalConsoleMode_);

            // Set console mode for raw input
            DWORD newMode = originalConsoleMode_ & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), newMode);

            // Set console output mode to handle colors better
            DWORD outputMode;
            GetConsoleMode(hConsole_, &outputMode);
            outputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole_, outputMode);

            // Set console code page to UTF-8 for better character support
            SetConsoleOutputCP(CP_UTF8);

            return true;
#else
            // Save original terminal settings
            if (tcgetattr(STDIN_FILENO, &originalTermios_) != 0)
                return false;

            struct termios newTermios = originalTermios_;
            // Set raw mode
            newTermios.c_lflag &= ~(ICANON | ECHO);
            newTermios.c_cc[VMIN] = 0;
            newTermios.c_cc[VTIME] = 0;

            if (tcsetattr(STDIN_FILENO, TCSANOW, &newTermios) != 0)
                return false;

            return true;
#endif
        }

        void TuiManager::restoreTerminal()
        {
#ifdef _WIN32
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), originalConsoleMode_);
#else
            tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios_);
#endif
        }

        void TuiManager::getTerminalSize(int &width, int &height)
        {
#ifdef _WIN32
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole_, &csbi);
            width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            width = w.ws_col;
            height = w.ws_row;
#endif
        }

        FunctionKey TuiManager::getKeyPress()
        {
#ifdef _WIN32
            if (!_kbhit())
                return FunctionKey::Unknown;

            int key = _getch();

            if (key == 0 || key == 224) // Extended key
            {
                key = _getch();
                switch (key)
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
                default:
                    return FunctionKey::Unknown;
                }
            }
            else
            {
                switch (key)
                {
                case 27:
                    return FunctionKey::Escape;
                case 13:
                    return FunctionKey::Enter;
                case ' ':
                    return FunctionKey::Space;
                case '\t':
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

            if (select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout) <= 0)
            {
                return FunctionKey::Unknown;
            }

            char buffer[8];
            ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (bytes <= 0)
                return FunctionKey::Unknown;

            if (bytes == 1)
            {
                switch (buffer[0])
                {
                case 27:
                    return FunctionKey::Escape;
                case '\n':
                case '\r':
                    return FunctionKey::Enter;
                case ' ':
                    return FunctionKey::Space;
                case '\t':
                    return FunctionKey::Tab;
                case 127:
                case 8:
                    return FunctionKey::Backspace;
                default:
                    return FunctionKey::Unknown;
                }
            }
            else if (bytes >= 3 && buffer[0] == 27 && buffer[1] == '[')
            {
                if (bytes >= 4 && buffer[2] == '1')
                {
                    switch (buffer[3])
                    {
                    case '1':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F1 : FunctionKey::Unknown;
                    case '2':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F2 : FunctionKey::Unknown;
                    case '3':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F3 : FunctionKey::Unknown;
                    case '4':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F4 : FunctionKey::Unknown;
                    case '5':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F5 : FunctionKey::Unknown;
                    case '7':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F6 : FunctionKey::Unknown;
                    case '8':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F7 : FunctionKey::Unknown;
                    case '9':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F8 : FunctionKey::Unknown;
                    default:
                        return FunctionKey::Unknown;
                    }
                }
                else if (bytes >= 4 && buffer[2] == '2')
                {
                    switch (buffer[3])
                    {
                    case '0':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F9 : FunctionKey::Unknown;
                    case '1':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F10 : FunctionKey::Unknown;
                    case '3':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F11 : FunctionKey::Unknown;
                    case '4':
                        return (bytes > 4 && buffer[4] == '~') ? FunctionKey::F12 : FunctionKey::Unknown;
                    default:
                        return FunctionKey::Unknown;
                    }
                }
                else
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
                    default:
                        return FunctionKey::Unknown;
                    }
                }
            }

            return FunctionKey::Unknown;
#endif
        }

        bool TuiManager::handleKeyPress(FunctionKey key)
        {
            // Handle screen-specific navigation first
            if (currentScreen_ == Screen::Logs)
            {
                switch (key)
                {
                case FunctionKey::Escape:
                case FunctionKey::F1:
                    switchToScreen(Screen::Main);
                    return true;
                case FunctionKey::ArrowUp:
                    if (logScrollOffset_ < 50) // Limit scroll
                        logScrollOffset_++;
                    return true;
                case FunctionKey::ArrowDown:
                    if (logScrollOffset_ > 0)
                        logScrollOffset_--;
                    return true;
                default:
                    break;
                }
            }

            auto it = menuItems_.find(key);
            if (it != menuItems_.end() && it->second.enabled)
            {
                it->second.callback();
                return key != FunctionKey::F12; // F12 is exit
            }

            // Handle other keys
            switch (key)
            {
            case FunctionKey::Escape:
                return false; // Exit
            default:
                break;
            }

            return true;
        }

        void TuiManager::renderHeader()
        {
            setColor(Color::BrightWhite, Color::Blue);

            // Create header line
            std::string header = " ELRS OTG Monitor v1.0 - ExpressLRS 2.4GHz Radio Interface ";
            int padding = terminalWidth_ - static_cast<int>(header.length());
            if (padding > 0)
            {
                header += std::string(padding, ' ');
            }
            else if (static_cast<int>(header.length()) > terminalWidth_)
            {
                header = header.substr(0, terminalWidth_);
            }

            moveCursor(0, 0);
            std::cout << header;

            resetColor();
        }

        void TuiManager::renderDeviceInfo()
        {
            int startY = 2;
            int leftCol = 2;
            int rightCol = terminalWidth_ / 2 + 2;

            // Get current state from RadioState
            auto deviceConfig = radio_state_->getDeviceConfiguration();
            auto telemetry = radio_state_->getLiveTelemetry();
            auto connectionStatus = radio_state_->getConnectionStatusString();
            auto lastUpdate = radio_state_->getLastUpdateTimeString();

            setColor(Color::BrightYellow);
            moveCursor(leftCol, startY);
            std::cout << "+--- Device Information -----------------------+";

            setColor(Color::White);
            moveCursor(leftCol, startY + 1);
            std::cout << "| Product    : " << std::left << std::setw(30) << deviceConfig.productName << "|";

            moveCursor(leftCol, startY + 2);
            std::cout << "| Manufacturer: " << std::left << std::setw(30) << deviceConfig.manufacturer << "|";

            moveCursor(leftCol, startY + 3);
            std::cout << "| Serial     : " << std::left << std::setw(30) << deviceConfig.serialNumber << "|";

            moveCursor(leftCol, startY + 4);
            std::string vidPid = std::to_string(deviceConfig.vid) + ":" + std::to_string(deviceConfig.pid);
            std::cout << "| VID:PID    : " << std::left << std::setw(30) << vidPid << "|";

            moveCursor(leftCol, startY + 5);
            std::cout << "| Status     : " << std::left << std::setw(30) << connectionStatus << "|";

            setColor(Color::BrightYellow);
            moveCursor(leftCol, startY + 6);
            std::cout << "+-----------------------------------------------+";

            // Connection Statistics
            setColor(Color::BrightCyan);
            moveCursor(rightCol, startY);
            std::cout << "+--- Connection Statistics -------------------+";

            setColor(Color::White);
            moveCursor(rightCol, startY + 1);
            std::cout << "| Packets RX : " << std::right << std::setw(30) << telemetry.packetsReceived << "|";

            moveCursor(rightCol, startY + 2);
            std::cout << "| Packets TX : " << std::right << std::setw(30) << telemetry.packetsTransmitted << "|";

            moveCursor(rightCol, startY + 3);
            std::cout << "| Link Quality: " << std::right << std::setw(29) << telemetry.linkQuality << "%|";

            moveCursor(rightCol, startY + 4);
            std::cout << "| Signal     : " << std::right << std::setw(27) << telemetry.rssi1 << "dBm|";

            moveCursor(rightCol, startY + 5);
            std::cout << "| Last Update: " << std::left << std::setw(29) << lastUpdate << "|";

            setColor(Color::BrightCyan);
            moveCursor(rightCol, startY + 6);
            std::cout << "+-----------------------------------------------+";

            resetColor();
        }

        void TuiManager::renderGraphs()
        {
            int graphStartY = 10;
            int graphHeight = terminalHeight_ - graphStartY - 6; // Leave space for footer
            int graphWidth = (terminalWidth_ - 6) / 2;

            // TX Graph
            setColor(Color::BrightGreen);
            moveCursor(2, graphStartY);
            std::cout << "+--- TX Power (dBm) " << std::string(graphWidth - 20, '-') << "+";

            // Simple ASCII graph representation
            renderSimpleGraph(2, graphStartY + 1, graphWidth, graphHeight - 2, txGraph_.get());

            setColor(Color::BrightGreen);
            moveCursor(2, graphStartY + graphHeight - 1);
            std::cout << "+" << std::string(graphWidth, '-') << "+";

            // RX Graph
            setColor(Color::BrightRed);
            moveCursor(graphWidth + 4, graphStartY);
            std::cout << "+--- RX Signal (dBm) " << std::string(graphWidth - 21, '-') << "+";

            renderSimpleGraph(graphWidth + 4, graphStartY + 1, graphWidth, graphHeight - 2, rxGraph_.get());

            setColor(Color::BrightRed);
            moveCursor(graphWidth + 4, graphStartY + graphHeight - 1);
            std::cout << "+" << std::string(graphWidth, '-') << "+";

            resetColor();
        }

        void TuiManager::renderSimpleGraph(int x, int y, int width, int height, LiveGraph *graph)
        {
            if (!graph)
                return;

            setColor(Color::White);

            // Get real data from RadioState for graphs
            std::vector<int> history;
            Color graphColor = Color::White;

            if (graph == txGraph_.get())
            {
                history = radio_state_->getTxPowerHistory(width - 2);
                graphColor = Color::BrightGreen;
            }
            else if (graph == rxGraph_.get())
            {
                history = radio_state_->getRSSIHistory(width - 2);
                graphColor = Color::BrightRed;
            }
            else if (graph == linkQualityGraph_.get())
            {
                history = radio_state_->getLinkQualityHistory(width - 2);
                graphColor = Color::BrightBlue;
            }

            // Render graph area with real data
            for (int row = 0; row < height; ++row)
            {
                moveCursor(x, y + row);
                std::cout << "|";

                for (int col = 1; col < width - 1; ++col)
                {
                    bool showPoint = false;

                    if (!history.empty() && col - 1 < static_cast<int>(history.size()))
                    {
                        int dataIndex = col - 1;
                        if (dataIndex < static_cast<int>(history.size()))
                        {
                            int value = history[dataIndex];

                            // Normalize value based on graph type
                            double normalizedValue = 0.0;
                            if (graph == txGraph_.get())
                            {
                                // TX Power: -30 to +30 dBm
                                normalizedValue = (value + 30.0) / 60.0;
                            }
                            else if (graph == rxGraph_.get())
                            {
                                // RX Signal: -120 to -30 dBm
                                normalizedValue = (value + 120.0) / 90.0;
                            }
                            else if (graph == linkQualityGraph_.get())
                            {
                                // Link Quality: 0 to 100%
                                normalizedValue = value / 100.0;
                            }

                            normalizedValue = (std::max)(0.0, (std::min)(1.0, normalizedValue));
                            int targetRow = height - 1 - static_cast<int>(normalizedValue * (height - 1));

                            if (std::abs(row - targetRow) <= 1)
                            {
                                showPoint = true;
                            }
                        }
                    }

                    if (showPoint)
                    {
                        setColor(graphColor);
                        std::cout << "*";
                        setColor(Color::White);
                    }
                    else
                    {
                        std::cout << " ";
                    }
                }

                std::cout << "|";
            }
        }

        void TuiManager::renderFooter()
        {
            int footerY = terminalHeight_ - 2;

            setColor(Color::BrightWhite, Color::Blue);
            moveCursor(0, footerY);

            // Function keys menu
            std::string footer = "";
            std::vector<std::pair<FunctionKey, std::string>> keys = {
                {FunctionKey::F1, "Device"}, {FunctionKey::F2, "Graphs"}, {FunctionKey::F3, "Config"}, {FunctionKey::F4, "Monitor"}, {FunctionKey::F5, "TX Test"}, {FunctionKey::F6, "RX Test"}, {FunctionKey::F7, "Bind"}, {FunctionKey::F8, "Update"}, {FunctionKey::F9, "Log"}, {FunctionKey::F10, "Export"}, {FunctionKey::F11, "Settings"}, {FunctionKey::F12, "Exit"}};

            for (const auto &keyPair : keys)
            {
                int keyNum = static_cast<int>(keyPair.first);
                footer += "F" + std::to_string(keyNum) + ":" + keyPair.second + " ";
            }

            // Truncate if too long
            if (static_cast<int>(footer.length()) > terminalWidth_)
            {
                footer = footer.substr(0, terminalWidth_ - 3) + "...";
            }

            // Pad to full width
            footer += std::string(terminalWidth_ - footer.length(), ' ');

            std::cout << footer;
            resetColor();
        }

        void TuiManager::renderStatusBar()
        {
            int statusY = terminalHeight_ - 1;

            setColor(Color::Black, Color::BrightWhite);
            moveCursor(0, statusY);

            // Get current state from RadioState
            auto connectionStatus = radio_state_->getConnectionStatusString();
            auto linkQuality = radio_state_->getLinkQuality();
            auto currentTime = radio_state_->getLastUpdateTimeString();
            auto uptime = radio_state_->getUptimeString();

            std::ostringstream status;
            status << " " << connectionStatus << " | Link Quality: " << linkQuality << "% | "
                   << currentTime << " | Uptime: " << uptime;

            std::string statusStr = status.str();
            if (statusStr.length() > static_cast<size_t>(terminalWidth_))
            {
                statusStr = statusStr.substr(0, terminalWidth_ - 3) + "...";
            }
            statusStr += std::string(terminalWidth_ - statusStr.length(), ' ');

            std::cout << statusStr;
            resetColor();
        }

        void TuiManager::moveCursor(int x, int y)
        {
#ifdef _WIN32
            COORD coord = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
            SetConsoleCursorPosition(hConsole_, coord);
#else
            std::cout << "\033[" << (y + 1) << ";" << (x + 1) << "H";
#endif
        }

        void TuiManager::setColor(Color fg, Color bg)
        {
#ifdef _WIN32
            WORD attributes = static_cast<WORD>(fg) | (static_cast<WORD>(bg) << 4);
            SetConsoleTextAttribute(hConsole_, attributes);
#else
            std::cout << "\033[" << (30 + static_cast<int>(fg)) << ";" << (40 + static_cast<int>(bg)) << "m";
#endif
        }

        void TuiManager::resetColor()
        {
#ifdef _WIN32
            SetConsoleTextAttribute(hConsole_, originalConsoleInfo_.wAttributes);
#else
            std::cout << "\033[0m";
#endif
        }

        void TuiManager::hideCursor()
        {
#ifdef _WIN32
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hConsole_, &cursorInfo);
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(hConsole_, &cursorInfo);
#else
            std::cout << "\033[?25l";
#endif
        }

        void TuiManager::showCursor()
        {
#ifdef _WIN32
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hConsole_, &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(hConsole_, &cursorInfo);
#else
            std::cout << "\033[?25h";
#endif
        }

        void TuiManager::switchToScreen(Screen screen)
        {
            currentScreen_ = screen;
            logScrollOffset_ = 0; // Reset scroll when switching screens
            clearScreen();
        }

        void TuiManager::renderLogScreen()
        {
            // Get recent logs
            auto logs = LogManager::getInstance().getRecentLogs(maxLogEntries_);

            int contentStartY = 3;                   // After header
            int contentHeight = terminalHeight_ - 5; // Leave space for footer and status
            int contentWidth = terminalWidth_ - 4;

            // Render log screen header
            moveCursor(2, contentStartY);
            setColor(Color::BrightYellow);
            std::cout << "+--- System Logs ";
            for (int i = 16; i < contentWidth; ++i)
                std::cout << "-";
            std::cout << "+";

            // Instructions
            moveCursor(2, contentStartY + 1);
            setColor(Color::BrightBlue);
            std::cout << "| Navigation: UP/DOWN to scroll, ESC or F1 to return to main screen";
            for (int i = 67; i < contentWidth; ++i)
                std::cout << " ";
            std::cout << "|";

            moveCursor(2, contentStartY + 2);
            setColor(Color::White);
            std::cout << "+";
            for (int i = 1; i < contentWidth; ++i)
                std::cout << "-";
            std::cout << "+";

            // Calculate visible log range
            int maxVisibleLogs = contentHeight - 3; // Account for headers
            int totalLogs = static_cast<int>(logs.size());
            int startIndex = (std::max)(0, totalLogs - maxVisibleLogs - logScrollOffset_);
            int endIndex = (std::min)(totalLogs, startIndex + maxVisibleLogs);

            // Render logs
            for (int i = 0; i < maxVisibleLogs; ++i)
            {
                int logIndex = startIndex + i;
                moveCursor(2, contentStartY + 3 + i);

                if (logIndex < endIndex && logIndex >= 0)
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

                    // Format: [HH:MM:SS] [LEVEL] [CATEGORY] Message
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
                    for (int j = 1; j < contentWidth; ++j)
                        std::cout << " ";
                    std::cout << "|";
                }
            }

            // Bottom border
            moveCursor(2, contentStartY + 3 + maxVisibleLogs);
            setColor(Color::White);
            std::cout << "+";
            for (int i = 1; i < contentWidth; ++i)
                std::cout << "-";
            std::cout << "+";

            // Scroll indicator
            if (totalLogs > maxVisibleLogs)
            {
                moveCursor(contentWidth - 15, contentStartY + 3 + maxVisibleLogs);
                setColor(Color::BrightBlue);
                std::cout << " [" << (totalLogs - endIndex) << " more] ";
            }

            resetColor();
        }

    } // namespace UI
} // namespace ELRS