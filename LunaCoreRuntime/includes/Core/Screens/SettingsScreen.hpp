#pragma once

#include "Api/Facade.hpp"
#include "Minecraft/Common/Client/Gui/Components/IconButton.hpp"
#include "Minecraft/Common/Client/Gui/Components/NinePatch.hpp"
#include "Minecraft/Common/Client/Gui/Components/OptionItem.hpp"
#include "Minecraft/Common/Client/Gui/Components/Slider.hpp"
#include "Minecraft/Common/Client/Gui/Screens/Screen.hpp"
#include "Minecraft/Common/Client/Gui/Screens/ScreenStack.hpp"
#include "Minecraft/Common/Client/Gui/Screens/SystemMessagesScreen.hpp"
#include "Minecraft/Common/Platform/AppPlatform.hpp"
#include <CTRPluginFramework.hpp>

#include "GameAPI.hpp"

#ifndef STRINGIFY_MACRO
#define STRINGIFY_MACRO(x) STRINGIFY_MACRO_HELPER(x)
#define STRINGIFY_MACRO_HELPER(x) #x   
#endif

namespace Core {
    namespace CTRPF = CTRPluginFramework;
    namespace MC3DSPF = MC3DSPluginFramework;
    
    class LunaCoreSettingsScreen : public MC3DSPF::Screen
    {
        USE_GAME_ALLOCATOR
    private:
        enum ButtonId {
            BUTTON_EXIT,
            BUTTON_OPTIONS,
        };

    public:
        LunaCoreSettingsScreen(MC3DSPF::MinecraftGame *mg, MC3DSPF::ClientInstance *ci) :
            Screen(mg, ci) {}

        ~LunaCoreSettingsScreen() override = default;

        void setupComponents() override
        {
            // IntRectangle uv1(64, 144, 8, 8);
            // IntRectangle uv2(56, 144, 8, 8);
            // void *unk = (void *)0xABFD74;
            // int Y     = 15;

            // BoxedPtr::Shared<IconButton> button1(std::make_unique<IconButton>(mMinecraftGame, 1, 110, Y, 200, 0x1C, "menu.options", false));
            // button1->setTexture(unk, 224, 144, 16, 16, 0, uv1, uv2, 2, 2, 0);
            // mButtons.push_back(std::move(button1));

            MC3DSPF::NinePatchFactory factory(mMinecraftGame->getTextures(), *(MC3DSPF::ResourceLocation *)0xABFD74);
            mBackgroundFrame = factory.createSymmetrical(MC3DSPF::IntRectangle(56, 160, 8, 8), 2, 2, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH, MC3DSPF::AppPlatform::BOTTOM_SCREEN_HEIGHT);
            mBackground      = factory.createSymmetrical(MC3DSPF::IntRectangle(80, 176, 8, 8), 3, 3, 308, 204);

            MC3DSPF::ResourceLocation &loc = *(MC3DSPF::ResourceLocation *)0xABFD74;
            mSprite1              = MC3DSPF::gstd::make_unique<MC3DSPF::Sprite>(mMinecraftGame, 7, 7, 16, 16, loc, 200, 25, 8, 8);

            using namespace MinecraftAPI;

            BoxedPtr::Shared<IconButton> exitButton(gstd::make_unique<IconButton>(
                mMinecraftGame, ButtonId::BUTTON_EXIT, MC3DSPF::AppPlatform::BOTTOM_SCREEN_WIDTH - 7 - 16, 7, 16, 16, "", false));
            exitButton->setTextures(GuiAtlas::EMPTY, GuiAtlas::BUTTON_CLOSE_BG, GuiAtlas::BUTTON_CLOSE_BG_HOVERED, 0, 0);
            mButtons.push_back(std::move(exitButton));

            BoxedPtr::Shared<IconButton> optionsButton(gstd::make_unique<IconButton>(
                mMinecraftGame, ButtonId::BUTTON_OPTIONS, 16, 40, 28, 28, "Plugin Menu", false));
            optionsButton->setTextures(GuiAtlas::OPTIONS_ICON, GuiAtlas::BUTTON_BG, GuiAtlas::BUTTON_BG_HOVERED, 2, 2);
            mButtons.push_back(std::move(optionsButton));
        }

        void buttonPressed(MC3DSPF::BaseButton &b) override
        {
            switch (b.getId())
            {
                default: break;
                case ButtonId::BUTTON_EXIT:
                    MC3DSPF::Facade::getMinecraftGame()->getScreenStack().schedulePopScreen(1);
                    break;
                case ButtonId::BUTTON_OPTIONS:
                    CTRPF::PluginMenu::ForceOpen();
                    break;
            }
        }

