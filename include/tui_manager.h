#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <deque>
#include <map>
#include "radio_state.h"
#include "log_manager.h"

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

        /**
         * Terminal colors for TUI
         */
        enum class Color
        {
            Black = 0,
            Red = 1,
            Green = 2,
            Yellow = 3,
            Blue = 4,
            Magenta = 5,
            Cyan = 6,
            White = 7,
            BrightBlack = 8,
            BrightRed = 9,
            BrightGreen = 10,
            BrightYellow = 11,
            BrightBlue = 12,
            BrightMagenta = 13,
            BrightCyan = 14,
            BrightWhite = 15
        };

        /**
         * Function key enumeration
         */
        enum class FunctionKey
        {
            F1 = 1,
            F2,
            F3,
            F4,
            F5,
            F6,
            F7,
            F8,
            F9,
            F10,
            F11,
            F12,
            Escape,
            Enter,
            Space,
            Tab,
            Backspace,
            ArrowUp,
            ArrowDown,
            ArrowLeft,
            ArrowRight,
            Unknown
        };

        /**
         * Screen enumeration
         */
        enum class Screen
        {
            Main = 0,
            Logs,
            Config,
            Monitor
        };

        /**
         * Data point for live graphs
         */
        struct GraphData
        {
            std::chrono::steady_clock::time_point timestamp;
            double value;
            Color color;
        };

        /**
         * Live graph widget for TX/RX data
         */
        class LiveGraph
        {
        public:
            LiveGraph(const std::string &title, int maxPoints = 100);

            void addDataPoint(double value, Color color = Color::Green);
            void addTxDataPoint(double value) { addDataPoint(value, Color::BrightGreen); }
            void addRxDataPoint(double value) { addDataPoint(value, Color::BrightRed); }

            void render(int x, int y, int width, int height);
            void clear();

            void setTitle(const std::string &title) { title_ = title; }
            void setMaxValue(double maxValue) { maxValue_ = maxValue; }
            void setMinValue(double minValue) { minValue_ = minValue; }

        private:
            std::string title_;
            std::deque<GraphData> dataPoints_;
            int maxPoints_;
            double maxValue_;
            double minValue_;
            bool autoScale_;

            void updateMinMax();
            char getGraphChar(double value, double min, double max, int height);
        };

        /**
         * Menu item structure
         */
        struct MenuItem
        {
            FunctionKey key;
            std::string label;
            std::string description;
            std::function<void()> callback;
            bool enabled;

            MenuItem() : key(FunctionKey::Unknown), label(""), description(""), callback(nullptr), enabled(false) {}

            MenuItem(FunctionKey k, const std::string &l, const std::string &d,
                     std::function<void()> cb, bool en = true)
                : key(k), label(l), description(d), callback(cb), enabled(en) {}
        };

        /**
         * Main TUI Manager class
         */
        class TuiManager
        {
        public:
            TuiManager();
            ~TuiManager();

            // Initialization and cleanup
            bool initialize();
            void cleanup();

            // Main TUI loop
            void run();
            void stop();

            // State management (React-like)
            void connectToRadioState();
            void disconnectFromRadioState();

            // Legacy compatibility (deprecated - use RadioState instead)
            void addTxData(double value);
            void addRxData(double value);

            // Menu management
            void addMenuItem(const MenuItem &item);
            void removeMenuItem(FunctionKey key);
            void enableMenuItem(FunctionKey key, bool enabled);

            // Screen management
            void clearScreen();
            void refreshScreen();

        private:
            // Terminal management
            bool initializeTerminal();
            void restoreTerminal();
            void getTerminalSize(int &width, int &height);

            // Input handling
            FunctionKey getKeyPress();
            bool handleKeyPress(FunctionKey key);

            // Rendering
            void renderHeader();
            void renderDeviceInfo();
            void renderGraphs();
            void renderFooter();
            void renderStatusBar();
            void renderSimpleGraph(int x, int y, int width, int height, LiveGraph *graph);
            void renderLogScreen();
            void switchToScreen(Screen screen);

            // Utility functions
            void moveCursor(int x, int y);
            void setColor(Color fg, Color bg = Color::Black);
            void resetColor();
            void hideCursor();
            void showCursor();

            // State variables
            bool running_;
            bool initialized_;
            int terminalWidth_;
            int terminalHeight_;

            // Screen management
            Screen currentScreen_;

            // State management
            RadioState *radio_state_;
            bool connected_to_state_;

            // Graphs
            std::unique_ptr<LiveGraph> txGraph_;
            std::unique_ptr<LiveGraph> rxGraph_;
            std::unique_ptr<LiveGraph> linkQualityGraph_;

            // Menu system
            std::map<FunctionKey, MenuItem> menuItems_;

            // Log screen management
            int logScrollOffset_;
            size_t maxLogEntries_;

            // Update timing
            std::chrono::steady_clock::time_point lastUpdate_;
            static constexpr int UPDATE_INTERVAL_MS = 100; // 10 FPS

            // Platform-specific terminal state
#ifdef _WIN32
            HANDLE hConsole_;
            CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo_;
            DWORD originalConsoleMode_;
#else
            struct termios originalTermios_;
#endif
        };

    } // namespace UI
} // namespace ELRS