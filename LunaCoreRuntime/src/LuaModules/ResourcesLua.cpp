#include "LuaModules.hpp"

#include "game/lang.hpp"

// ----------------------------------------------------------------------------

//$Game.Resources

// ----------------------------------------------------------------------------

/*
- Forces the game to reload the current locale
### Game.Resources.reloadLocale
*/
static int l_Resources_reloadLocale(lua_State* L) {
    Minecraft::Lang::reloadLang();
    return 0;
}

/*
- Returns the current seleted locale
## return: string?
### Game.Resources.getLocale
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