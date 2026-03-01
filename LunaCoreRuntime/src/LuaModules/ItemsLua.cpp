#include "LuaModules.hpp"

#include <cstring>
#include "string_hash.hpp"

#include "Core/Event.hpp"
#include "Core/Utils/ItemUtils.hpp"

#include "game/Minecraft.hpp"

#include "Helpers/Allocation.hpp"
#include "Helpers/LuaObject.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;
using Item = Game::Item;
using ItemInstance = Game::ItemInstance;

// ----------------------------------------------------------------------------

//$Game.Items
//@@GameItem
//@@GameItemInstance

// ----------------------------------------------------------------------------

/*
- Find an item using its ID
## name: string
## return: GameItem?
### Game.Items.findItemByName
*/
static int l_Items_findItemByName(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Item *itemData = Core::Items::SearchItemByName(name);
    LuaObjectUtils::NewObjectCheck(L, "GameItem", itemData);
    return 1;
}

/*
- Find and item using its name
## itemID: integer
## return: GameItem?
### Game.Items.findItemByID
*/
static int l_Items_findItemByID(lua_State *L) {
    u16 itemID = luaL_checknumber(L, 1);
    Item *itemData = Core::Items::SearchItemByID(itemID);
    LuaObjectUtils::NewObjectCheck(L, "GameItem", itemData);
    return 1;
}

/*
- Get the item position in creative using the id
## itemID: integer
## groupID: integer
## return: number
### Game.Items.getCreativePosition
*/
static int l_Items_getCreativePosition(lua_State *L) {
    u16 itemID = luaL_checknumber(L, 1);
    u16 groupID = luaL_checknumber(L, 2);
    u16 position = Core::Items::GetCreativeItemPositionOfGroup(itemID, groupID);
    lua_pushnumber(L, position);
    return 1;
}

/*
- Creates a new item and stores it in the game's items table. Returns the address to the item
## itemName: string
## itemId: integer
## return: GameItem?
### Game.Items.registerItem
*/
static int l_Items_registerItem(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item* regItem = Game::registerItem(itemName, itemId);
    LuaObjectUtils::NewObjectCheck(L, "GameItem", regItem);
    return 1;
}

/*
- Takes a registered item with Game.Items.registerItem, and sets its texture
## item: GameItem
## textureName: string
## textureIndex: integer
### GameItem:setTexture
*/
static int l_Items_registerItemTexture(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "GameItem").get();
    const char* textureName = luaL_checkstring(L, 2);
    u16 textureIndex = luaL_checkinteger(L, 3);
    item->setTexture(hash(textureName), textureIndex);
    return 0;
}

/*
- Takes a registered item with Game.Items.registerItem, and registers it in creative menu
## item: GameItem
## groupId: integer
## position: integer
### Game.Items.registerCreativeItem
*/
static int l_Items_registerCreativeItem(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "GameItem").get();
    u16 groupId = luaL_checkinteger(L, 2);
    s16 position = luaL_checkinteger(L, 3);
    Item::addCreativeItem(item, groupId, position);
    return 0;
}

/*
- Returns a GameItemInstance
## item: GameItem
## count: integer
## data: integer
## return: GameItemInstance
### Game.Items.getItemInstance
*/
static int l_Items_getItemInstance(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "GameItem").get();
    u16 count = luaL_checkinteger(L, 2);
    u16 data = luaL_checkinteger(L, 3);
    ItemInstance* ins = alloc<ItemInstance>(item, count, data);
    LuaObjectUtils::NewObject(L, "GameItemInstance", ins);
    return 1;
}

static const luaL_Reg items_functions[] = {
    {"findItemByName", l_Items_findItemByName},
    {"findItemByID", l_Items_findItemByID},
    {"getCreativePosition", l_Items_getCreativePosition},
    {"registerItem", l_Items_registerItem},
    {"registerCreativeItem", l_Items_registerCreativeItem},
    {"getItemInstance", l_Items_getItemInstance},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

/*
=GameItem.StackSize = 64
=GameItem.ID = 1
=GameItem.NameID = ""
=GameItem.DescriptionID = ""
*/

static int l_gc_GameItemInstance(lua_State* L) {
    LuaObject<ItemInstance> ptr = LuaObjectUtils::CheckObject<ItemInstance>(L, 1, "GameItemInstance");
    Core::dealloc(ptr.get());
    return 0;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterItemsModule(lua_State *L) {
    ClassBuilder<Item>(L, "GameItem")
        .property("StackSize", &Item::maxStackSize)
        .property("ID", &Item::itemId, true)
        .property_get<Item, const char*>("NameID", [](const Item& i) -> const char* {
            return i.nameId.c_str();
        })
        .property_get<Item, const char*>("DescriptionID", [](const Item& i) -> const char* {
            return i.descriptionId.c_str();
        })
        .method("setTexture", l_Items_registerItemTexture)
        .build();
    ClassBuilder<ItemInstance>(L, "GameItemInstance")
        .gc(l_gc_GameItemInstance)
        .build();

    lua_getglobal(L, "Game");
    lua_newtable(L); // Items

    //$@@@Game.Items.OnRegisterItems: EventClass
    Core::Event::NewEvent(L, "OnRegisterItems");

    //$@@@Game.Items.OnRegisterItemsTextures: EventClass
    Core::Event::NewEvent(L, "OnRegisterItemsTextures");

    //$@@@Game.Items.OnRegisterCreativeItems: EventClass
    Core::Event::NewEvent(L, "OnRegisterCreativeItems");

    luaL_register(L, NULL, items_functions);

    lua_setfield(L, -2, "Items");
    lua_pop(L, 1);
    return true;
}