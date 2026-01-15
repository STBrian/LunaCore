#include "LuaModules.hpp"

#include "Core/Debug.hpp"

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/Modules.cpp
//$Core.Debug

// ----------------------------------------------------------------------------

/*
- Displays a notification on screen
## msg: string
### Core.Debug.message
*/
static int l_Debug_message(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);

    CTRPluginFramework::OSD::Notify(msg);
    return 0;
}

/*
- Appends the message to log file. Optionally shows the message on screen
## msg: string
## showOnScreen: boolean?
### Core.Debug.log
*/
static int l_Debug_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    bool showOnScreen = false;
    if (lua_type(L, 2) == LUA_TBOOLEAN)
        showOnScreen = lua_toboolean(L, 2);
    Core::Debug::LogInfo(msg);
    if (showOnScreen)
        CTRPluginFramework::OSD::Notify(msg);
    return 0;
}

/*
- Appends the error message to log file and shows it on screen
## msg: string
### Core.Debug.logerror
*/
static int l_Debug_logerror(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    Core::Debug::LogError(msg);
    return 0;
}

/*
- Show error on screen
## msg: string
### Core.Debug.error
*/

static const luaL_Reg debug_functions[] =
{
    {"message", l_Debug_message},
    {"log", l_Debug_log},
    {"logerror", l_Debug_logerror},
    {"error", l_Debug_logerror},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterDebugModule(lua_State *L)
{
    lua_getglobal(L, "Core");
    luaC_register_field(L, debug_functions, "Debug");
    lua_pop(L, 1);
    return true;
}