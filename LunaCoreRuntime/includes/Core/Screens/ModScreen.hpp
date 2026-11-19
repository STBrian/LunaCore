#pragma once

#include "Api/Facade.hpp"
#include "Minecraft/Common/Client/Gui/Components/IconButton.hpp"
#include "Minecraft/Common/Client/Gui/Components/GuiButton.hpp"
#include "Minecraft/Common/Client/Gui/Components/NinePatch.hpp"
#include "Minecraft/Common/Client/Gui/Screens/Screen.hpp"
#include "Minecraft/Common/Client/Gui/Screens/ScreenStack.hpp"
#include "Minecraft/Common/Client/Gui/Screens/ScreenChooser.hpp"
#include "Minecraft/Common/Platform/AppPlatform.hpp"
#include <CTRPluginFramework.hpp>

#include "GameAPI.hpp"
#include "Core/ModInfo.hpp"
#include "Core/ModSettings.hpp"
#include "ModSettingsScreen.hpp"

#include <string>
#include <vector>
#include <sstream>

#ifndef STRINGIFY_MACRO
#define STRINGIFY_MACRO(x) STRINGIFY_MACRO_HELPER(x)
#define STRINGIFY_MACRO_HELPER(x) #x   
#endif

namespace Core {
    namespace CTRPF = CTRPluginFramework;
    namespace MC3DSPF = MC3DSPluginFramework;
    using MC3DSPF::GuiButton;

    class LunaCoreModScreen : public MC3DSPF::Screen
    {
        USE_GAME_ALLOCATOR
    private:
        enum ButtonId {
            BUTTON_EXIT = 1,
            BUTTON_SETTINGS = 2,
        };

        ModInfo mModInfo;

        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackgroundFrame;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackground;

        static std::vector<std::string> wrapText(const std::string& text, size_t maxCharsPerLine) {
            std::vector<std::string> lines;
            std::istringstream words(text);
            std::string word;
            std::string currentLine;

            while (words >> word) {
                if (currentLine.empty()) {
                    currentLine = word;
                } else if (currentLine.length() + 1 + word.length() <= maxCharsPerLine) {
                    currentLine += " " + word;
                } else {
                    lines.push_back(currentLine);
                    currentLine = word;
                }
            }
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }
            return lines;
        }

    public:
        LunaCoreModScreen(MC3DSPF::MinecraftGame *mg, MC3DSPF::ClientInstance *ci, const ModInfo& modInfo) :
            Screen(mg, ci), mModInfo(modInfo) {}

        ~LunaCoreModScreen() override = default;

        void setupComponents() override
        {
            MC3DSPF::NinePatchFactory factory(mMinecraftGame->getTextures(), *(MC3DSPF::ResourceLocation *)0xABFD74);
            mBackgroundFrame = factory.createSymmetrical(MC3DSPF::IntRectangle(56, 160, 8, 8), 2, 2, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH, MC3DSPF::AppPlatform::BOTTOM_SCREEN_HEIGHT);
            mBackground      = factory.createSymmetrical(MC3DSPF::IntRectangle(80, 176, 8, 8), 3, 3, 308, 204);

            using namespace MinecraftAPI;

            // Close button
            BoxedPtr::Shared<IconButton> exitButton(gstd::make_unique<IconButton>(
                mMinecraftGame, ButtonId::BUTTON_EXIT, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH - 7 - 16, 7, 16, 16, "", false));
            exitButton->setTextures(GuiAtlas::EMPTY, GuiAtlas::BUTTON_CLOSE_BG, GuiAtlas::BUTTON_CLOSE_BG_HOVERED, 0, 0);
            mButtons.push_back(std::move(exitButton));

            // Only add settings button if mod has configurable settings
            if (ModSettingsManager::getInstance().hasSettings(mModInfo.id)) {
                BoxedPtr::Shared<GuiButton> settingsBtn(gstd::make_unique<GuiButton>(
                    mMinecraftGame, ButtonId::BUTTON_SETTINGS, 60, 182, 200, 28, "Settings", false, INT_MAX));
                settingsBtn->buildBackground();
                mButtons.push_back(std::move(settingsBtn));
            }
        }

        void buttonPressed(MC3DSPF::BaseButton &b) override
        {
            switch (b.getId())
            {
                case ButtonId::BUTTON_EXIT:
                    MC3DSPF::Facade::getMinecraftGame()->getScreenStack().schedulePopScreen(1);
                    break;
                case ButtonId::BUTTON_SETTINGS: {
                    MC3DSPF::ScreenChooser& chooser = MC3DSPF::Facade::getMinecraftGame()->getScreenChooser();
                    MC3DSPF::MinecraftGame* mg = MC3DSPF::Facade::getMinecraftGame();
                    MC3DSPF::ClientInstance* ci = MC3DSPF::Facade::getClient();
                    MC3DSPF::BoxedPtr::Shared<LunaCoreModSettingsScreen> screen(
                        MC3DSPF::gstd::make_unique<LunaCoreModSettingsScreen>(mg, ci, mModInfo.id, mModInfo.name));
                    chooser._pushScreen(screen);
                    break;
                }
                default: break;
            }
        }

        bool onBack(int p1) override
        {
            MC3DSPF::Facade::getMinecraftGame()->getScreenStack().schedulePopScreen(1);
            return true;
        }

        int getUseScreen() override
        {
            return MC3DSPF::UseScreen::Both;
        }

        MC3DSPF::gstd::string getName() override
        {
            return "lunacore_mod_screen";
        }

        void render(int touchX, int touchY, int useScreen, float tick) override
        {
            if (useScreen & MC3DSPF::UseScreen::Bottom)
            {
                mBackgroundFrame->draw(MC3DSPF::Tessellator::instance, 0, 0);
                mBackground->draw(MC3DSPF::Tessellator::instance, 6, 30);

                // Header
                std::string header = mModInfo.name + " (" + mModInfo.version + ")";
                mFont->drawWithShadow(12, 12, 1.0f, header.c_str(), MC3DSPF::Color(0, 0, 0), 0, -1);

                // Description header
                mFont->drawWithShadow(15, 38, 1.0f, "Description:", MC3DSPF::Color(1, 1, 0.5f), 0, -1);

                // Word-wrapped description lines
                std::vector<std::string> descLines = wrapText(mModInfo.description, 42);
                int curY = 54;
                for (size_t i = 0; i < descLines.size() && curY < 140; i++) {
                    mFont->drawWithShadow(15, curY, 1.0f, descLines[i].c_str(), MC3DSPF::Color(0.85f, 0.85f, 0.85f), 0, -1);
                    curY += 14;
                }

                // Dependencies summary
                std::string depText = "Dependencies: ";
                if (mModInfo.dependencies.empty()) {
                    depText += "None";
                } else {
                    for (size_t i = 0; i < mModInfo.dependencies.size(); i++) {
                        if (i > 0) depText += ", ";
                        depText += mModInfo.dependencies[i];
                    }
                }
                mFont->drawWithShadow(15, 148, 1.0f, depText.c_str(), MC3DSPF::Color(0.7f, 0.85f, 1.0f), 0, -1);

                if (!ModSettingsManager::getInstance().hasSettings(mModInfo.id)) {
                    mFont->drawWithShadow(65, 188, 1.0f, "(No settings available)", MC3DSPF::Color(0.55f, 0.55f, 0.55f), 0, -1);
                }

                Screen::render(touchX, touchY, useScreen, tick);
            }
        }
    };
}
