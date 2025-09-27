#pragma once

#include "screen_base.h"
#include <map>
#include <memory>
#include <stack>

namespace ELRS
{
    namespace UI
    {
        /**
         * Screen Manager - Handles screen lifecycle and transitions
         */
        class ScreenManager
        {
        public:
            ScreenManager();
            ~ScreenManager();

            /**
             * Initialize the screen manager
             */
            bool initialize();

            /**
             * Run the main screen loop
             */
            void run();

            /**
             * Shutdown and cleanup
             */
            void shutdown();

            /**
             * Switch to a specific screen
             */
            void switchToScreen(ScreenType screenType);

            /**
             * Go back to previous screen
             */
            void goBack();

            /**
             * Exit the application
             */
            void exit();

            /**
             * Check if the manager is running
             */
            bool isRunning() const { return running_; }

        private:
            void initializeTerminal();
            void cleanupTerminal();
            void updateTerminalSize();
            FunctionKey readKey();
            void handleGlobalKeys(FunctionKey key);

            // Screen management
            std::map<ScreenType, std::unique_ptr<ScreenBase>> screens_;
            ScreenBase *currentScreen_;
            std::stack<ScreenType> screenHistory_;

            // System state
            bool running_;
            bool initialized_;

            // Rendering context
            RenderContext renderContext_;
            NavigationContext navContext_;

            // Timing
            std::chrono::steady_clock::time_point lastUpdate_;
            static constexpr int UPDATE_INTERVAL_MS = 500; // 2 FPS

#ifdef _WIN32
            HANDLE hConsole_;
            DWORD originalConsoleMode_;
            CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo_;
#else
            struct termios originalTermios_;
#endif
        };

    } // namespace UI
} // namespace ELRS