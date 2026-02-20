#include "LuaModules.hpp"

#include <time.h>
#include <3ds.h>

// ----------------------------------------------------------------------------

//$Core.System

// ----------------------------------------------------------------------------

/*
- Returns UNIX time
## return: number
### Core.System.getTime
*/
static int l_System_getTime(lua_State *L)
{
    lua_pushnumber(L, time(NULL));
    return 1;
}

/*
- Returns current state of the 3D Slider
## return: number
### Core.System.get3DSliderState
*/
static int l_System_get3DSliderState(lua_State* L) {
    float slider = osGet3DSliderState();
    lua_pushnumber(L, slider);
    return 1;
}

static const luaL_Reg system_functions[] =
{
    {"getTime", l_System_getTime},
    {"get3DSliderState", l_System_get3DSliderState},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterSystemModule(lua_State *L)
{
    lua_getglobal(L, "Core");
    luaC_register_field(L, system_functions, "System");
    lua_pop(L, 1);
    return true;
}