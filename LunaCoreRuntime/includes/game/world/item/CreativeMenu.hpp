#pragma once

#include "game/gstd/gstd_vector.hpp"
#include "game/world/item/ItemInstance.hpp"

namespace Minecraft::Creative {
    inline gstd::vector<ItemInstance*>& InventoryItems = *reinterpret_cast<gstd::vector<ItemInstance*>*>(0x00b0d744);

    inline static void addCreativeItem(Item* item, u8 categoryId, s16 position) {
            ItemInstance itemins(item->itemId, 1, 0); // Def values
            itemins.unknown1 = categoryId;
            itemins.unknown2 = position;
            reinterpret_cast<void(*)(ItemInstance*)>(0x0056e108)(&itemins);
        }
}