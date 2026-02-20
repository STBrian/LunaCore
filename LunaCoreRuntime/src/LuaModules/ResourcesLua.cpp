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

// ----------------------------------------------------------------------------

bool Core::Module::RegisterResourcesModule(lua_State *L) {
    lua_getglobal(L, "Game");
    lua_newtable(L); // Resources

    lua_pushcfunction(L, l_Resources_reloadLocale);
    lua_setfield(L, -2, "reloadLocale");

    lua_setfield(L, -2, "Resources");
    lua_pop(L, 1);
    return true;
}