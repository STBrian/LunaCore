#pragma once

#include "Api/Facade.hpp"
#include "Minecraft/Common/Client/Gui/Components/IconButton.hpp"
#include "Minecraft/Common/Client/Gui/Components/GuiButton.hpp"
#include "Minecraft/Common/Client/Gui/Components/NinePatch.hpp"
#include "Minecraft/Common/Client/Gui/Screens/Screen.hpp"
#include "Minecraft/Common/Client/Gui/Screens/ScreenStack.hpp"
#include "Minecraft/Common/Platform/AppPlatform.hpp"
#include <CTRPluginFramework.hpp>

#include "GameAPI.hpp"
#include "Core/ModSettings.hpp"

#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

#ifndef STRINGIFY_MACRO
#define STRINGIFY_MACRO(x) STRINGIFY_MACRO_HELPER(x)
#define STRINGIFY_MACRO_HELPER(x) #x   
#endif

namespace Core {
    namespace CTRPF = CTRPluginFramework;
    namespace MC3DSPF = MC3DSPluginFramework;
    using MC3DSPF::GuiButton;

    class LunaCoreModSettingsScreen : public MC3DSPF::Screen
    {
        USE_GAME_ALLOCATOR
    private:
        static constexpr int ITEMS_PER_PAGE = 4;

        enum ButtonId {
            BUTTON_EXIT = 1,
            BUTTON_PREV_PAGE = 2,
            BUTTON_NEXT_PAGE = 3,
            BUTTON_SETTING_BASE = 100,
        };

        std::string mModId;
        std::string mModTitle;
        int mCurrentPage;

        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackgroundFrame;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackground;

        static std::string formatFloat(float val) {
            char buf[32];
            if (std::floor(val) == val) {
                snprintf(buf, sizeof(buf), "%d", (int)val);
            } else {
                snprintf(buf, sizeof(buf), "%.2f", val);
            }
            return std::string(buf);
        }

        void rebuildComponents() {
            mButtons.clear();
            setupComponents();
        }

    public:
        LunaCoreModSettingsScreen(MC3DSPF::MinecraftGame *mg, MC3DSPF::ClientInstance *ci,
                                  const std::string& modId, const std::string& modTitle) :
            Screen(mg, ci), mModId(modId), mModTitle(modTitle), mCurrentPage(0) {}

        ~LunaCoreModSettingsScreen() override = default;

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

            ModSettingsConfig* cfg = ModSettingsManager::getInstance().getConfig(mModId);
            if (!cfg) return;

            int totalItems = (int)cfg->items.size();
            int startIndex = mCurrentPage * ITEMS_PER_PAGE;
            int endIndex = std::min(startIndex + ITEMS_PER_PAGE, totalItems);

