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
    };
}