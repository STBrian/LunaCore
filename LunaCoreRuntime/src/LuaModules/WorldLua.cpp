#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "string_hash.hpp"

#include "game/Minecraft.hpp"
#include "Helpers/LuaCustomTable.hpp"

namespace CTRPF = CTRPluginFramework;

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
*/
static int l_World_index(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

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
        default:
            valid_key = false;
            break;
    }

    if (valid_key) return 0;
    else return LUACT_INVALID_KEY;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterWorldModule(lua_State *L) {
    LuaCustomTable::RegisterNewCustomTable(L, "World", l_World_index, l_World_newindex);
    LuaCustomTable::GetIndexTable(L, "World");

    //$@@@Game.World.OnWorldJoin: EventClass
    Core::Event::NewEvent(L, "OnWorldJoin");

    //$@@@Game.World.OnWorldLeave: EventClass
    Core::Event::NewEvent(L, "OnWorldLeave");

    lua_pop(L, 1); // Pop index table
    lua_getglobal(L, "Game");
    LuaCustomTable::SetNewCustomTable(L, -1, "World");
    lua_pop(L, 1); // Pop Game
    return true;
}