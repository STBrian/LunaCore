#pragma once

#include "types.h"

#include "Game/world/item/Item.hpp"

namespace Game {
    class Inventory {
        using Item = Game::Item;
        
        public:
        typedef struct {
            u8 itemCount;
            u8 unk1 = 0;
            u16 dataValue = 0;
            bool show = true;
            u8 unk2[3] = {};
            u32 enchant = 0;
            Item* itemData = nullptr;
            void* renderID = nullptr;
            u8 unk3[28] = {};
        } InventorySlot;
    };
}
