#include "LuaModules.hpp"

#include <vector>
#include <CTRPluginFramework.hpp>
#include "MC3DSPluginFramework.hpp"

#include "string_hash.hpp"
#include "lua_utils.hpp"

#include "game/Minecraft.hpp"
#include "game/world/actor/player/Inventory.hpp"
#include "game/world/item/ItemInstance.hpp"

#include "Core/Utils/ItemUtils.hpp"

#include "Helpers/LuaObject.hpp"
#include "Helpers/LuaCustomTable.hpp"

namespace CTRPF = CTRPluginFramework;
using InventorySlot = Minecraft::Inventory::InventorySlot;
using Item = Game::Item;

// ----------------------------------------------------------------------------

//$Game.LocalPlayer.Inventory
//#$Game.LocalPlayer.Inventory.Slots : ---@type table<"hand"|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36,InventorySlot>
//#$Game.LocalPlayer.Inventory.ArmorSlots : ---@type table<"helmet"|"chestplate"|"leggings"|"boots"|1|2|3|4,InventorySlot>

// ----------------------------------------------------------------------------

static int l_Inventory_Slots_index(lua_State *L) {
    int index = 0;
    if (lua_type(L, 2) == LUA_TNUMBER)
        index = lua_tonumber(L, 2);
    else if (lua_type(L, 2) == LUA_TSTRING) {
        uint32_t key = hash(lua_tostring(L, 2));
        if (key == hash("hand"))
            index = Minecraft::GetHeldSlotNumber();
    }
    LuaObjectUtils::NewObjectCheck(L, "InventorySlot", (InventorySlot*)Minecraft::GetSlotAddress(index));
    return 1;
}

// ----------------------------------------------------------------------------

//@@InventorySlot

/*
## return: boolean
### InventorySlot:isEmpty
*/
static int l_InventorySlot_isEmpty(lua_State *L) {
    InventorySlot* slotData = LuaObjectUtils::CheckObject<InventorySlot>(L, 1, "InventorySlot").get();
    lua_pushboolean(L, slotData->itemCount == 0 || slotData->itemData == nullptr);
    return 1;
}

/*
## item: GameItem
## value: integer?
## return: boolean
### InventorySlot:setItem
*/
static int l_InventorySlot_setItem(lua_State *L) {
    auto slotData = LuaObjectUtils::CheckObject<MC3DSPluginFramework::ItemInstance>(L, 1, "InventorySlot").get();
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 2, "GameItem").get();
    u16 dataValue = 0;
    if (lua_gettop(L) > 2)
        dataValue = luaL_checknumber(L, 3);
    u8 count = slotData->mCount;
    if (count > item->maxStackSize)
        count = item->maxStackSize;
    slotData->init((MC3DSPluginFramework::ItemId)item->itemId, count, dataValue);
    return 1;
}

/*
#$InventorySlot.Item : ---@type GameItem?
=InventorySlot.ItemCount = 0
=InventorySlot.ItemData = 0
*/

// ----------------------------------------------------------------------------

static int l_Inventory_ArmorSlots_index(lua_State *L)
{
    int index = 0;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        index = lua_tonumber(L, 2);
    }
    else if (lua_type(L, 2) == LUA_TSTRING) {
        uint32_t key = hash(lua_tostring(L, 2));
        switch (key) {
            case hash("helmet"):
                index = 1;
                break;
            case hash("chestplate"):
                index = 2;
                break;
            case hash("leggings"):
                index = 3;
                break;
            case hash("boots"):
                index = 4;
                break;
        }
    }
    LuaObjectUtils::NewObjectCheck(L, "InventorySlot", (InventorySlot*)Minecraft::GetArmorSlotAddress(index));
    return 1;
}

// ----------------------------------------------------------------------------

template<>
struct LuaTypeName<Item> {
    static constexpr const char* value = "GameItem";
};

// Required to be called inside LocalPlayer definition
bool Core::Module::LocalPlayer::RegisterInventoryModule(lua_State *L) {
    ClassBuilder<InventorySlot>(L, "InventorySlot")
        .property("Item", &InventorySlot::itemData)
        .property("ItemCount", &InventorySlot::itemCount)
        .property("ItemData", &InventorySlot::dataValue)
        .method("isEmpty", l_InventorySlot_isEmpty)
        .method("setItem", l_InventorySlot_setItem)
        .build();

    LuaCustomTable::RegisterNewCustomTable(L, "Inventory");
    LuaCustomTable::RegisterNewCustomTable(L, "Inventory.Slots", l_Inventory_Slots_index);
    LuaCustomTable::RegisterNewCustomTable(L, "Inventory.ArmorSlots", l_Inventory_ArmorSlots_index);

    LuaCustomTable::GetIndexTable(L, "Inventory"); // LocalPlayer.Inventory

    LuaCustomTable::NewCustomTable(L, "Inventory.Slots"); // LocalPlayer.Inventory.Slots
    lua_setfield(L, -2, "Slots");
    LuaCustomTable::NewCustomTable(L, "Inventory.ArmorSlots"); // LocalPlayer.Inventory.ArmorSlots
    lua_setfield(L, -2, "ArmorSlots");

    lua_pop(L, 1); // Pop inventory index table
    LuaCustomTable::SetNewCustomTable(L, -1, "Inventory"); // Sets Inventory to LocalPLayer
    return true;
}