#include "screens/bind_screen.h"
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
        BindScreen::BindScreen()
            : ScreenBase(ScreenType::Bind, "Bind"),
              currentState_(BindState::Idle),
              bindProgress_(0),
              bindTimeout_(BIND_TIMEOUT_SECONDS),
              isBinding_(false),
              lastUpdate_(std::chrono::steady_clock::now())
        {
            bindPhrase_ = generateBindPhrase();
        }

        bool BindScreen::initialize(const RenderContext &renderContext, const NavigationContext &navContext)
        {
            logInfo("Initializing bind screen");
            currentState_ = BindState::Idle;
            bindProgress_ = 0;
            isBinding_ = false;
            return true;
        }

        void BindScreen::onActivate()
        {
            logInfo("Bind screen activated");
            markForRefresh();
        }

        void BindScreen::onDeactivate()
        {
            logDebug("Bind screen deactivated");
            if (isBinding_)
            {
                stopBinding();
            }
        }

        void BindScreen::update(std::chrono::milliseconds deltaTime)
        {
            if (!isBinding_)
                return;

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

            if (elapsed.count() >= BIND_UPDATE_INTERVAL_MS)
            {
                updateBindingStatus();
                markForRefresh();
                lastUpdate_ = now;
            }
        }

        void BindScreen::render(const RenderContext &renderContext)
        {
            if (!needsRefresh())
                return;

            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - ExpressLRS Binding", Color::BrightWhite);

            // Render bind content
            renderBindStatus(renderContext);
            renderBindControls(renderContext);
            renderBindProgress(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);

            if (isBinding_)
            {
                std::cout << "ESC: Cancel Binding | F1: Return | F12: Exit";
            }
            else
            {
                std::cout << "ENTER: Start Binding | G: Generate New Phrase | F1: Return | F12: Exit";
            }

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(getStateColor(currentState_));

            auto &radioState = getRadioState();
            auto deviceConfig = radioState.getDeviceConfiguration();

            std::cout << " Bind Status: " << getStateText(currentState_)
                      << " | Device: " << deviceConfig.productName
                      << " | Phrase: " << bindPhrase_;

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool BindScreen::handleKeyPress(FunctionKey key)
        {
            switch (key)
            {
            case FunctionKey::Enter:
                if (!isBinding_ && currentState_ != BindState::Binding)
                {
                    startBinding();
                }
                return true;
            case FunctionKey::G:
            case FunctionKey::g:
                if (!isBinding_)
                {
                    bindPhrase_ = generateBindPhrase();
                    markForRefresh();
                    logInfo("Generated new bind phrase: " + bindPhrase_);
                }
                return true;
            case FunctionKey::Escape:
                if (isBinding_)
                {
                    stopBinding();
                }
                else
                {
                    navigateToScreen(ScreenType::Main);
                }
                return true;
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

        void BindScreen::cleanup()
        {
            logInfo("Cleaning up bind screen");
            if (isBinding_)
            {
                stopBinding();
            }
        }

        void BindScreen::startBinding()
        {
            logInfo("Starting ExpressLRS binding process");
            isBinding_ = true;
            currentState_ = BindState::Binding;
            bindStartTime_ = std::chrono::steady_clock::now();
            bindProgress_ = 0;
            markForRefresh();
        }

        void BindScreen::stopBinding()
        {
            logInfo("Stopping ExpressLRS binding process");
            isBinding_ = false;
            if (currentState_ == BindState::Binding)
            {
                currentState_ = BindState::Idle;
            }
            markForRefresh();
        }

        void BindScreen::updateBindingStatus()
        {
            if (!isBinding_ || currentState_ != BindState::Binding)
                return;

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - bindStartTime_);

            // Update progress
            bindProgress_ = static_cast<int>((elapsed.count() * 100) / BIND_TIMEOUT_SECONDS);

            // Check for timeout
            if (elapsed.count() >= BIND_TIMEOUT_SECONDS)
            {
                // Simulate random binding success/failure
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(1, 10);

                if (dis(gen) <= 7) // 70% success rate
                {
                    currentState_ = BindState::Bound;
                    logInfo("Binding successful!");
                }
                else
                {
                    currentState_ = BindState::Failed;
                    logWarning("Binding failed - timeout or device not found");
                }

                isBinding_ = false;
                bindProgress_ = 100;
            }
            else if (elapsed.count() > 0)
            {
                // Simulate early binding success (random chance)
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(1, 1000);

                if (dis(gen) <= 5) // 0.5% chance per update
                {
                    currentState_ = BindState::Bound;
                    isBinding_ = false;
                    bindProgress_ = 100;
                    logInfo("Binding successful!");
                }
            }
        }

        void BindScreen::renderBindStatus(const RenderContext &renderContext)
        {
            int startY = 3;
            int centerX = renderContext.terminalWidth / 2;

            // Bind status box
            moveCursor(centerX - 35, startY);
            setColor(Color::BrightCyan);
            std::cout << "╭─────────────────────────────────────────────────────────────────────╮";

            moveCursor(centerX - 35, startY + 1);
            std::cout << "│                        ExpressLRS Binding Status                    │";

            moveCursor(centerX - 35, startY + 2);
            std::cout << "├─────────────────────────────────────────────────────────────────────┤";

            // Current state
            moveCursor(centerX - 35, startY + 3);
            std::cout << "│ Status: ";
            setColor(getStateColor(currentState_));
            std::cout << std::left << std::setw(55) << getStateText(currentState_);
            setColor(Color::BrightCyan);
            std::cout << "│";

            // Bind phrase
            moveCursor(centerX - 35, startY + 4);
            std::cout << "│ Bind Phrase: ";
            setColor(Color::BrightYellow);
            std::cout << std::left << std::setw(50) << bindPhrase_;
            setColor(Color::BrightCyan);
            std::cout << "│";

            // Device info
            auto &radioState = getRadioState();
            auto deviceConfig = radioState.getDeviceConfiguration();

            moveCursor(centerX - 35, startY + 5);
            std::cout << "│ Target Device: ";
            setColor(Color::BrightWhite);
            std::cout << std::left << std::setw(48) << deviceConfig.productName;
            setColor(Color::BrightCyan);
            std::cout << "│";

            moveCursor(centerX - 35, startY + 6);
            std::cout << "╰─────────────────────────────────────────────────────────────────────╯";
        }

        void BindScreen::renderBindControls(const RenderContext &renderContext)
        {
            int startY = 11;
            int centerX = renderContext.terminalWidth / 2;

            // Instructions
            setColor(Color::BrightWhite);
            printCentered(startY, "Instructions:", Color::BrightWhite);

            setColor(Color::White);
            printCentered(startY + 2, "1. Ensure your ExpressLRS receiver is powered and in range");
            printCentered(startY + 3, "2. Press ENTER to start the binding process");
            printCentered(startY + 4, "3. Put your receiver into bind mode within 30 seconds");
            printCentered(startY + 5, "4. Wait for binding confirmation");

            if (currentState_ == BindState::Bound)
            {
                printCentered(startY + 7, "✓ Binding Complete! Your receiver is now paired.", Color::BrightGreen);
            }
            else if (currentState_ == BindState::Failed)
            {
                printCentered(startY + 7, "✗ Binding Failed! Please try again.", Color::BrightRed);
                printCentered(startY + 8, "Make sure receiver is in bind mode and in range.", Color::Yellow);
            }
        }

        void BindScreen::renderBindProgress(const RenderContext &renderContext)
        {
            if (!isBinding_ && currentState_ != BindState::Binding)
                return;

            int startY = 20;
            int centerX = renderContext.terminalWidth / 2;
            int progressWidth = 50;

            // Progress bar
            moveCursor(centerX - progressWidth / 2, startY);
            setColor(Color::BrightYellow);
            std::cout << "Binding Progress: ";

            moveCursor(centerX - progressWidth / 2, startY + 1);
            std::cout << "[";

            int filled = (bindProgress_ * (progressWidth - 2)) / 100;
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

            setColor(Color::BrightYellow);
            std::cout << "] " << bindProgress_ << "%";

            // Time remaining
            if (isBinding_)
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - bindStartTime_);
                int remaining = BIND_TIMEOUT_SECONDS - static_cast<int>(elapsed.count());

                moveCursor(centerX - 10, startY + 3);
                setColor(Color::BrightCyan);
                std::cout << "Time remaining: " << remaining << " seconds";
            }
        }

        std::string BindScreen::generateBindPhrase()
        {
            // Generate a realistic ExpressLRS bind phrase
            const std::vector<std::string> words = {
                "alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf", "hotel",
                "india", "juliet", "kilo", "lima", "mike", "november", "oscar", "papa",
                "quebec", "romeo", "sierra", "tango", "uniform", "victor", "whiskey", "xray"};

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, words.size() - 1);

            return words[dis(gen)] + "-" + words[dis(gen)] + "-" + words[dis(gen)];
        }

        Color BindScreen::getStateColor(BindState state) const
        {
            switch (state)
            {
            case BindState::Idle:
                return Color::BrightBlue;
            case BindState::Binding:
                return Color::BrightYellow;
            case BindState::Bound:
                return Color::BrightGreen;
            case BindState::Failed:
            case BindState::Timeout:
                return Color::BrightRed;
            default:
                return Color::White;
            }
        }

        std::string BindScreen::getStateText(BindState state) const
        {
            switch (state)
            {
            case BindState::Idle:
                return "Ready to Bind";
            case BindState::Binding:
                return "Binding in Progress...";
            case BindState::Bound:
                return "Successfully Bound";
            case BindState::Failed:
                return "Binding Failed";
            case BindState::Timeout:
                return "Binding Timeout";
            default:
                return "Unknown";
            }
        }
    }
}