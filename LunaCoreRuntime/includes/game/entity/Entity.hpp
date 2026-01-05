#pragma once

#include "types.h"

namespace Game {
    class Entity {
        public:
        u32 padding1[133]; 
        float x;
        float y;
        float z;
        u32 padding2[22];
        u32 entityTypeId;

        inline static u32* mobHostileCount = (u32*)0x9338AC;
        inline static u32* mobLandCount = (u32*)0x9338A8;
        inline static u32* mobOceanCount = (u32*)0x9338B0;
        inline static u32* dragonCount = (u32*)0x933898;
    };
}