#include "Core/Game/Player/Inventory.hpp"

#include <vector>

#include <CTRPluginFramework.hpp>

#include "Core/Game/Items.hpp"
#include "string_hash.hpp"

#include "lua_object.hpp"

#include "Minecraft/Minecraft.hpp"
#include "Minecraft/Inventory.hpp"

namespace CTRPF = CTRPluginFramework;
using InventorySlot = Game::Inventory::InventorySlot;
using Item = Game::Item;

// ----------------------------------------------------------------------------

//$Game.LocalPlayer.Inventory
//#$Game.LocalPlayer.Inventory.Slots : ---@type table<"hand"|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36,InventorySlot>
//#$Game.LocalPlayer.Inventory.ArmorSlots : ---@type table<"helmet"|"chestplate"|"leggings"|"boots"|1|2|3|4,InventorySlot>

// ----------------------------------------------------------------------------

//@@InventorySlot

/*
## return: boolean
### InventorySlot:isEmpty
*/
static int l_InventorySlot_isEmpty(lua_State *L) {
    InventorySlot* slotData = *(InventorySlot**)luaC_funccheckudata(L, 1, "InventorySlot");
    lua_pushboolean(L, slotData->itemCount == 0 || slotData->itemData == nullptr);
    return 1;
}

// ----------------------------------------------------------------------------

/*
#$InventorySlot.Item : ---@type GameItem?
=InventorySlot.ItemCount = 0
=InventorySlot.ItemData = 0
*/

static int l_Inventory_Slots_index(lua_State *L) {
    int index = 0;
    if (lua_type(L, 2) == LUA_TNUMBER)
        index = lua_tonumber(L, 2);
    else if (lua_type(L, 2) == LUA_TSTRING) {
        uint32_t key = hash(lua_tostring(L, 2));
        if (key == hash("hand"))
            index = Minecraft::GetHeldSlotNumber();
    }
    InventorySlot* ptr = (InventorySlot*)Minecraft::GetSlotAddress(index);
    if (ptr) {
        InventorySlot** invSlot_ptr = (InventorySlot**)lua_newuserdata(L, sizeof(void*));
        *invSlot_ptr = ptr;
        luaC_setmetatable(L, "InventorySlot");
        return 1;
    }
    return 0;
}

// ----------------------------------------------------------------------------

static int l_Inventory_Slot_class_index(lua_State *L)
{
    InventorySlot* slotData = *(InventorySlot**)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;
    uint32_t key = hash(lua_tostring(L, 2));
    switch (key) {
        case hash("isEmpty"):
            lua_pushcfunction(L, l_InventorySlot_isEmpty);
            break;
        case hash("Item"): {
            if (slotData->itemData)
                LuaObject::NewObject(L, "GameItem", slotData->itemData);
            else 
                lua_pushnil(L);
            break;
        }
        case hash("ItemCount"):
            lua_pushnumber(L, slotData->itemCount);
            break;
        case hash("ItemData"):
            lua_pushnumber(L, slotData->dataValue);
            break;
        default:
            lua_pushnil(L);
            break;
    }
    return 1;
}

static int l_Inventory_Slot_class_newindex(lua_State *L)
{
    InventorySlot* slotData = *(InventorySlot**)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "Unable to set field '?' of 'InventorySlot' instance");
    uint32_t key = hash(lua_tostring(L, 2));
    switch (key) {
        case hash("Item"): {
            Item *itemData = *(Item**)luaC_checkudata(L, 3, "GameItem", "unable to assign %s to %s data type");
            void* renderID = Core::Items::GetRenderIDByItemID(itemData->itemId);
            slotData->itemData = itemData;
            //if (renderID != nullptr)
                //slotData->renderID = renderID;
            break;
        }
        case hash("ItemCount"):
            slotData->itemCount = luaC_indexchecknumber(L, 3);
            break;
        case hash("ItemData"):
            slotData->dataValue = luaC_indexchecknumber(L, 3);
            break;
        default:
            luaL_error(L, "Unable to set field '%s' of 'InventorySlot' instance", lua_tostring(L, 2));
            break;
    }
    return 0;
}

// ----------------------------------------------------------------------------

/*
@@InventoryArmorSlot
=InventoryArmorSlot.Slot = 0
#$InventoryArmorSlot.Item : ---@type GameItem
=InventoryArmorSlot.ItemData = 0
=InventoryArmorSlot.ItemName = ""
*/

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
    if (index != 0 && Minecraft::GetArmorSlotAddress(index) == 0)
        index = 0;
    if (index >= 1 && index <= 4) {
        InventorySlot** invSlot_ptr = (InventorySlot**)lua_newuserdata(L, sizeof(void*));
        *invSlot_ptr = (InventorySlot*)Minecraft::GetArmorSlotAddress(index);
        luaC_setmetatable(L, "InventorySlot");
        return 1;
    }
    return 0;
}

// ----------------------------------------------------------------------------

static inline void RegisterInventoryMetatables(lua_State *L)
{
    luaL_newmetatable(L, "InventorySlotsMetatable");
    lua_pushcfunction(L, l_Inventory_Slots_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaC_invalid_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushstring(L, "InventorySlotsMetatable");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    luaL_newmetatable(L, "InventorySlot");
    lua_pushcfunction(L, l_Inventory_Slot_class_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_Inventory_Slot_class_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushstring(L, "InventorySlot");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    luaL_newmetatable(L, "InventoryArmorSlotsMetatable");
    lua_pushcfunction(L, l_Inventory_ArmorSlots_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaC_invalid_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushstring(L, "InventoryArmorSlotsMetatable");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);
}

// Required to be called inside LocalPlayer definition
bool Core::Module::LocalPlayer::RegisterInventoryModule(lua_State *L)
{
    RegisterInventoryMetatables(L);

    lua_newtable(L); // LocalPlayer.Inventory
    lua_newtable(L); // LocalPlayer.Inventory.Slots
    luaC_setmetatable(L, "InventorySlotsMetatable");
    lua_setfield(L, -2, "Slots");
    lua_newtable(L); // LocalPlayer.Inventory.ArmorSlots
    luaC_setmetatable(L, "InventoryArmorSlotsMetatable");
    lua_setfield(L, -2, "ArmorSlots");
    lua_setfield(L, -2, "Inventory");
    return true;
}