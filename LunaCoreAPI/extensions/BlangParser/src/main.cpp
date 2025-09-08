#include "types.h"
#include "SharedFunctions.hpp"

#define LCEX_ENTRY __attribute__((used, section(".entry")))

Core::SharedFunctions* g_pfuncs = NULL;

inline void lua_newtable(lua_State* L) {
    g_pfuncs->lua->createtable(L, 0, 0);
}

inline void lua_pushcfunction(lua_State* L, lua_CFunction f) {
    g_pfuncs->lua->pushcclosure(L, f, 0);
}

inline void lua_pushstring(lua_State* L, const char* s) {
    g_pfuncs->lua->pushstring(L, s);
}

inline void lua_pushboolean(lua_State* L, int b) {
    g_pfuncs->lua->pushboolean(L, b);
}

inline void lua_getfield(lua_State* L, int idx, const char* k) {
    g_pfuncs->lua->getfield(L, idx, k);
}

inline void lua_setfield(lua_State* L, int idx, const char* k) {
    g_pfuncs->lua->setfield(L, idx, k);
}

inline void lua_getglobal(lua_State* L, const char* s) {
    g_pfuncs->lua->getfield(L, LUA_GLOBALSINDEX, s);
}

inline int lua_pcall(lua_State* L, int a, int b, int c) {
    return g_pfuncs->lua->pcall(L, a, b, c);
}

inline void lua_settop(lua_State* L, int n) {
    g_pfuncs->lua->settop(L, n);
}

inline void lua_pop(lua_State* L, int n) {
    g_pfuncs->lua->settop(L, -(n)-1);
}

inline void lua_remove(lua_State* L, int n) {
    g_pfuncs->lua->remove(L, n);
}

inline lua_Number luaL_checknumber(lua_State* L, int narg) {
    return g_pfuncs->luaL->checknumber(L, narg);
}

inline const char *lua_tolstring(lua_State* L, int narg, size_t* len) {
    return g_pfuncs->lua->tolstring(L, narg, len);
}

inline const char *lua_tostring(lua_State* L, int narg) {
    return g_pfuncs->lua->tolstring(L, narg, NULL);
}

inline const char* luaL_checkstring(lua_State* L, int narg) {
    return g_pfuncs->luaL->checklstring(L, narg, NULL);
}

void Core_Filesystem_open(lua_State* L, const char* filename, const char* mode) {
    lua_getglobal(L, "Core");
    lua_getfield(L, -1, "Filesystem");
    lua_remove(L, -2);
    lua_getfield(L, -1, "open");
    lua_remove(L, -2);

    lua_pushstring(L, filename);
    lua_pushstring(L, mode);

    lua_pcall(L, 2, 0, 0);
}

int l_blangParser_open(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    g_pfuncs->debug->LogMessage("Hi from LunaCoreExtension", true);
    return 0;
}

const static luaL_Reg blangParser_functions[] = {
    {"open", l_blangParser_open},
    {NULL, NULL}
};

LCEX_ENTRY int main(lua_State* L, Core::SharedFunctions* pfuncs) {
    g_pfuncs = pfuncs;
    lua_newtable(L);
    pfuncs->luaL->lregister(L, NULL, blangParser_functions);
    return 1;
}