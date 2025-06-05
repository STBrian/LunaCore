#include "World.hpp"

#include <CTRPluginFramework.hpp>

#include "Game/GameState.hpp"
#include "string_hash.hpp"

#include "Minecraft.hpp"

namespace CTRPF = CTRPluginFramework;

extern GameState_s GameState;

enum world_offsets : u32
{
    cloudsHeight = 0x100000 + 0x2C5398
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
        return luaL_error(L, "Attempt to set unknown member of World");

    shash key = hash(lua_tostring(L, 2));
    bool valid_key = true;

    switch (key) {
        case hash("Raining"):
            Minecraft::SetRain(lua_toboolean(L, 3));
            break;
        case hash("Thunderstorm"):
            Minecraft::SetThunder(lua_toboolean(L, 3));
            break;
        case hash("CloudsHeight"): {
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
        return luaL_error(L, "'%s' is not a valid member of World or is a read-only value", key);
}

// ----------------------------------------------------------------------------

bool Core::Game::RegisterWorldModule(lua_State *L)
{
    luaL_newmetatable(L, "WorldMetatable");
    lua_pushcfunction(L, l_World_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_World_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);

    lua_getglobal(L, "Game");
    lua_newtable(L);
    luaC_setmetatable(L, "WorldMetatable");
    lua_setfield(L, -2, "World");
    lua_pop(L, 1);
    return true;
}