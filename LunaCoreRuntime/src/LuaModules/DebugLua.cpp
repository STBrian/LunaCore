#include "LuaModules.hpp"

#include "Core/Debug.hpp"

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/LuaModules/LoadModules.cpp

/* @doc:v2
@module Core.Debug
@desc Debugging and logging utilities
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@func Core.Debug.message(msg: string)
@desc Displays a notification on screen
@param msg string
*/
static int l_Debug_message(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);

    CTRPluginFramework::OSD::Notify(msg);
    return 0;
}

/* @doc:v2
@func Core.Debug.log(msg: string, showOnScreen: boolean?)
@desc Appends the message to log file. Optionally shows the message on screen
@param msg string
@param showOnScreen boolean?
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

/* @doc:v2
@func Core.Debug.logerror(msg: string)
@desc Appends the error message to log file and shows it on screen
@param msg string
*/
static int l_Debug_logerror(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    Core::Debug::LogError(msg);
    return 0;
}

/* @doc:v2
@func Core.Debug.error(msg: string)
@desc Show error on screen
@param msg string
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