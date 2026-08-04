#pragma once

#include <MC3DSPluginFramework.hpp>

namespace Minecraft {
    namespace MC3DSPF = MC3DSPluginFramework;

    class Platform3DS {
        void TriggerOpenManual(void) {
            MC3DSPF::MinecraftGame* game = MC3DSPluginFramework::Facade::getMinecraftGame();
            *(reinterpret_cast<u8*>(game) + 0x3c7) = 1;
        }
    };
}