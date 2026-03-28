#pragma once

#include "types.h"

#include "game/world/item/Item.hpp"

namespace Core::Items {
    
    Minecraft::Item *SearchItemByName(const std::string& name);

    Minecraft::Item *SearchItemByID(u16 id);

    void* GetRenderIDByItemID(u16 id);

    u16 GetCreativeItemPositionOfGroup(u16 itemId, u16 groupId);
}