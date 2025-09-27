#pragma once

// Include the proper screen implementations
#include "bind_screen.h"
#include "update_screen.h"
#include "export_screen.h"
#include "settings_screen.h"

// This file now serves as a convenience include for all the implemented screens
// The actual implementations are in their respective header files

#include "../screen_base.h"
#include <iostream>

namespace ELRS
{
    namespace UI
    {
        class BindScreen : public ScreenBase
        {
        public:
            BindScreen() : ScreenBase(ScreenType::Bind, "Bind") {}
            virtual ~BindScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override
            {
                logInfo("Initializing bind screen");
                return true;
            }

            void onActivate() override
            {
                logInfo("Bind screen activated");
                markForRefresh();
            }
            void onDeactivate() override { logDebug("Bind screen deactivated"); }
            void update(std::chrono::milliseconds deltaTime) override {}

            void render(const RenderContext &renderContext) override
            {
                if (!needsRefresh())
                    return;
                clearScreen();
                printCentered(0, "ELRS OTG Monitor v1.0 - Device Binding", Color::BrightWhite);
                printCentered(renderContext.terminalHeight / 2, "Bind functionality coming soon...", Color::BrightYellow);
                moveCursor(0, renderContext.terminalHeight - 1);
                setColor(Color::BrightBlue);
                std::cout << "ESC/F1: Return | F12: Exit";
                resetColor();
                std::cout.flush();
                clearRefreshFlag();
            }

            bool handleKeyPress(FunctionKey key) override
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
                default:
                    return false;
                }
            }

            void cleanup() override { logInfo("Cleaning up bind screen"); }
        };

        class UpdateScreen : public ScreenBase
        {
        public:
            UpdateScreen() : ScreenBase(ScreenType::Update, "Update") {}
            virtual ~UpdateScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override
            {
                logInfo("Initializing update screen");
                return true;
            }

            void onActivate() override
            {
                logInfo("Update screen activated");
                markForRefresh();
            }
            void onDeactivate() override { logDebug("Update screen deactivated"); }
            void update(std::chrono::milliseconds deltaTime) override {}

            void render(const RenderContext &renderContext) override
            {
                if (!needsRefresh())
                    return;
                clearScreen();
                printCentered(0, "ELRS OTG Monitor v1.0 - Firmware Update", Color::BrightWhite);
                printCentered(renderContext.terminalHeight / 2, "Firmware update functionality coming soon...", Color::BrightYellow);
                moveCursor(0, renderContext.terminalHeight - 1);
                setColor(Color::BrightBlue);
                std::cout << "ESC/F1: Return | F12: Exit";
                resetColor();
                std::cout.flush();
                clearRefreshFlag();
            }

            bool handleKeyPress(FunctionKey key) override
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
                default:
                    return false;
                }
            }

            void cleanup() override { logInfo("Cleaning up update screen"); }
        };

        class ExportScreen : public ScreenBase
        {
        public:
            ExportScreen() : ScreenBase(ScreenType::Export, "Export") {}
            virtual ~ExportScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override
            {
                logInfo("Initializing export screen");
                return true;
            }

            void onActivate() override
            {
                logInfo("Export screen activated");
                markForRefresh();
            }
            void onDeactivate() override { logDebug("Export screen deactivated"); }
            void update(std::chrono::milliseconds deltaTime) override {}

            void render(const RenderContext &renderContext) override
            {
                if (!needsRefresh())
                    return;
                clearScreen();
                printCentered(0, "ELRS OTG Monitor v1.0 - Data Export", Color::BrightWhite);
                printCentered(renderContext.terminalHeight / 2, "Data export functionality coming soon...", Color::BrightYellow);
                moveCursor(0, renderContext.terminalHeight - 1);
                setColor(Color::BrightBlue);
                std::cout << "ESC/F1: Return | F12: Exit";
                resetColor();
                std::cout.flush();
                clearRefreshFlag();
            }

            bool handleKeyPress(FunctionKey key) override
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
                default:
                    return false;
                }
            }

            void cleanup() override { logInfo("Cleaning up export screen"); }
        };

        class SettingsScreen : public ScreenBase
        {
        public:
            SettingsScreen() : ScreenBase(ScreenType::Settings, "Settings") {}
            virtual ~SettingsScreen() = default;

            bool initialize(const RenderContext &renderContext, const NavigationContext &navContext) override
            {
                logInfo("Initializing settings screen");
                return true;
            }

            void onActivate() override
            {
                logInfo("Settings screen activated");
                markForRefresh();
            }
            void onDeactivate() override { logDebug("Settings screen deactivated"); }
            void update(std::chrono::milliseconds deltaTime) override {}

            void render(const RenderContext &renderContext) override
            {
                if (!needsRefresh())
                    return;
                clearScreen();
                printCentered(0, "ELRS OTG Monitor v1.0 - Application Settings", Color::BrightWhite);
                printCentered(renderContext.terminalHeight / 2, "Settings functionality coming soon...", Color::BrightYellow);
                moveCursor(0, renderContext.terminalHeight - 1);
                setColor(Color::BrightBlue);
                std::cout << "ESC/F1: Return | F12: Exit";
                resetColor();
                std::cout.flush();
                clearRefreshFlag();
            }

            bool handleKeyPress(FunctionKey key) override
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
                default:
                    return false;
                }
            }

            void cleanup() override { logInfo("Cleaning up settings screen"); }
        };
    }
}