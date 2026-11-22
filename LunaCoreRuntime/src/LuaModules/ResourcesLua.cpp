#include "LuaModules.hpp"

#include "game/lang.hpp"

// ----------------------------------------------------------------------------

/* @doc:v2
@module Game.Resources
@desc Localization and game resource assets module
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@func Game.Resources.reloadLocale()
@desc Forces the game to reload the current locale
*/
static int l_Resources_reloadLocale(lua_State* L) {
    Minecraft::Lang::reloadLang();
    return 0;
}

/* @doc:v2
@func Game.Resources.getLocale(): string?
@desc Returns the current seleted locale
@return string?
*/
static int l_Resources_getLocale(lua_State* L) {
    if (Minecraft::Lang::current)
        lua_pushstring(L, Minecraft::Lang::current->name);
    else
        lua_pushnil(L);
    return 1;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterResourcesModule(lua_State *L) {
    lua_getglobal(L, "Game");
    lua_newtable(L); // Resources

    lua_pushcfunction(L, l_Resources_reloadLocale);
    lua_setfield(L, -2, "reloadLocale");
    lua_pushcfunction(L, l_Resources_getLocale);
    lua_setfield(L, -2, "getLocale");

    lua_setfield(L, -2, "Resources");
    lua_pop(L, 1);
    return true;
}