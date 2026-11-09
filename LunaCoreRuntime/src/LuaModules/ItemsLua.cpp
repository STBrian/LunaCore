#include "LuaModules.hpp"

#include <cstring>
#include "string_hash.hpp"

#include "Core/Event.hpp"
#include "Core/Utils/ItemUtils.hpp"

#include "game/Minecraft.hpp"
#include "game/world/item/ItemInstance.hpp"
#include "game/world/item/CreativeMenu.hpp"

#include "Helpers/Allocation.hpp"
#include "Helpers/LuaObject.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;
using Item = Minecraft::Item;
using ItemInstance = Minecraft::ItemInstance;

// ----------------------------------------------------------------------------

/* @doc:v2
@module Game.Items
@desc Item management and registration module
*/

/* @doc:v2
@class MCItem
@desc Represents a registered Minecraft item definition
@field StackSize integer
@field ID integer
@field NameID string
@field DescriptionID string
*/

/* @doc:v2
@class MCItemInstance
@desc Represents an instance of an item in game
*/

/* @doc:v2
@class MCToolTier
@desc Tool tier definition for mining and durability
@field MiningLevel integer
@field Durability integer
@field MiningEfficiency number
@field DamageBonus number
@field Enchantability integer
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@func Game.Items.findItemByName(name: string): MCItem?
@desc Find an item using its ID
@param name string
@return MCItem?
*/
static int l_Items_findItemByName(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Item *itemData = Core::Items::SearchItemByName(name);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", itemData);
    return 1;
}

/* @doc:v2
@func Game.Items.findItemByID(itemID: integer): MCItem?
@desc Find and item using its name
@param itemID integer
@return MCItem?
*/
static int l_Items_findItemByID(lua_State *L) {
    u16 itemID = luaL_checknumber(L, 1);
    Item *itemData = Core::Items::SearchItemByID(itemID);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", itemData);
    return 1;
}

/* @doc:v2
@func Game.Items.getCreativePosition(itemID: integer, groupID: integer): number
@desc Get the item position in creative using the id
@param itemID integer
@param groupID integer
@return number
*/
static int l_Items_getCreativePosition(lua_State *L) {
    u16 itemID = luaL_checknumber(L, 1);
    u16 groupID = luaL_checknumber(L, 2);
    u16 position = Core::Items::GetCreativeItemPositionOfGroup(itemID, groupID);
    lua_pushnumber(L, position);
    return 1;
}

/* @doc:v2
@func Game.Items.newToolTier(): MCToolTier
@desc Returns a new empty ToolTier
@return MCToolTier
*/
static int l_Items_newToolTier(lua_State *L) {
    LuaObjectUtils::NewObject(L, "MCToolTier", alloc<Item::Tier>());
    return 1;
}

/* @doc:v2
@func Game.Items.registerItem(namespace: string, itemName: string, itemId: integer): MCItem?
@desc Creates a new item and stores it in the game's items table. Returns the address to the item
@param namespace string
@param itemName string
@param itemId integer
@return MCItem?
*/
static int l_Items_registerItem(lua_State *L) {
    if (lua_gettop(L) > 2) {
        const char* itemnamespace = luaL_checkstring(L, 1);
        const char* itemName = luaL_checkstring(L, 2);
        u16 itemId = luaL_checknumber(L, 3);
        Item* regItem = Minecraft::registerRegularItem(itemName, itemId, itemnamespace);
        LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    } else {
        const char* itemName = luaL_checkstring(L, 1);
        u16 itemId = luaL_checknumber(L, 2);
        Item* regItem = Minecraft::registerItem<Item>(itemName, itemId);
        LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    }
    return 1;
}

/* @doc:v2
@func Game.Items.registerSwordItem(itemName: string, itemId: integer, tier: MCToolTier): MCItem?
@desc Creates a new sword tool item and registers it in the game. Returns the address to the item
@param itemName string
@param itemId integer
@param tier MCToolTier
@return MCItem?
*/
static int l_Items_registerItemSwordTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemSwordTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/* @doc:v2
@func Game.Items.registerHoeItem(itemName: string, itemId: integer, tier: MCToolTier): MCItem?
@desc Creates a new hoe tool item and registers it in the game. Returns the address to the item
@param itemName string
@param itemId integer
@param tier MCToolTier
@return MCItem?
*/
static int l_Items_registerItemHoeTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemHoeTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/* @doc:v2
@func Game.Items.registerAxeItem(itemName: string, itemId: integer, tier: MCToolTier): MCItem?
@desc Creates a new axe tool item and registers it in the game. Returns the address to the item
@param itemName string
@param itemId integer
@param tier MCToolTier
@return MCItem?
*/
static int l_Items_registerItemAxeTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemAxeTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/* @doc:v2
@func Game.Items.registerPickaxeItem(itemName: string, itemId: integer, tier: MCToolTier): MCItem?
@desc Creates a new pickaxe tool item and registers it in the game. Returns the address to the item
@param itemName string
@param itemId integer
@param tier MCToolTier
@return MCItem?
*/
static int l_Items_registerItemPickaxeTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemPickaxeTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/* @doc:v2
@func Game.Items.registerShovelItem(itemName: string, itemId: integer, tier: MCToolTier): MCItem?
@desc Creates a new shovel tool item and registers it in the game. Returns the address to the item
@param itemName string
@param itemId integer
@param tier MCToolTier
@return MCItem?
*/
static int l_Items_registerItemShovelTool(lua_State *L) {
    const char* itemName = luaL_checkstring(L, 1);
    u16 itemId = luaL_checknumber(L, 2);
    Item::Tier* tier = LuaObjectUtils::CheckObject<Item::Tier>(L, 3, "MCToolTier").get();
    Item* regItem = Minecraft::registerItemShovelTool(itemName, itemId, tier);
    LuaObjectUtils::NewObjectCheck(L, "MCItem", regItem);
    return 1;
}

