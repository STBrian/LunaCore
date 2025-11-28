#include "Core/GameModules/Items.hpp"

#include <cstring>
#include "string_hash.hpp"
#include "lua_object.hpp"

#include "Core/Event.hpp"

#include "game/Minecraft.hpp"

namespace CTRPF = CTRPluginFramework;
using Item = Game::Item;
using ItemInstance = Game::ItemInstance;

Item* Core::Items::SearchItemByName(const std::string& name) {
    short i = 1;
    while (i <= Item::MAX_ITEMS) {
        if (Item::mItems[i] != nullptr) {
            if (Item::mItems[i]->nameId == name)
                return Item::mItems[i];
        }
        i++;
    }
    return nullptr;
}

Item *Core::Items::SearchItemByID(u16 id) {
    if (id <= Item::MAX_ITEMS)
        return Item::mItems[id];
    return nullptr;
}

void* Core::Items::GetRenderIDByItemID(u16 id) {
    /*if (id <= Item::MAX_ITEMS) {
        if (Item::renderTable[id] != nullptr) {
            return Item::renderTable[id];
        }
    }*/
    return nullptr;
}

u16 Core::Items::GetCreativeItemPositionOfGroup(u16 itemId, u16 groupId) {
    Game::ItemInstance* actualPos = *Item::creativeItems;
    while (actualPos < *Item::creativeItemsEnd) {
        if (actualPos->unknown1 == groupId && (*reinterpret_cast<Item**>((u32)(actualPos) + 0xc))->itemId == itemId)
            return actualPos->unknown2;
        actualPos = actualPos + 1;
    }
    return 0x7FFF;
}

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
    if (itemData == NULL)
        lua_pushnil(L);
    else
        LuaObject::NewObject(L, "GameItem", itemData);
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
    if (itemData == NULL)
        lua_pushnil(L);
    else
        LuaObject::NewObject(L, "GameItem", itemData);
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
    if (regItem == nullptr)
        lua_pushnil(L);
    else
        LuaObject::NewObject(L, "GameItem", regItem);
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
    Item* item_ptr = *(Item**)LuaObject::CheckObject(L, 1, "GameItem");
    const char* textureName = luaL_checkstring(L, 2);
    u16 textureIndex = luaL_checkinteger(L, 3);
    item_ptr->setTexture(hash(textureName), textureIndex);
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
    Item* item_ptr = *(Item**)LuaObject::CheckObject(L, 1, "GameItem");
    u16 groupId = luaL_checkinteger(L, 2);
    s16 position = luaL_checkinteger(L, 3);
    Item::addCreativeItem(item_ptr, groupId, position);
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
    Item *itemData = *(Item**)LuaObject::CheckObject(L, 1, "GameItem");
    u16 count = luaL_checkinteger(L, 2);
    u16 data = luaL_checkinteger(L, 3);
    ItemInstance* ins = new ItemInstance(itemData, count, data);
    LuaObject::NewObject(L, "GameItemInstance", ins);
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

static const LuaObjectField GameItemFields[] = {
    {"StackSize", OBJF_TYPE_SHORT, offsetof(Item, maxStackSize)},
    {"ID", OBJF_TYPE_SHORT, offsetof(Item, itemId), OBJF_ACCESS_INDEX},
    {"NameID", OBJF_TYPE_STRING, offsetof(Item, nameId), OBJF_ACCESS_INDEX},
    {"DescriptionID", OBJF_TYPE_STRING, offsetof(Item, descriptionId), OBJF_ACCESS_INDEX},
    {"setTexture", OBJF_TYPE_METHOD, (u32)l_Items_registerItemTexture},
    {NULL, OBJF_TYPE_NIL, 0}
};

static int l_gc_GameItemInstance(lua_State* L) {
    ItemInstance* ptr = *(ItemInstance**)LuaObject::CheckObject(L, 1, "GameItemInstance");
    delete ptr;
    return 0;
}

static const LuaObjectField GameItemInstanceFields[] = {
    {"dummy", OBJF_TYPE_SHORT, 0},
    {NULL, OBJF_TYPE_NIL, 0}
};

bool Core::Module::RegisterItemsModule(lua_State *L) {
    LuaObject::RegisterNewObject(L, "GameItem", GameItemFields);
    LuaObject::RegisterNewObject(L, "GameItemInstance", GameItemInstanceFields);
    LuaObject::SetGCObjectField(L, "GameItemInstance", l_gc_GameItemInstance);
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