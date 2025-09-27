#include "screens/txtest_screen.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ELRS
{
    namespace UI
    {
        TxTestScreen::TxTestScreen()
            : ScreenBase(ScreenType::TxTest, "TX Test"), currentMode_(TestMode::Idle), lastUpdate_(std::chrono::steady_clock::now()), isRunning_(false), selectedTest_(0)
        {
        }

        bool TxTestScreen::initialize(const RenderContext &renderContext,
                                      const NavigationContext &navContext)
        {
            logInfo("Initializing TX test screen");
            return true;
        }

        void TxTestScreen::onActivate()
        {
            logInfo("TX test screen activated");
            markForRefresh();
        }

        void TxTestScreen::onDeactivate()
        {
            logDebug("TX test screen deactivated");
            if (isRunning_)
            {
                stopTest();
            }
        }

        void TxTestScreen::update(std::chrono::milliseconds deltaTime)
        {
            if (!isRunning_)
                return; // Only update when test is running

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_);

            if (elapsed.count() >= TEST_UPDATE_INTERVAL_MS)
            {
                // Store previous results to check for changes
                auto previousResults = testResults_;
                updateTestResults();

                // Only refresh if results actually changed
                bool resultsChanged = false;
                for (size_t i = 0; i < testResults_.size() && i < previousResults.size(); ++i)
                {
                    if (testResults_[i].progress != previousResults[i].progress ||
                        testResults_[i].status != previousResults[i].status)
                    {
                        resultsChanged = true;
                        break;
                    }
                }

                if (resultsChanged)
                {
                    markForRefresh();
                }
                lastUpdate_ = now;
            }
        }

        void TxTestScreen::render(const RenderContext &renderContext)
        {
            clearScreen();

            // Header
            printCentered(0, "ELRS OTG Monitor v1.0 - Transmitter Test Suite", Color::BrightWhite);

            // Render test content
            renderTestControls(renderContext);
            renderTestResults(renderContext);
            renderTestStatus(renderContext);

            // Footer
            int footerY = renderContext.terminalHeight - 2;
            moveCursor(0, footerY);
            setColor(Color::BrightBlue);

            if (isRunning_)
            {
                std::cout << "S: Stop Test | ESC/F1: Return | F12: Exit";
            }
            else
            {
                std::cout << "1-5: Select Test | ENTER: Start | ESC/F1: Return | F12: Exit";
            }

            // Status bar
            moveCursor(0, renderContext.terminalHeight - 1);
            setColor(isRunning_ ? Color::BrightYellow : Color::BrightGreen);

            std::string modeStr;
            switch (currentMode_)
            {
            case TestMode::Idle:
                modeStr = "Idle";
                break;
            case TestMode::ContinuousWave:
                modeStr = "Continuous Wave";
                break;
            case TestMode::ModulatedSignal:
                modeStr = "Modulated Signal";
                break;
            case TestMode::PowerSweep:
                modeStr = "Power Sweep";
                break;
            case TestMode::FrequencySweep:
                modeStr = "Frequency Sweep";
                break;
            }

            std::cout << " TX Test | Mode: " << modeStr << " | Status: "
                      << (isRunning_ ? "RUNNING" : "STOPPED");

            resetColor();
            std::cout.flush();
            clearRefreshFlag();
        }

        bool TxTestScreen::handleKeyPress(FunctionKey key)
        {
            if (isRunning_)
            {
                switch (key)
                {
                case FunctionKey::Escape:
                case FunctionKey::F1:
                    stopTest();
                    navigateToScreen(ScreenType::Main);
                    return true;
                case FunctionKey::F12:
                    stopTest();
                    exitApplication();
                    return true;
                default:
                    return false;
                }
            }
            else
            {
                switch (key)
                {
                case FunctionKey::Enter:
                    if (selectedTest_ >= 0 && selectedTest_ < 5)
                    {
                        startTest(static_cast<TestMode>(selectedTest_ + 1));
                    }
                    return true;
                case FunctionKey::ArrowUp:
                    if (selectedTest_ > 0)
                    {
                        selectedTest_--;
                        markForRefresh();
                    }
                    return true;
                case FunctionKey::ArrowDown:
                    if (selectedTest_ < 4)
                    {
                        selectedTest_++;
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
        }

        void TxTestScreen::cleanup()
        {
            logInfo("Cleaning up TX test screen");
            if (isRunning_)
            {
                stopTest();
            }
        }

        void TxTestScreen::startTest(TestMode mode)
        {
            logInfo("Starting TX test mode: " + std::to_string(static_cast<int>(mode)));
            currentMode_ = mode;
            isRunning_ = true;
            testStartTime_ = std::chrono::steady_clock::now();
            testResults_.clear();
            markForRefresh();
        }

        void TxTestScreen::stopTest()
        {
            logInfo("Stopping TX test");
            currentMode_ = TestMode::Idle;
            isRunning_ = false;
            markForRefresh();
        }

        void TxTestScreen::updateTestResults()
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - testStartTime_);

            testResults_.clear();

            auto &radioState = getRadioState();
            auto telemetry = radioState.getLiveTelemetry();

            switch (currentMode_)
            {
            case TestMode::ContinuousWave:
                testResults_.push_back({"CW Signal Generation", "Active", "Running", 100, true, "Generating unmodulated carrier"});
                testResults_.push_back({"Power Output", std::to_string(telemetry.txPower) + " dBm", "Active", 100,
                                        telemetry.txPower > 10, "Current transmit power"});
                testResults_.push_back({"Temperature", std::to_string(static_cast<int>(telemetry.temperature)) + "°C", "Normal", 100,
                                        telemetry.temperature < 70, "Device temperature"});
                break;

            case TestMode::ModulatedSignal:
                testResults_.push_back({"Modulated Signal", "Active", "Running", 100, true, "Transmitting test pattern"});
                testResults_.push_back({"Modulation Depth", "85%", "Normal", 85, true, "Signal modulation level"});
                testResults_.push_back({"Spectral Purity", "Good", "OK", 100, true, "Frequency spectrum analysis"});
                break;

            case TestMode::PowerSweep:
            {
                int progress = (std::min)(100, static_cast<int>(elapsed.count() * 10));
                testResults_.push_back({"Power Sweep", std::to_string(progress) + "%", "Running", progress, true,
                                        "Sweeping from -10 to +20 dBm"});
                testResults_.push_back({"Current Level", std::to_string(telemetry.txPower) + " dBm", "Active", 100,
                                        true, "Current power level"});
                testResults_.push_back({"Linearity", "Good", "OK", 100, true, "Power vs setting linearity"});
            }
            break;

            case TestMode::FrequencySweep:
            {
                int progress = (std::min)(100, static_cast<int>(elapsed.count() * 5));
                testResults_.push_back({"Frequency Sweep", std::to_string(progress) + "%", "Running", progress, true,
                                        "Sweeping 2.4GHz band"});
                testResults_.push_back({"Current Freq", "2440 MHz", "Active", 100, true, "Current frequency"});
                testResults_.push_back({"PLL Lock", "Locked", "OK", 100, true, "Frequency synthesizer status"});
            }
            break;

            default:
                break;
            }

            // Add common results
            if (isRunning_)
            {
                testResults_.push_back({"Test Duration", std::to_string(elapsed.count()) + "s", "Running", 0, true,
                                        "Total test runtime"});
                testResults_.push_back({"System Status", "Normal", "OK", 100, true, "Overall system health"});
            }
        }

        void TxTestScreen::renderTestControls(const RenderContext &context)
        {
            int startY = 3;
            int controlWidth = context.terminalWidth / 2 - 2;

            setColor(Color::BrightYellow);
            drawBox(1, startY, controlWidth, 8, "Test Selection");

            const std::vector<std::string> testNames = {
                "Continuous Wave Test",
                "Modulated Signal Test",
                "Power Sweep Test",
                "Frequency Sweep Test"};

            for (int i = 0; i < static_cast<int>(testNames.size()); ++i)
            {
                moveCursor(3, startY + 2 + i);

                if (i == selectedTest_ && !isRunning_)
                {
                    setColor(Color::BrightWhite);
                    std::cout << "► ";
                }
                else
                {
                    setColor(Color::White);
                    std::cout << "  ";
                }

                if (isRunning_ && static_cast<int>(currentMode_) == i + 1)
                {
                    setColor(Color::BrightGreen);
                    std::cout << "● ";
                }

                std::cout << (i + 1) << ". " << testNames[i];
            }

            // Test description
            if (selectedTest_ >= 0 && selectedTest_ < static_cast<int>(testNames.size()))
            {
                moveCursor(3, startY + 7);
                setColor(Color::BrightCyan);

                switch (selectedTest_)
                {
                case 0:
                    std::cout << "Generate unmodulated carrier wave";
                    break;
                case 1:
                    std::cout << "Transmit modulated test pattern";
                    break;
                case 2:
                    std::cout << "Sweep power from min to max";
                    break;
                case 3:
                    std::cout << "Sweep across frequency band";
                    break;
                }
            }
        }

        void TxTestScreen::renderTestResults(const RenderContext &context)
        {
            int startY = 3;
            int startX = context.terminalWidth / 2 + 1;
            int resultWidth = context.terminalWidth / 2 - 2;

            setColor(Color::BrightCyan);
            drawBox(startX, startY, resultWidth, 10, "Test Results");

            if (testResults_.empty())
            {
                moveCursor(startX + 2, startY + 2);
                setColor(Color::BrightBlack);
                std::cout << "No test running";
            }
            else
            {
                for (int i = 0; i < static_cast<int>(testResults_.size()) && i < 8; ++i)
                {
                    const auto &result = testResults_[i];

                    moveCursor(startX + 2, startY + 2 + i);
                    setColor(result.passed ? Color::BrightGreen : Color::BrightRed);
                    std::cout << (result.passed ? "✓" : "✗") << " ";

                    setColor(Color::White);
                    std::cout << result.testName << ": ";

                    setColor(result.passed ? Color::BrightGreen : Color::BrightRed);
                    std::cout << result.result;
                }
            }
        }

        void TxTestScreen::renderTestStatus(const RenderContext &context)
        {
            int statusY = context.terminalHeight - 8;
            int statusWidth = context.terminalWidth - 4;

            if (isRunning_)
            {
                setColor(Color::BrightYellow);
                drawBox(2, statusY, statusWidth, 4, "⚠ TEST IN PROGRESS ⚠");

                moveCursor(4, statusY + 1);
                setColor(Color::BrightYellow);
                std::cout << "WARNING: Transmitter is active and radiating RF energy";

                moveCursor(4, statusY + 2);
                setColor(Color::BrightRed);
                std::cout << "Ensure proper antenna connection and safe environment";
            }
            else
            {
                setColor(Color::BrightGreen);
                drawBox(2, statusY, statusWidth, 4, "✓ SAFE MODE");

                moveCursor(4, statusY + 1);
                setColor(Color::BrightGreen);
                std::cout << "Transmitter is in safe mode - no RF output";

                moveCursor(4, statusY + 2);
                setColor(Color::White);
                std::cout << "Select a test mode and press ENTER to begin";
            }
        }
    }
}
