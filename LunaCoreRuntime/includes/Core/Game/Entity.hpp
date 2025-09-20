#pragma once

#include "lua_common.h"
#include "types.h"

namespace Core {
    namespace Module {
        bool RegisterEntityModule(lua_State *L);
    }

    namespace Entity {
        inline u32* mobHostileCount = (u32*)0x9338AC;
        inline u32* mobLandCount = (u32*)0x9338A8;
        inline u32* mobOceanCount = (u32*)0x9338B0;
        inline u32* dragonCount = (u32*)0x933898;
    }
}