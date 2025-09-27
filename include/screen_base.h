#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include "radio_state.h"
#include "log_manager.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

namespace ELRS
{
    namespace UI
    {
        /**
         * Terminal colors for screens
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
            BrightWhite = 15,
            DarkGray = BrightBlack  // Alias for BrightBlack
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
            PageUp,
            PageDown,
            Home,
            End,
            // Character keys
            A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H', I = 'I', J = 'J',
            K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T',
            U = 'U', V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',
            a = 'a', b = 'b', c = 'c', d = 'd', e = 'e', f = 'f', g = 'g', h = 'h', i = 'i', j = 'j',
            k = 'k', l = 'l', m = 'm', n = 'n', o = 'o', p = 'p', q = 'q', r = 'r', s = 's', t = 't',
            u = 'u', v = 'v', w = 'w', x = 'x', y = 'y', z = 'z',
            Tilde = '~',
            Unknown
        };

        /**
         * Screen types enumeration
         */
        enum class ScreenType
        {
            Main = 0,
            Logs,
            Config,
            Monitor,
            Graphs,
            TxTest,
            RxTest,
            Bind,
            Update,
            Export,
            Settings
        };

        /**
         * Screen navigation context
         */
        struct NavigationContext
        {
            ScreenType currentScreen;
            ScreenType previousScreen;
            std::function<void(ScreenType)> switchToScreen;
            std::function<void()> exitApplication;

            NavigationContext()
                : currentScreen(ScreenType::Main), previousScreen(ScreenType::Main), switchToScreen(nullptr), exitApplication(nullptr) {}
        };

        /**
         * Screen rendering context
         */
        struct RenderContext
        {
            int terminalWidth;
            int terminalHeight;
            int contentStartX;
            int contentStartY;
            int contentWidth;
            int contentHeight;

#ifdef _WIN32
            HANDLE consoleHandle;
#endif

            RenderContext()
                : terminalWidth(80), terminalHeight(25), contentStartX(0), contentStartY(3), contentWidth(80), contentHeight(22)
#ifdef _WIN32
                  ,
                  consoleHandle(INVALID_HANDLE_VALUE)
#endif
            {
            }
        };

        /**
         * Abstract base class for all TUI screens
         *
         * This class provides the foundation for all screens in the ELRS OTG Monitor.
         * Each screen should inherit from this class and implement the pure virtual methods.
         */
        class ScreenBase
        {
        public:
            /**
             * Constructor
             * @param screenType The type of this screen
             * @param title The title displayed for this screen
             */
            ScreenBase(ScreenType screenType, const std::string &title);

            /**
             * Virtual destructor
             */
            virtual ~ScreenBase() = default;

            // Core screen lifecycle methods (pure virtual)

            /**
             * Initialize the screen - called once when screen is created
             * @param renderContext The rendering context with terminal dimensions
             * @param navContext The navigation context for screen switching
             * @return true if initialization successful, false otherwise
             */
            virtual bool initialize(const RenderContext &renderContext,
                                    const NavigationContext &navContext) = 0;

            /**
             * Called when screen becomes active (gains focus)
             */
            virtual void onActivate() = 0;

            /**
             * Called when screen becomes inactive (loses focus)
             */
            virtual void onDeactivate() = 0;

            /**
             * Update screen logic - called every frame
             * @param deltaTime Time elapsed since last update
             */
            virtual void update(std::chrono::milliseconds deltaTime) = 0;

            /**
             * Render the screen content
             * @param renderContext Current rendering context
             */
            virtual void render(const RenderContext &renderContext) = 0;

            /**
             * Handle keyboard input
             * @param key The pressed key
             * @return true if key was handled, false if should be passed to parent
             */
            virtual bool handleKeyPress(FunctionKey key) = 0;

            /**
             * Cleanup screen resources - called when screen is destroyed
             */
            virtual void cleanup() = 0;

            // Common utility methods (implemented in base class)

            /**
             * Get screen type
             */
            ScreenType getScreenType() const { return screenType_; }

            /**
             * Get screen title
             */
            const std::string &getTitle() const { return title_; }

            /**
             * Check if screen is active
             */
            bool isActive() const { return isActive_; }

            /**
             * Check if screen needs refresh
             */
            bool needsRefresh() const { return needsRefresh_; }

            /**
             * Mark screen for refresh
             */
            void markForRefresh() { needsRefresh_ = true; }

            /**
             * Clear refresh flag
             */
            void clearRefreshFlag() { needsRefresh_ = false; }

        protected:
            // Protected utility methods for derived classes

            /**
             * Access to RadioState singleton
             */
            RadioState &getRadioState() { return RadioState::getInstance(); }

            /**
             * Access to LogManager singleton
             */
            LogManager &getLogManager() { return LogManager::getInstance(); }

            /**
             * Navigate to another screen
             */
            void navigateToScreen(ScreenType screen);

            /**
             * Go back to previous screen
             */
            void goBack();

            /**
             * Exit the application
             */
            void exitApplication();

            // Terminal utility methods
            void moveCursor(int x, int y);
            void setColor(Color fg, Color bg = Color::Black);
            void resetColor();
            void clearScreen();
            void clearLine();
            void hideCursor();
            void showCursor();

            // Drawing helpers
            void drawBox(int x, int y, int width, int height, const std::string &title = "");
            void drawHorizontalLine(int x, int y, int length, char ch = '-');
            void drawVerticalLine(int x, int y, int length, char ch = '|');
            void printCentered(int y, const std::string &text, Color color = Color::White);
            void printAt(int x, int y, const std::string &text, Color color = Color::White);

            // Status and logging helpers
            void logInfo(const std::string &message);
            void logWarning(const std::string &message);
            void logError(const std::string &message);
            void logDebug(const std::string &message);

        private:
            ScreenType screenType_;
            std::string title_;
            bool isActive_;
            bool needsRefresh_;
            NavigationContext navContext_;
            RenderContext renderContext_;

#ifdef _WIN32
            CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo_;
#else
            struct termios originalTermios_;
#endif
        };

        /**
         * Screen factory for creating screen instances
         */
        class ScreenFactory
        {
        public:
            /**
             * Create a screen instance
             * @param screenType The type of screen to create
             * @return Unique pointer to the created screen, or nullptr if type not supported
             */
            static std::unique_ptr<ScreenBase> createScreen(ScreenType screenType);

            /**
             * Get screen name for display purposes
             * @param screenType The screen type
             * @return Human-readable screen name
             */
            static std::string getScreenName(ScreenType screenType);
        };

    } // namespace UI
} // namespace ELRS