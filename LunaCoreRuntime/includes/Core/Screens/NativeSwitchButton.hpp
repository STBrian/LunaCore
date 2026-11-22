#pragma once

#include "Minecraft/Common/Client/Gui/Components/GuiButton.hpp"
#include "Minecraft/Common/Client/Gui/Components/NinePatch.hpp"
#include "Minecraft/Common/Client/Game/MinecraftGame.hpp"
#include "Minecraft/Common/Client/Gui/Gui/Font.hpp"
#include "Minecraft/Common/Client/Renderer/Renderer/Tessellator.hpp"
#include <string>
#include <functional>
#include <climits>

namespace Core {
    namespace MC3DSPF = MC3DSPluginFramework;

    class NativeSwitchButton : public MC3DSPF::GuiButton
    {
        USE_GAME_ALLOCATOR
    public:
        using ToggleCallback = std::function<void(bool)>;

    private:
        bool mChecked;
        std::string mCustomLabel;
        ToggleCallback mCallback;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mTrack;
        MC3DSPF::gstd::unique_ptr<MC3DSPF::NinePatchLayer> mThumb;

    public:
        NativeSwitchButton(MC3DSPF::MinecraftGame *mg, int id, int x, int y, int w, int h,
                           const std::string &label, bool initialState,
                           ToggleCallback callback = nullptr) :
            MC3DSPF::GuiButton(mg, id, x, y, w, h, "", false, INT_MAX),
            mChecked(initialState),
            mCustomLabel(label),
            mCallback(callback)
        {
            buildBackground();

            // Native GUI spritesheet (0x00ABFD74)
            MC3DSPF::NinePatchFactory factory(mMinecraftGame->getTextures(), *(MC3DSPF::ResourceLocation *)0xABFD74);

            // Native track region: UV (0, 195, 33, 18), margins (4, 4), display size 33x18
            mTrack = factory.createSymmetrical(MC3DSPF::IntRectangle(0, 195, 33, 18), 4, 4, 33.0f, 18.0f);

            // Native thumb region: UV (72, 184, 14, 14), margins (4, 4), display size 12x18
            mThumb = factory.createSymmetrical(MC3DSPF::IntRectangle(72, 184, 14, 14), 4, 4, 12.0f, 18.0f);
        }

        ~NativeSwitchButton() override = default;

        bool isChecked() const { return mChecked; }
        void setChecked(bool checked) { mChecked = checked; }

        static void playToggleSound(MC3DSPF::MinecraftGame *game)
        {
            if (game)
            {
                // MinecraftGame_playSound (0x230B18) with 'random.click' precomputed hash 0x7E943735
                reinterpret_cast<void (*)(MC3DSPF::MinecraftGame *, u32, float, float)>(0x230B18)(game, 0x7E943735, 1.0f, 1.0f);
            }
        }

        void toggle()
        {
            mChecked = !mChecked;
            playToggleSound(mMinecraftGame);
            if (mCallback)
            {
                mCallback(mChecked);
            }
        }

        void render(MC3DSPF::MinecraftGame *minecraftGame, int touchX, int touchY) override
        {
            // Draw button background if row is wider than standalone toggle
            if (mWidth > 48)
            {
                MC3DSPF::GuiButton::render(minecraftGame, touchX, touchY);
            }

            // Draw custom label aligned to the left
            if (!mCustomLabel.empty())
            {
                MC3DSPF::Font &font = mMinecraftGame->getDefaultFont();
                int textH = font.getTextHeight(mCustomLabel.c_str(), 0, 1);
                int textY = mY + (mHeight - textH) / 2;
                font.drawWithShadow(mX + 12, textY, 1.0f, mCustomLabel.c_str(), MC3DSPF::Color::BLACK, 0, -1);
            }

            // Calculate track position (right aligned if row, centered if standalone)
            float trackX, trackY;
            if (mWidth > 48)
            {
                trackX = static_cast<float>(mX + mWidth - 45);
                trackY = static_cast<float>(mY + (mHeight - 18) / 2);
            }
            else
            {
                trackX = static_cast<float>(mX + (mWidth - 33) / 2);
                trackY = static_cast<float>(mY + (mHeight - 18) / 2);
            }

            // Draw native track
            if (mTrack)
            {
                mTrack->draw(MC3DSPF::Tessellator::instance, trackX, trackY);
            }

            // Draw native thumb (offset: 0px when OFF, +21px when ON)
            if (mThumb)
            {
                float thumbX = trackX + (mChecked ? 21.0f : 0.0f);
                mThumb->draw(MC3DSPF::Tessellator::instance, thumbX, trackY);
            }
        }
    };
}
