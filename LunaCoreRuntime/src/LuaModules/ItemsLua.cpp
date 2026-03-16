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
using Item = Minecraft::Item;
using ItemInstance = Minecraft::ItemInstance;

// ----------------------------------------------------------------------------

//$Game.Items
//@@MCItem
//@@MCItemInstance
//@@MCToolTier

// ----------------------------------------------------------------------------

/*
- Find an item using its ID
## name: string
## return: MCItem?
### Game.Items.findItemByName
*/
static int l_Items_findItemByName(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Item *itemData = Core::Items::SearchItemByName(name);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", itemData);
    return 1;
}

/*
- Find and item using its name
## itemID: integer
## return: MCItem?
### Game.Items.findItemByID
*/
static int l_Items_findItemByID(lua_State *L) {
    u16 itemID = luaL_checknumber(L, 1);
    Item *itemData = Core::Items::SearchItemByID(itemID);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", itemData);
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
- Returns a new empty ToolTier
## return: MCToolTier
### Game.Items.newToolTier
*/
static int l_Items_newToolTier(lua_State *L) {
    LuaObjectUtils::NewObject(L, "MCToolTier", alloc<Item::Tier>());
    return 1;
}

/*
- Creates a new item and stores it in the game's items table. Returns the address to the item
## itemName: string
## itemId: integer
## return: MCItem?
### Game.Items.registerItem
*/
static int l_Items_registerItem(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item* regItem = Minecraft::registerItem<Item>(itemName, itemId);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/*
- Creates a new sword tool item and registers it in the game. Returns the address to the item
## itemName: string
## itemId: integer
## tier: MCToolTier
## return: MCItem?
### Game.Items.registerSwordItem
*/
static int l_Items_registerItemSwordTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemSwordTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/*
- Creates a new hoe tool item and registers it in the game. Returns the address to the item
## itemName: string
## itemId: integer
## tier: MCToolTier
## return: MCItem?
### Game.Items.registerHoeItem
*/
static int l_Items_registerItemHoeTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemHoeTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/*
- Creates a new axe tool item and registers it in the game. Returns the address to the item
## itemName: string
## itemId: integer
## tier: MCToolTier
## return: MCItem?
### Game.Items.registerAxeItem
*/
static int l_Items_registerItemAxeTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemAxeTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/*
- Creates a new pickaxe tool item and registers it in the game. Returns the address to the item
## itemName: string
## itemId: integer
## tier: MCToolTier
## return: MCItem?
### Game.Items.registerPickaxeItem
*/
static int l_Items_registerItemPickaxeTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemPickaxeTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/*
- Creates a new shovel tool item and registers it in the game. Returns the address to the item
## itemName: string
## itemId: integer
## tier: MCToolTier
## return: MCItem?
### Game.Items.registerShovelItem
*/
static int l_Items_registerItemShovelTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemShovelTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/*
- Takes a registered item with Game.Items.registerItem, and sets its texture
## item: MCItem
## textureName: string
## textureIndex: integer
### MCItem:setTexture
*/
static int l_Items_registerItemTexture(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "MCItem").get();
    const char* textureName = luaL_checkstring(L, 2);
    u16 textureIndex = luaL_checkinteger(L, 3);
    item->setTexture(hash(textureName), textureIndex);
    return 0;
}

/*
- Takes a registered item with Game.Items.registerItem, and registers it in creative menu
## item: MCItem
## groupId: integer
## position: integer
### Game.Items.registerCreativeItem
*/
static int l_Items_registerCreativeItem(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "MCItem").get();
    u16 groupId = luaL_checkinteger(L, 2);
    s16 position = luaL_checkinteger(L, 3);
    Item::addCreativeItem(item, groupId, position);
    return 0;
}

/*
- Returns a MCItemInstance
## item: MCItem
## count: integer
## data: integer
## return: MCItemInstance
### Game.Items.getItemInstance
*/
static int l_Items_getItemInstance(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "MCItem").get();
    u16 count = luaL_checkinteger(L, 2);
    u16 data = luaL_checkinteger(L, 3);
    ItemInstance* ins = alloc<ItemInstance>(item, count, data);
    LuaObjectUtils::NewObject(L, "MCItemInstance", ins);
    return 1;
}

static const luaL_Reg items_functions[] = {
    {"findItemByName", l_Items_findItemByName},
    {"findItemByID", l_Items_findItemByID},
    {"getCreativePosition", l_Items_getCreativePosition},
    {"newToolTier", l_Items_newToolTier},
    {"registerItem", l_Items_registerItem},
    {"registerSwordItem", l_Items_registerItemSwordTool},
    {"registerAxeItem", l_Items_registerItemAxeTool},
    {"registerPickaxeItem", l_Items_registerItemPickaxeTool},
    {"registerShovelItem", l_Items_registerItemShovelTool},
    {"registerHoeItem", l_Items_registerItemHoeTool},
    {"registerCreativeItem", l_Items_registerCreativeItem},
    {"getItemInstance", l_Items_getItemInstance},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

/*
=MCItem.StackSize = 64
=MCItem.ID = 1
=MCItem.NameID = ""
=MCItem.DescriptionID = ""
*/
/*
=MCToolTier.MiningLevel = 0
=MCToolTier.Durability = 0
=MCToolTier.MiningEfficiency = 0
=MCToolTier.DamageBonus = 0
=MCToolTier.Enchantability = 0
*/


static int l_gc_MCItemInstance(lua_State* L) {
    LuaObject<ItemInstance> ptr = LuaObjectUtils::CheckObject<ItemInstance>(L, 1, "MCItemInstance");
    Core::dealloc(ptr.get());
    return 0;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterItemsModule(lua_State *L) {
    ClassBuilder<Item>(L, "MCItem")
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
    ClassBuilder<ItemInstance>(L, "MCItemInstance")
        .gc(l_gc_MCItemInstance)
        .build();
    ClassBuilder<Item::Tier>(L, "MCToolTier")
        .property("MiningLevel", &Item::Tier::miningLevel)
        .property("Durability", &Item::Tier::durability)
        .property("MiningEfficiency", &Item::Tier::miningEfficiency)
        .property("DamageBonus", &Item::Tier::damageBonus)
        .property("Enchantability", &Item::Tier::enchantability)
        .build();

    lua_getglobal(L, "Game");
    lua_newtable(L); // Items

    //$Game.Items.ToolTiers
    lua_newtable(L);
    //$@@@Game.Items.ToolTiers.WOOD: MCToolTier
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::WOOD);
    lua_setfield(L, -2, "WOOD");
    //$@@@Game.Items.ToolTiers.STONE: MCToolTier
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::STONE);
    lua_setfield(L, -2, "STONE");
    //$@@@Game.Items.ToolTiers.IRON: MCToolTier
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::IRON);
    lua_setfield(L, -2, "IRON");
    //$@@@Game.Items.ToolTiers.GOLD: MCToolTier
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::GOLD);
    lua_setfield(L, -2, "GOLD");
    //$@@@Game.Items.ToolTiers.DIAMOND: MCToolTier
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::DIAMOND);
    lua_setfield(L, -2, "DIAMOND");
    lua_setfield(L, -2, "ToolTiers");

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