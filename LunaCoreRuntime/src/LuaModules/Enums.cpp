#include "LuaModules.hpp"

#include <MC3DSPluginFramework.hpp>

#include "Enums.hpp"

#include "Helpers/LuaObject.hpp"
#include "Helpers/Allocation.hpp"

using namespace Core;

EnumGroups& EnumGroups::getInstance() {
    static EnumGroups ins;
    return ins;
}

// ----------------------------------------------------------------------------

//$Enums

/*
#---@class BaseEnumItem
#---@field Name string
#---@field Value integer
#local BaseEnumItem = {}
@EnumItem : BaseEnumItem
*/

// ----------------------------------------------------------------------------

#define SETNEWENUM(group, name, value) do { \
    LuaObjectUtils::NewObject(L, group.enumType, group.addItem(name, value)); \
    lua_setfield(L, -2, name); \
    } while (0)

//$@@@EntityState : EnumItem
//$@@@WeatherType : EnumItem

bool Core::Module::RegisterEnumsModule(lua_State *L) {
    ClassBuilder<EnumItem>(L, "EnumItem")
        .property("Name", &EnumItem::name, true)
        .property("Value", &EnumItem::value, true)
        .property("EnumType", &EnumItem::enumType, true)
        .build();
    ClassBuilder<EnumItem>(L, "EntityState").setBase("EnumItem").build();
    ClassBuilder<EnumItem>(L, "WeatherType").setBase("EnumItem").build();

    lua_newtable(L);

    //$Enums.EntityState
    lua_newtable(L);
    EnumGroup& entityState = EnumGroups::getInstance().addGroup("EntityState");
    //#$Enums.EntityState.Burning : ---@type EntityState
    SETNEWENUM(entityState, "Burning", 0);
    //#$Enums.EntityState.Sneaking : ---@type EntityState
    SETNEWENUM(entityState, "Sneaking", 1);
    //#$Enums.EntityState.Sprinting : ---@type EntityState
    SETNEWENUM(entityState, "Sprinting", 3);
    //#$Enums.EntityState.Eating : ---@type EntityState
    SETNEWENUM(entityState, "Eating", 4);
    //#$Enums.EntityState.Invisible : ---@type EntityState
    SETNEWENUM(entityState, "Invisible", 5);
    //#$Enums.EntityState.Tempted : ---@type EntityState
    SETNEWENUM(entityState, "Tempted", 6);
    //#$Enums.EntityState.InLove : ---@type EntityState
    SETNEWENUM(entityState, "InLove", 7);
    //#$Enums.EntityState.HasSaddle : ---@type EntityState
    SETNEWENUM(entityState, "HasSaddle", 8);
    //#$Enums.EntityState.IsAdult : ---@type EntityState
    SETNEWENUM(entityState, "IsAdult", 11);
    //#$Enums.EntityState.Named : ---@type EntityState
    SETNEWENUM(entityState, "Named", 14);
    //#$Enums.EntityState.Tamed : ---@type EntityState
    SETNEWENUM(entityState, "Tamed", 27);
    //#$Enums.EntityState.Leaded : ---@type EntityState
    SETNEWENUM(entityState, "Leaded", 28);
    //#$Enums.EntityState.Sheared : ---@type EntityState
    SETNEWENUM(entityState, "Sheared", 29);
    //#$Enums.EntityState.ElytraFly : ---@type EntityState
    SETNEWENUM(entityState, "ElytraFly", 30);
    lua_setfield(L, -2, "EntityState");

    //$Enums.WeatherType
    lua_newtable(L);
    EnumGroup& weatherType = EnumGroups::getInstance().addGroup("WeatherType");
    //#$Enums.WeatherType.Clear : ---@type WeatherType
    SETNEWENUM(weatherType, "Clear", MC3DSPluginFramework::Weathers::Clear);
    //#$Enums.WeatherType.Rain : ---@type WeatherType
    SETNEWENUM(weatherType, "Rain", MC3DSPluginFramework::Weathers::Rain);
    //#$Enums.WeatherType.Thunder : ---@type WeatherType
    SETNEWENUM(weatherType, "Thunder", MC3DSPluginFramework::Weathers::Thunder);
    //#$Enums.WeatherType.RainThunder : ---@type WeatherType
    SETNEWENUM(weatherType, "RainThunder", MC3DSPluginFramework::Weathers::Rain|MC3DSPluginFramework::Weathers::Thunder);
    lua_setfield(L, -2, "WeatherType");

    lua_setglobal(L, "Enums");
    return true;
}