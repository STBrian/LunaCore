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
#include "ModScreen.hpp"
#include "SettingsScreen.hpp"

#include <string>
#include <vector>
#include <algorithm>

#ifndef STRINGIFY_MACRO
#define STRINGIFY_MACRO(x) STRINGIFY_MACRO_HELPER(x)
#define STRINGIFY_MACRO_HELPER(x) #x   
#endif

namespace Core {
    namespace CTRPF = CTRPluginFramework;
    namespace MC3DSPF = MC3DSPluginFramework;
    using MC3DSPF::GuiButton;

    class ModListItemButton : public MC3DSPF::GuiButton
    {
        USE_GAME_ALLOCATOR
    public:
        ModListItemButton(MC3DSPF::MinecraftGame* game, int id, int x, int y, int w, int h,
                          const std::string& title, const std::string& desc) :
            GuiButton(game, id, x, y, w, h, "", false, INT_MAX),
            mTitle(title), mDesc(desc) {}

        ~ModListItemButton() override = default;

        void drawLabel(int touchX, int touchY) override
        {
            if (!mMinecraftGame) return;
            MC3DSPF::Font& font = mMinecraftGame->getDefaultFont();

            int textX = mX + 8;
            int titleY = mY + 8;
            int descY = mY + 22;

            if (FUN_005e6338(touchX, touchY)) {
                titleY += 1;
                descY += 1;
            }

            font.drawWithShadow((int)textX + 1, titleY + 1, 1, mTitle.c_str(), MC3DSPF::Color(0.45f, 0.45f, 0.45f), 0, -1);
            font.drawWithShadow((int)textX, titleY, 1, mTitle.c_str(), MC3DSPF::Color(1, 1, 1), 0, -1);

            std::string shortDesc = mDesc;
            if (shortDesc.length() > 42 - 4) {
                shortDesc = shortDesc.substr(0, 39 - 4) + "...";
            }
            font.drawWithShadow(textX, descY, 1, shortDesc.c_str(), MC3DSPF::Color(0.35f, 0.35f, 0.35f), 0, -1);
        }

    private:
        std::string mTitle;
        std::string mDesc;
    };

    class LunaCoreModsScreen : public MC3DSPF::Screen
    {
        USE_GAME_ALLOCATOR
    private:
        static constexpr int ITEMS_PER_PAGE = 4;

        enum ButtonId {
            BUTTON_EXIT = 1,
            BUTTON_PREV_PAGE = 2,
            BUTTON_NEXT_PAGE = 3,
            BUTTON_ITEM_BASE = 10,
        };

        int mCurrentPage;

        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mTopBar;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackground;

        int getTotalItems() const {
            // 1 for LunaCore + loaded mods
            return 1 + (int)Core::LoadedModList.size();
        }

        void rebuildComponents() {
            mButtons.clear();
            setupComponents();
        }

    public:
        LunaCoreModsScreen(MC3DSPF::MinecraftGame *mg, MC3DSPF::ClientInstance *ci) :
            Screen(mg, ci), mCurrentPage(0) {}

        ~LunaCoreModsScreen() override = default;

        void setupComponents() override
        {
            MC3DSPF::NinePatchFactory factory(mMinecraftGame->getTextures(), *(MC3DSPF::ResourceLocation *)0xABFD74);
            mTopBar = factory.createSymmetrical(MC3DSPF::IntRectangle(56, 160, 8, 8), 2, 2, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH + 4, 30);
            mBackground      = factory.createSymmetrical(MC3DSPF::IntRectangle(80, 176, 8, 8), 2, 2, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH + 4, MC3DSPF::AppPlatform::BOTTOM_SCREEN_HEIGHT);

            using namespace MinecraftAPI;

            // Close button
            BoxedPtr::Shared<IconButton> exitButton(gstd::make_unique<IconButton>(
                mMinecraftGame, ButtonId::BUTTON_EXIT, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH - 7 - 16, 7, 16, 16, "", false));
            exitButton->setTextures(GuiAtlas::EMPTY, GuiAtlas::BUTTON_CLOSE_BG, GuiAtlas::BUTTON_CLOSE_BG_HOVERED, 0, 0);
            mButtons.push_back(std::move(exitButton));

            int totalItems = getTotalItems();
            int startIndex = mCurrentPage * ITEMS_PER_PAGE;
            int endIndex = std::min(startIndex + ITEMS_PER_PAGE, totalItems);

            for (int i = startIndex; i < endIndex; i++) {
                int slot = i - startIndex;
                int itemY = 38 + slot * 40;
                int btnId = BUTTON_ITEM_BASE + slot;

                std::string itemTitle;
                std::string itemDesc;

                if (i == 0) {
                    // First item is always LunaCore (special, but visually identical to others)
                    itemTitle = "LunaCore";
                    itemDesc = "LunaCore plugin runtime & settings";
                } else {
                    int modIdx = i - 1;
                    if (modIdx < (int)Core::LoadedModList.size()) {
                        itemTitle = Core::LoadedModList[modIdx].name;
                        itemDesc = Core::LoadedModList[modIdx].description;
                        if (itemDesc.empty()) itemDesc = "No description provided.";
                    }
                }

                BoxedPtr::Shared<ModListItemButton> btn(gstd::make_unique<ModListItemButton>(
                    mMinecraftGame, btnId, 15, itemY, 290, 36, itemTitle, itemDesc));
                btn->buildBackground();
                mButtons.push_back(std::move(btn));
            }

            // Pagination controls
            if (totalItems > ITEMS_PER_PAGE) {
                if (mCurrentPage > 0) {
                    BoxedPtr::Shared<GuiButton> prevBtn(gstd::make_unique<GuiButton>(
                        mMinecraftGame, ButtonId::BUTTON_PREV_PAGE, 15, 200, 60, 24, "< Prev", false, INT_MAX));
                    prevBtn->buildBackground();
                    mButtons.push_back(std::move(prevBtn));
                }
                if (endIndex < totalItems) {
                    BoxedPtr::Shared<GuiButton> nextBtn(gstd::make_unique<GuiButton>(
                        mMinecraftGame, ButtonId::BUTTON_NEXT_PAGE, 245, 200, 60, 24, "Next >", false, INT_MAX));
                    nextBtn->buildBackground();
                    mButtons.push_back(std::move(nextBtn));
                }
            }
        }

