#pragma once

#include "types.h"

namespace Minecraft {
    class Block {
        public:
        void* vtable; // maybe?
        u8 blockId;

        inline static Block** mBlocks = reinterpret_cast<Block**>(0x00b10520);
        inline static void** BlockModels = reinterpret_cast<void**>(0xac2ac4);
    };
}