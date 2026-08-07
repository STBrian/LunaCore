#pragma once

#include <MC3DSPluginFramework.hpp>

namespace MinecraftAPI {
    namespace MC3DSPF = MC3DSPluginFramework;
    namespace BoxedPtr = MC3DSPF::BoxedPtr;
    namespace gstd = MC3DSPF::gstd;

    using MinecraftGame = MC3DSPF::MinecraftGame;

    class Platform3DS {
        static void TriggerOpenManual(void) {
            MinecraftGame* game = MC3DSPF::Facade::getMinecraftGame();
            *(reinterpret_cast<u8*>(game) + 0x3c7) = 1;
        }
    };

    using TextureRegion = MC3DSPF::IntRectangle;
    using ResourceLocation = MC3DSPF::ResourceLocation;

    template <typename Owner>
    struct Resource {
        TextureRegion region;
    };

    #define ADD_RESOURCE(name, x, y, w, h) \
        static inline const Resource<RES_OWNER> name = {{x, y, w, h}}

    struct GuiAtlas {
        #define RES_OWNER GuiAtlas
        static inline const ResourceLocation &location = *(ResourceLocation *)0xABFD74;

        ADD_RESOURCE(PLAY_ICON, 224, 128, 16, 16);
        ADD_RESOURCE(MULTIPLAYER_ICON, 224, 128, 16, 16);
        ADD_RESOURCE(OPTIONS_ICON, 224, 144, 16, 16);
        ADD_RESOURCE(SKINS_ICON, 224, 176, 16, 16);
        ADD_RESOURCE(ACHIEVEMENTS_ICON, 224, 160, 16, 16);
        ADD_RESOURCE(MANUAL_ICON, 240, 128, 16, 16);
        ADD_RESOURCE(STORE_ICON, 240, 144, 16, 16);

        ADD_RESOURCE(BUTTON_BG, 64, 144, 8, 8);
        ADD_RESOURCE(BUTTON_BG_HOVERED, 56, 144, 8, 8);

        ADD_RESOURCE(BUTTON_CLOSE_BG, 24, 128, 16, 16);
        ADD_RESOURCE(BUTTON_CLOSE_BG_HOVERED, 40, 128, 16, 16);

        ADD_RESOURCE(EMPTY, 0, 0, 8, 8);

        #undef RES_OWNER
    };

    #undef ADD_RESOURCE

    class IconButton : public MC3DSPF::IconButton {
        public:
        IconButton(MinecraftGame *minecraftGame, int id, int x, int y, int w, int h, const char *localizeKey, bool bigIcon = false) :
            MC3DSPF::IconButton(minecraftGame, id, x, y, w, h, localizeKey, bigIcon) {}

        void setTextures(const ResourceLocation& location, const TextureRegion& iconTex, const TextureRegion& bgSubtexUV, const TextureRegion& hoveredSubtexUV, u16 borderX = 2, u16 borderY = 2) {
            this->setTexture(location, iconTex.mX, iconTex.mY, iconTex.mWidth, iconTex.mHeight, 0, bgSubtexUV, hoveredSubtexUV, borderX, borderY, 0);
        }

        /* Sets the textures for the button. Use the same owner for all resources! */
        template <typename Owner>
        void setTextures(const Resource<Owner>& icon, const Resource<Owner>& bgNormal, const Resource<Owner>& bgHovered, u16 borderX = 2, u16 borderY = 2) {
            this->setTextures(Owner::location, icon.region, bgNormal.region, bgHovered.region, borderX, borderY);
        }
    };
}