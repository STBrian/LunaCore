#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>
#include "MC3DSPluginFramework.hpp"

#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "string_hash.hpp"

#include "Enums.hpp"
#include "Helpers/LuaObject.hpp"

#include "game/Minecraft.hpp"
#include "Helpers/LuaCustomTable.hpp"

#include "MC3DSPluginFramework.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;

enum world_offsets : u32 {
    cloudsHeight = 0x3C5398
};

// ----------------------------------------------------------------------------

//$Game.World

// ----------------------------------------------------------------------------

/*
=Game.World.Loaded = false
=Game.World.Raining = false
=Game.World.Thunderstorm = false
=Game.World.CloudsHeight = 0.0
#---@type WeatherType
#Game.World.Weather = {}
*/

static int l_World_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    auto ply = MC3DSPluginFramework::Player::GetInstance();
    MC3DSPluginFramework::BlockSource* blockSrc = nullptr;
    MC3DSPluginFramework::Level* level = nullptr;
    if (ply) {
        blockSrc = ply->GetBlockSource();
        level = ply->GetLevel();
    }

    switch (key) {
        case hash("Loaded"): // Read-only
            lua_pushboolean(L, GameState.WorldLoaded.load());
            break;
        case hash("Raining"):
            lua_pushboolean(L, Minecraft::IsRaining());
            break;
        case hash("Thunderstorm"):
            lua_pushboolean(L, Minecraft::IsThundering());
            break;
        case hash("CloudsHeight"): {
            float value;
            CTRPF::Process::ReadFloat(world_offsets::cloudsHeight, value);
            lua_pushnumber(L, value);
            break;
        }
        case hash("Weather"): {
            MC3DSPluginFramework::WeatherState* weather = level->weather();
            Core::EnumGroup& group = EnumGroups::getInstance().getGroup("WeatherType");
            if (weather->get() & MC3DSPluginFramework::Weathers::Thunder)
                LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Thunder"));
            else if (weather->get() & MC3DSPluginFramework::Weathers::Rain)
                LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Rain"));
            else
                LuaObjectUtils::NewObject(L, group.enumType, group.getItemByName("Clear"));
            break;
        }
        default:
            valid_key = false;
            break;
    }

    return valid_key;
}

static int l_World_newindex(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return LUACT_INVALID_KEY;

    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    auto ply = MC3DSPluginFramework::Player::GetInstance();
    MC3DSPluginFramework::BlockSource* blockSrc = nullptr;
    MC3DSPluginFramework::Level* level = nullptr;
    if (ply) {
        blockSrc = ply->GetBlockSource();
        level = ply->GetLevel();
    }

    switch (key) {
        case hash("Raining"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetRain(lua_toboolean(L, 3));
            break;
        case hash("Thunderstorm"):
            LUACT_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetThunder(lua_toboolean(L, 3));
            break;
        case hash("CloudsHeight"): {
            LUACT_CHECKTYPE(L, LUA_TNUMBER, 3);
            CTRPF::Process::WriteFloat(world_offsets::cloudsHeight, luaL_checknumber(L, 3));
            break;
        }
        case hash("Weather"): {
            Core::EnumItem* val = EnumItemUtils::LuaToEnumItemOrNull(L, 3, "WeatherType", true);
            if (!val) return -1;
            if (level) level->weather()->set(val->value);
            break;
        }
        default:
            valid_key = false;
            break;
    }

    if (valid_key) return 0;
    else return LUACT_INVALID_KEY;
}

static int l_World_message(lua_State *L) {
    const char* msg = luaL_checkstring(L, 1);
    if (MC3DSPluginFramework::SystemMessagesScreen::RunningInstance) {
        MC3DSPluginFramework::SystemMessagesScreen::RunningInstance->pushMessage(msg, false);
    }
    return 0;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterWorldModule(lua_State *L) {
    LuaCustomTable::RegisterNewCustomTable(L, "World", l_World_index, l_World_newindex);
    LuaCustomTable::GetIndexTable(L, "World");

    //$@@@Game.World.OnWorldJoin: EventClass
    Core::Event::NewEvent(L, "OnWorldJoin");

    //$@@@Game.World.OnWorldLeave: EventClass
    Core::Event::NewEvent(L, "OnWorldLeave");

    lua_pushcfunction(L, l_World_message);
    lua_setfield(L, -2, "message");

    lua_pop(L, 1); // Pop index table
    lua_getglobal(L, "Game");
    LuaCustomTable::SetNewCustomTable(L, -1, "World");
    lua_pop(L, 1); // Pop Game
    return true;
}