            for (int i = startIndex; i < endIndex; i++) {
                int slot = i - startIndex;
                int itemY = 34 + slot * 40;
                SettingItem& item = cfg->items[i];
                int baseId = BUTTON_SETTING_BASE + slot * 10;

                switch (item.type) {
                    case SettingType::Toggle: {
                        std::string label = item.label + ": " + (item.boolValue ? "[ON]" : "[OFF]");
                        BoxedPtr::Shared<GuiButton> btn(gstd::make_unique<GuiButton>(
                            mMinecraftGame, baseId, 15, itemY, 290, 34, label.c_str(), false, INT_MAX));
                        btn->buildBackground();
                        mButtons.push_back(std::move(btn));
                        break;
                    }
                    case SettingType::Dropdown: {
                        std::string optName = "None";
                        if (!item.options.empty() && item.selectedIndex >= 0 && item.selectedIndex < (int)item.options.size()) {
                            optName = item.options[item.selectedIndex];
                        }
                        std::string label = item.label + ": < " + optName + " >";
                        BoxedPtr::Shared<GuiButton> btn(gstd::make_unique<GuiButton>(
                            mMinecraftGame, baseId, 15, itemY, 290, 34, label.c_str(), false, INT_MAX));
                        btn->buildBackground();
                        mButtons.push_back(std::move(btn));
                        break;
                    }
                    case SettingType::Button: {
                        BoxedPtr::Shared<GuiButton> btn(gstd::make_unique<GuiButton>(
                            mMinecraftGame, baseId, 15, itemY, 290, 34, item.label.c_str(), false, INT_MAX));
                        btn->buildBackground();
                        mButtons.push_back(std::move(btn));
                        break;
                    }
                    case SettingType::Slider: {
                        // [-] Button
                        BoxedPtr::Shared<GuiButton> minusBtn(gstd::make_unique<GuiButton>(
                            mMinecraftGame, baseId + 1, 15, itemY, 32, 34, "-", false, INT_MAX));
                        minusBtn->buildBackground();
                        mButtons.push_back(std::move(minusBtn));

                        // Value / Label Button
                        std::string label = item.label + ": " + formatFloat(item.floatValue);
                        BoxedPtr::Shared<GuiButton> valBtn(gstd::make_unique<GuiButton>(
                            mMinecraftGame, baseId, 50, itemY, 220, 34, label.c_str(), false, INT_MAX));
                        valBtn->buildBackground();
                        mButtons.push_back(std::move(valBtn));

                        // [+] Button
                        BoxedPtr::Shared<GuiButton> plusBtn(gstd::make_unique<GuiButton>(
                            mMinecraftGame, baseId + 2, 273, itemY, 32, 34, "+", false, INT_MAX));
                        plusBtn->buildBackground();
                        mButtons.push_back(std::move(plusBtn));
                        break;
                    }
                }
            }

            // Pagination buttons
            if (totalItems > ITEMS_PER_PAGE) {
                if (mCurrentPage > 0) {
                    BoxedPtr::Shared<GuiButton> prevBtn(gstd::make_unique<GuiButton>(
                        mMinecraftGame, ButtonId::BUTTON_PREV_PAGE, 15, 196, 60, 24, "< Prev", false, INT_MAX));
                    prevBtn->buildBackground();
                    mButtons.push_back(std::move(prevBtn));
                }
                if (endIndex < totalItems) {
                    BoxedPtr::Shared<GuiButton> nextBtn(gstd::make_unique<GuiButton>(
                        mMinecraftGame, ButtonId::BUTTON_NEXT_PAGE, 245, 196, 60, 24, "Next >", false, INT_MAX));
                    nextBtn->buildBackground();
                    mButtons.push_back(std::move(nextBtn));
                }
            }
        }

        void buttonPressed(MC3DSPF::BaseButton &b) override
        {
            int id = b.getId();
            if (id == ButtonId::BUTTON_EXIT) {
                ModSettingsManager::getInstance().notifyClose(mModId);
                MC3DSPF::Facade::getMinecraftGame()->getScreenStack().schedulePopScreen(1);
                return;
            }
            if (id == ButtonId::BUTTON_PREV_PAGE) {
                if (mCurrentPage > 0) {
                    mCurrentPage--;
                    rebuildComponents();
                }
                return;
            }
            if (id == ButtonId::BUTTON_NEXT_PAGE) {
                ModSettingsConfig* cfg = ModSettingsManager::getInstance().getConfig(mModId);
                if (cfg && (mCurrentPage + 1) * ITEMS_PER_PAGE < (int)cfg->items.size()) {
                    mCurrentPage++;
                    rebuildComponents();
                }
                return;
            }

            if (id >= BUTTON_SETTING_BASE) {
                int slot = (id - BUTTON_SETTING_BASE) / 10;
                int subId = (id - BUTTON_SETTING_BASE) % 10;
                int itemIndex = mCurrentPage * ITEMS_PER_PAGE + slot;

                ModSettingsConfig* cfg = ModSettingsManager::getInstance().getConfig(mModId);
                if (!cfg || itemIndex >= (int)cfg->items.size()) return;

                SettingItem& item = cfg->items[itemIndex];
                switch (item.type) {
                    case SettingType::Toggle:
                        item.boolValue = !item.boolValue;
                        rebuildComponents();
                        break;
                    case SettingType::Dropdown:
                        if (!item.options.empty()) {
                            item.selectedIndex = (item.selectedIndex + 1) % item.options.size();
                        }
                        rebuildComponents();
                        break;
                    case SettingType::Button:
                        ModSettingsManager::getInstance().triggerButton(item.luaCallbackRef);
                        break;
                    case SettingType::Slider:
                        if (subId == 1) { // Minus
                            item.floatValue -= item.step;
                            if (item.floatValue < item.minValue) item.floatValue = item.minValue;
                            rebuildComponents();
                        } else if (subId == 2) { // Plus
                            item.floatValue += item.step;
                            if (item.floatValue > item.maxValue) item.floatValue = item.maxValue;
                            rebuildComponents();
                        } else if (subId == 0) { // Cycle
                            item.floatValue += item.step;
                            if (item.floatValue > item.maxValue) item.floatValue = item.minValue;
                            rebuildComponents();
                        }
                        break;
                }
            }
        }