        void buttonPressed(MC3DSPF::BaseButton &b) override
        {
            int id = b.getId();
            switch (id)
            {
                case ButtonId::BUTTON_EXIT:
                    MC3DSPF::Facade::getMinecraftGame()->getScreenStack().schedulePopScreen(1);
                    return;
                case ButtonId::BUTTON_PREV_PAGE:
                    if (mCurrentPage > 0) {
                        mCurrentPage--;
                        rebuildComponents();
                    }
                    return;
                case ButtonId::BUTTON_NEXT_PAGE:
                    if ((mCurrentPage + 1) * ITEMS_PER_PAGE < getTotalItems()) {
                        mCurrentPage++;
                        rebuildComponents();
                    }
                    return;
                default: break;
            }

            if (id >= BUTTON_ITEM_BASE && id < BUTTON_ITEM_BASE + ITEMS_PER_PAGE) {
                int slot = id - BUTTON_ITEM_BASE;
                int itemIndex = mCurrentPage * ITEMS_PER_PAGE + slot;

                if (itemIndex == 0) {
                    // LunaCore entry -> open SettingsScreen
                    MC3DSPF::ScreenChooser& chooser = MC3DSPF::Facade::getMinecraftGame()->getScreenChooser();
                    MC3DSPF::MinecraftGame* mg = MC3DSPF::Facade::getMinecraftGame();
                    MC3DSPF::ClientInstance* ci = MC3DSPF::Facade::getClient();
                    MC3DSPF::BoxedPtr::Shared<LunaCoreSettingsScreen> screen(
                        MC3DSPF::gstd::make_unique<LunaCoreSettingsScreen>(mg, ci));
                    chooser._pushScreen(screen);
                } else {
                    int modIdx = itemIndex - 1;
                    if (modIdx < (int)Core::LoadedModList.size()) {
                        const ModInfo& info = Core::LoadedModList[modIdx];
                        MC3DSPF::ScreenChooser& chooser = MC3DSPF::Facade::getMinecraftGame()->getScreenChooser();
                        MC3DSPF::MinecraftGame* mg = MC3DSPF::Facade::getMinecraftGame();
                        MC3DSPF::ClientInstance* ci = MC3DSPF::Facade::getClient();
                        MC3DSPF::BoxedPtr::Shared<LunaCoreModScreen> screen(
                            MC3DSPF::gstd::make_unique<LunaCoreModScreen>(mg, ci, info));
                        chooser._pushScreen(screen);
                    }
                }
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
            return "lunacore_mods_screen";
        }

        void render(int touchX, int touchY, int useScreen, float tick) override
        {
            if (useScreen & MC3DSPF::UseScreen::Top)
            {
                std::string footer("LunaCore v" STRINGIFY_MACRO(LUNACORE_VER_MAJOR) "." STRINGIFY_MACRO(LUNACORE_VER_MINOR) "." STRINGIFY_MACRO(LUNACORE_VER_PATCH));
                int h = mFont->getTextHeight(footer.c_str(), 0, 1);
                mFont->drawWithShadow(15, MC3DSPF::AppPlatform::TOP_SCREEN_HEIGHT - h - 5, 1.0f, footer.c_str(), MC3DSPF::Color(1.0f, 1.0f, 1.0f), 0, -1);
            }

            if (useScreen & MC3DSPF::UseScreen::Bottom)
            {
                mBackground->draw(MC3DSPF::Tessellator::instance, -2, 2);
                mTopBar->draw(MC3DSPF::Tessellator::instance, -2, 0);

                int totalCount = getTotalItems();
                char headerBuf[64];
                snprintf(headerBuf, sizeof(headerBuf), "Mods (%d)", totalCount);
                // mFont->drawWithShadow(13, 12, 1.0f, headerBuf, MC3DSPF::Color(0.45f, 0.45f, 0.45f), 0, -1);
                mFont->drawWithShadow(12, 12, 1.0f, headerBuf, MC3DSPF::Color(0, 0, 0), 0, -1);

                if (totalCount > ITEMS_PER_PAGE) {
                    int totalPages = (totalCount + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
                    char pageBuf[32];
                    snprintf(pageBuf, sizeof(pageBuf), "%d / %d", mCurrentPage + 1, totalPages);
                    mFont->drawWithShadow(145, 202, 1.0f, pageBuf, MC3DSPF::Color(1.0f, 1.0f, 1.0f), 0, -1);
                }

                Screen::render(touchX, touchY, useScreen, tick);
            }
        }
    };
}