        bool onBack(int p1) override
        {
            // MC3DSPF::SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return true;
        }

        /*
        // handleInput?
        void Unknown82(u32 unknown) override
        {
        }
        */

        // +0x18
        void Unknown5() override
        {
            // MC3DSPF::SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return;
        }

        // 表示したときに呼ばれる
        // +0x1C
        void Unknown6() override
        {
            // MC3DSPF::SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return;
        }

        void Unknown24(int p1) override
        {
            // MC3DSPF::SystemMessagesScreen::RunningInstance->pushMessage(CTRPluginFramework::Utils::ToHex(p1).c_str(), 0);
            return;
        }

        // tick?
        // trueを返すようにしないとバグるっぽい？
        // LevelRendererで呼ばれてる
        bool Unknown40() override
        {
            return true;
        }

        // cスティックとスライドパッドを動かすと呼ばれる
        bool disableCstick() override
        {
            return *(bool *)0x918F01;
        }

        // ZLZR、左右十字でのホットバー操作を受け付けるか
        bool allowsHotbarInput() override
        {
            return false;
        }

        // 下画面タッチ時に二回呼ばれて、他のボタンを押したあとに下画面タッチすると、再度2回呼ばれる
        // UIが手前に来た際にも一度呼ばれる
        bool Unknown45() override
        {
            return *(bool *)0x918F03;
        }

        bool Unknown46() override
        {
            // SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return *(bool *)0x918F04;
        }

        // 他のスクリーンよりも手前表示
        bool isOverlay() override
        {
            return true;
        }

        bool Unknown51() override
        {
            // SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return *(bool *)0x918F06;
        }

        // 他スクリーンがあると非表示
        // isOverlayより優先される
        bool renderBehind() override
        {
            return false;
        }

        // ポーズ中以外の時に呼ばれてる
        bool Unknown55() override
        {
            // SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return *(bool *)0x918F08;
        }

        // 他スクリーンのスティック、XY、Start、Selectが無効化される
        // もしかしたら、他のスクリーンを開くことができるかのフラグかもしれない
        /*
        bool disablesStickInput() const override
        {
            // SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return *(bool *)0x918F09;
        }
        */

        void Unknown86(int p1, float x, float y) override
        {
            // Cスティック動かすとunknown3は1、離すとunknown3が3になる。スティックを動かすと0
            // unknown3はInputHandlerのメンバーから来てる

            // SystemMessagesScreen::RunningInstance->pushMessage(std::to_string(x).c_str(), 0);
            // SystemMessagesScreen::RunningInstance->pushMessage(std::to_string(y).c_str(), 0);
            // SystemMessagesScreen::RunningInstance->pushMessage(CTRPluginFramework::Utils::ToHex(unknown3).c_str(), 0);
        }

        bool Unknown87() override
        {
            // MC3DSPF::SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return *(bool *)0x918F0A;
        }

        bool Unknown96() override
        {
            // MC3DSPF::SystemMessagesScreen::RunningInstance->pushMessage(__func__, 0);
            return *(bool *)0x918F0B;
        }

        int getUseScreen() override
        {
            return MC3DSPF::UseScreen::Both;
        }

        MC3DSPF::gstd::string getName() override
        {
            return "lunacore_settings_screen";
        }

        void render(int touchX, int touchY, int useScreen, float tick) override
        {
            if (useScreen & MC3DSPF::UseScreen::Top)
            {
                MC3DSPF::gstd::string str("LunaCore v" STRINGIFY_MACRO(LUNACORE_VER_MAJOR) "." STRINGIFY_MACRO(LUNACORE_VER_MINOR) "." STRINGIFY_MACRO(LUNACORE_VER_PATCH));
                // int          w = mFont->getTextWidth(str, 0, 1);
                int          h = mFont->getTextHeight(str, 0, 1);
                // GuiComponent::drawRect(0, 0, w + 10, h + 10, MC3DSPF::Color(0, 0, 0, 0.5f));
                mFont->drawWithShadow(10, MC3DSPF::AppPlatform::TOP_SCREEN_HEIGHT - h, 5, str, MC3DSPF::Color(1, 1, 1), 0, -1);
            }

            if (useScreen & MC3DSPF::UseScreen::Bottom)
            {
                mBackgroundFrame->draw(MC3DSPF::Tessellator::instance, 0, 0);
                mBackground->draw(MC3DSPF::Tessellator::instance, 6, 30);
                mSprite1->render();
                Screen::render(touchX, touchY, useScreen, tick);
            }
        }

    private:
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackgroundFrame;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mBackground;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::Sprite>         mSprite1;
    };
}