/* @doc:v2
@method MCItem:setTexture(textureName: string, textureIndex: integer)
@desc Takes a registered item with Game.Items.registerItem, and sets its texture
@param textureName string
@param textureIndex integer
*/
static int l_Items_registerItemTexture(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "MCItem").get();
    const char* textureName = luaL_checkstring(L, 2);
    u16 textureIndex = luaL_checkinteger(L, 3);
    item->setTexture(hash(textureName), textureIndex);
    return 0;
}

/* @doc:v2
@func Game.Items.registerCreativeItem(item: MCItem, groupId: integer, position: integer)
@desc Takes a registered item with Game.Items.registerItem, and registers it in creative menu
@param item MCItem
@param groupId integer
@param position integer
*/
static int l_Items_registerCreativeItem(lua_State *L) {
    Item* item = LuaObjectUtils::CheckObject<Item>(L, 1, "MCItem").get();
    u16 groupId = luaL_checkinteger(L, 2);
    s16 position = luaL_checkinteger(L, 3);
    Minecraft::Creative::addCreativeItem(item, groupId, position);
    return 0;
}

/* @doc:v2
@func Game.Items.getItemInstance(item: MCItem, count: integer, data: integer): MCItemInstance
@desc Returns a MCItemInstance
@param item MCItem
@param count integer
@param data integer
@return MCItemInstance
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

/* @doc:v2
@const MCItem.StackSize = 64
@const MCItem.ID = 1
@const MCItem.NameID = ""
@const MCItem.DescriptionID = ""

@const MCToolTier.MiningLevel = 0
@const MCToolTier.Durability = 0
@const MCToolTier.MiningEfficiency = 0
@const MCToolTier.DamageBonus = 0
@const MCToolTier.Enchantability = 0
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

    /* @doc:v2
    @table Game.Items.ToolTiers
    @desc Built-in tool tier constants
    */
    lua_newtable(L);
    /* @doc:v2
    @class WOOD: MCToolTier
    @target Game.Items.ToolTiers.WOOD
    */
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::WOOD);
    lua_setfield(L, -2, "WOOD");
    /* @doc:v2
    @class STONE: MCToolTier
    @target Game.Items.ToolTiers.STONE
    */
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::STONE);
    lua_setfield(L, -2, "STONE");
    /* @doc:v2
    @class IRON: MCToolTier
    @target Game.Items.ToolTiers.IRON
    */
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::IRON);
    lua_setfield(L, -2, "IRON");
    /* @doc:v2
    @class GOLD: MCToolTier
    @target Game.Items.ToolTiers.GOLD
    */
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::GOLD);
    lua_setfield(L, -2, "GOLD");
    /* @doc:v2
    @class DIAMOND: MCToolTier
    @target Game.Items.ToolTiers.DIAMOND
    */
    LuaObjectUtils::NewObject(L, "MCToolTier", Item::Tier::DIAMOND);
    lua_setfield(L, -2, "DIAMOND");
    lua_setfield(L, -2, "ToolTiers");

    /* @doc:v2
    @class OnRegisterItems: EventClass
    @target Game.Items.OnRegisterItems
    */
    Core::Event::NewEvent(L, "OnRegisterItems");

    /* @doc:v2
    @class OnRegisterItemsTextures: EventClass
    @target Game.Items.OnRegisterItemsTextures
    */
    Core::Event::NewEvent(L, "OnRegisterItemsTextures");

    /* @doc:v2
    @class OnRegisterCreativeItems: EventClass
    @target Game.Items.OnRegisterCreativeItems
    */
    Core::Event::NewEvent(L, "OnRegisterCreativeItems");

    luaL_register(L, NULL, items_functions);

    lua_setfield(L, -2, "Items");
    lua_pop(L, 1);
    return true;
}