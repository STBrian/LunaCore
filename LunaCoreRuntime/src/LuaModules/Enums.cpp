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

/* @doc:v2
@module Enums
@desc Enumeration groups module
*/

/* @doc:v2
@class BaseEnumItem
@field Name string
@field Value integer
*/

/* @doc:v2
@class EnumItem: BaseEnumItem
*/

// ----------------------------------------------------------------------------

#define SETNEWENUM(group, name, value) do { \
    LuaObjectUtils::NewObject(L, group.enumType, group.addItem(name, (int)(value))); \
    lua_setfield(L, -2, name); \
    } while (0)

/* @doc:v2
@class EntityState: EnumItem
@global EntityState
*/

/* @doc:v2
@class WeatherType: EnumItem
@global WeatherType
*/

bool Core::Module::RegisterEnumsModule(lua_State *L) {
    ClassBuilder<EnumItem>(L, "EnumItem")
        .property("Name", &EnumItem::name, true)
        .property("Value", &EnumItem::value, true)
        .property("EnumType", &EnumItem::enumType, true)
        .build();
    ClassBuilder<EnumItem>(L, "EntityState").setBase("EnumItem").build();
    ClassBuilder<EnumItem>(L, "WeatherType").setBase("EnumItem").build();

    lua_newtable(L);

    using EntityState = MC3DSPluginFramework::SynchedEntityData::EntitySharedFlag;
    /* @doc:v2
    @table Enums.EntityState
    */
    lua_newtable(L);
    EnumGroup& entityState = EnumGroups::getInstance().addGroup("EntityState");
    /* @doc:v2
    @table Enums.EntityState.Burning: EntityState
    */
    SETNEWENUM(entityState, "Burning", EntityState::Burning);
    /* @doc:v2
    @table Enums.EntityState.Sneaking: EntityState
    */
    SETNEWENUM(entityState, "Sneaking", EntityState::Sneak);
    /* @doc:v2
    @table Enums.EntityState.Sprinting: EntityState
    */
    SETNEWENUM(entityState, "Sprinting", EntityState::Sprint);
    /* @doc:v2
    @table Enums.EntityState.Eating: EntityState
    */
    SETNEWENUM(entityState, "Eating", EntityState::Eating);
    /* @doc:v2
    @table Enums.EntityState.Invisible: EntityState
    */
    SETNEWENUM(entityState, "Invisible", EntityState::InVisible);
    /* @doc:v2
    @table Enums.EntityState.Tempted: EntityState
    */
    SETNEWENUM(entityState, "Tempted", EntityState::IsTempted);
    /* @doc:v2
    @table Enums.EntityState.InLove: EntityState
    */
    SETNEWENUM(entityState, "InLove", EntityState::Love);
    /* @doc:v2
    @table Enums.EntityState.HasSaddle: EntityState
    */
    SETNEWENUM(entityState, "HasSaddle", EntityState::Saddled);
    /* @doc:v2
    @table Enums.EntityState.IsAdult: EntityState
    */
    SETNEWENUM(entityState, "IsAdult", EntityState::Baby);
    /* @doc:v2
    @table Enums.EntityState.Named: EntityState
    */
    SETNEWENUM(entityState, "Named", EntityState::Named);
    /* @doc:v2
    @table Enums.EntityState.Tamed: EntityState
    */
    SETNEWENUM(entityState, "Tamed", EntityState::Tamed);
    /* @doc:v2
    @table Enums.EntityState.Leaded: EntityState
    */
    SETNEWENUM(entityState, "Leaded", 28);
    /* @doc:v2
    @table Enums.EntityState.Sheared: EntityState
    */
    SETNEWENUM(entityState, "Sheared", EntityState::Sheared);
    /* @doc:v2
    @table Enums.EntityState.ElytraFly: EntityState
    */
    SETNEWENUM(entityState, "ElytraFly", EntityState::Gliding);
    /* @doc:v2
    @table Enums.EntityState.Gliding: EntityState
    */
    SETNEWENUM(entityState, "Gliding", EntityState::Gliding);
    /* @doc:v2
    @table Enums.EntityState.Chested: EntityState
    */
    SETNEWENUM(entityState, "Chested", EntityState::Chested);
    /* @doc:v2
    @table Enums.EntityState.Sitting: EntityState
    */
    SETNEWENUM(entityState, "Sitting", EntityState::Sitting);
    /* @doc:v2
    @table Enums.EntityState.Angry: EntityState
    */
    SETNEWENUM(entityState, "Angry", EntityState::Angry);
    /* @doc:v2
    @table Enums.EntityState.ShowBottom: EntityState
    */
    SETNEWENUM(entityState, "ShowBottom", EntityState::ShowBottom);
    lua_setfield(L, -2, "EntityState");

    /* @doc:v2
    @table Enums.WeatherType
    */
    lua_newtable(L);
    EnumGroup& weatherType = EnumGroups::getInstance().addGroup("WeatherType");
    /* @doc:v2
    @table Enums.WeatherType.Clear: WeatherType
    */
    SETNEWENUM(weatherType, "Clear", MC3DSPluginFramework::Weathers::Clear);
    /* @doc:v2
    @table Enums.WeatherType.Rain: WeatherType
    */
    SETNEWENUM(weatherType, "Rain", MC3DSPluginFramework::Weathers::Rain);
    /* @doc:v2
    @table Enums.WeatherType.Thunder: WeatherType
    */
    SETNEWENUM(weatherType, "Thunder", MC3DSPluginFramework::Weathers::Thunder);
    /* @doc:v2
    @table Enums.WeatherType.RainThunder: WeatherType
    */
    SETNEWENUM(weatherType, "RainThunder", MC3DSPluginFramework::Weathers::Rain|MC3DSPluginFramework::Weathers::Thunder);
    lua_setfield(L, -2, "WeatherType");

    lua_setglobal(L, "Enums");
    return true;
}