        bool onBack(int p1) override
        {
            ModSettingsManager::getInstance().notifyClose(mModId);
            MC3DSPF::Facade::getMinecraftGame()->getScreenStack().schedulePopScreen(1);
            return true;
        }

        int getUseScreen() override
        {
            return MC3DSPF::UseScreen::Both;
        }

        MC3DSPF::gstd::string getName() override
        {
            return "lunacore_mod_settings_screen";
        }

        void render(int touchX, int touchY, int useScreen, float tick) override
        {
            if (useScreen & MC3DSPF::UseScreen::Top)
            {
                std::string titleStr = mModTitle + " Settings";
                int h = mFont->getTextHeight(titleStr.c_str(), 0, 1);
                mFont->drawWithShadow(10, 15, 1.0f, titleStr.c_str(), MC3DSPF::Color(1, 1, 0.4f), 0, -1);
                mFont->drawWithShadow(10, 35, 1.0f, "Configure mod options below", MC3DSPF::Color(0.8f, 0.8f, 0.8f), 0, -1);

                std::string footer("LunaCore v" STRINGIFY_MACRO(LUNACORE_VER_MAJOR) "." STRINGIFY_MACRO(LUNACORE_VER_MINOR) "." STRINGIFY_MACRO(LUNACORE_VER_PATCH));
                mFont->drawWithShadow(10, MC3DSPF::AppPlatform::TOP_SCREEN_HEIGHT - h - 5, 1.0f, footer.c_str(), MC3DSPF::Color(0.6f, 0.6f, 0.6f), 0, -1);
            }

            if (useScreen & MC3DSPF::UseScreen::Bottom)
            {
                mBackgroundFrame->draw(MC3DSPF::Tessellator::instance, 0, 0);
                mBackground->draw(MC3DSPF::Tessellator::instance, 6, 30);

                std::string header = mModTitle + " Settings";
                if (header.length() > 28) header = header.substr(0, 25) + "...";
                mFont->drawWithShadow(12, 10, 1.0f, header.c_str(), MC3DSPF::Color(1, 1, 1), 0, -1);

                ModSettingsConfig* cfg = ModSettingsManager::getInstance().getConfig(mModId);
                if (cfg && cfg->items.size() > ITEMS_PER_PAGE) {
                    int totalPages = ((int)cfg->items.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
                    char pageBuf[32];
                    snprintf(pageBuf, sizeof(pageBuf), "%d / %d", mCurrentPage + 1, totalPages);
                    mFont->drawWithShadow(145, 202, 1.0f, pageBuf, MC3DSPF::Color(0.8f, 0.8f, 0.8f), 0, -1);
                }

                Screen::render(touchX, touchY, useScreen, tick);
            }
        }
    };
}
