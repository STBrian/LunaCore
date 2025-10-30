#pragma once

#include <CTRPluginFramework.hpp>

#include "CoreGlobals.hpp"
#include "lua_common.h"
#include "Minecraft/world/item/Item.hpp"

namespace Core {
    namespace Module {
        bool RegisterItemsModule(lua_State *L);
    }

    namespace Items {
        using Item = Game::Item;
        
        Item *SearchItemByName(const std::string& name);

        Item *SearchItemByID(u16 id);

        void* GetRenderIDByItemID(u16 id);

        u16 GetCreativeItemPositionOfGroup(u16 itemId, u16 groupId);
    }
}