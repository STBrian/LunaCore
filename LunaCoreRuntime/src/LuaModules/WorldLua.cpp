#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "string_hash.hpp"

#include "game/Minecraft.hpp"

#include "lua_utils.hpp"

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

    if (valid_key)
        return 1;
    else
        return 0;
}

static int l_World_newindex(lua_State *L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "attempt to set field '?' of \"World\"");

    // "Safe" c++ error handler
    LUAUTILS_INIT_TYPEERROR_HANDLER();
    LUAUTILS_SET_TYPEERROR_MESSAGE("unable to assign to a \"%s\" field a \"%s\" value");

    {
    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("Raining"):
            LUAUTILS_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetRain(lua_toboolean(L, 3));
            break;
        case hash("Thunderstorm"):
            LUAUTILS_CHECKTYPE(L, LUA_TBOOLEAN, 3);
            Minecraft::SetThunder(lua_toboolean(L, 3));
            break;
        case hash("CloudsHeight"): {
            LUAUTILS_CHECKTYPE(L, LUA_TNUMBER, 3);
            CTRPF::Process::WriteFloat(world_offsets::cloudsHeight, luaL_checknumber(L, 3));
            break;
        }
        default:
            valid_key = false;
            break;
    }

    if (valid_key)
        return 0;
    else
        return luaL_error(L, "'%s' is not a valid member of World or is a read-only value", lua_tostring(L, 2));
    }

    LUAUTILS_SET_ERROR_HANDLER(L);
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterWorldModule(lua_State *L)
{
    luaL_newmetatable(L, "WorldMetatable");
    lua_pushcfunction(L, l_World_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_World_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);

    lua_getglobal(L, "Game");
    lua_newtable(L); // World

    //$@@@Game.World.OnWorldJoin: EventClass
    Core::Event::NewEvent(L, "OnWorldJoin");

    //$@@@Game.World.OnWorldLeave: EventClass
    Core::Event::NewEvent(L, "OnWorldLeave");

    luaC_setmetatable(L, "WorldMetatable");
    lua_setfield(L, -2, "World");
    lua_pop(L, 1);
    return true;
}