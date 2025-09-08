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

inline void* lua_newuserdata(lua_State* L, size_t s) {
    return g_pfuncs->lua->newuserdata(L, s);
}

typedef struct {
    BlangIndex* indexes;
    BlangIndex** newIndexes;
    char* data;
    char** newData;
    int added;
} BlangHandler;

typedef struct {
    u32 hash;
    u32 position;
} BlangIndex;

int l_blangParser_open(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    FilesystemFile* file = g_pfuncs->fslib->fopen(filename, "r");
    if (file == NULL)
        return 0;
    u32 idxlength;
    g_pfuncs->fslib->fread(&idxlength, sizeof(u32), 1, file);
    BlangIndex* indexes = (BlangIndex*)g_pfuncs->cstdlib->malloc(sizeof(BlangIndex) * idxlength);
    if (indexes == NULL) {
        g_pfuncs->fslib->fclose(file);
        return 0;
    }
    if (g_pfuncs->fslib->fread(indexes, sizeof(BlangIndex), idxlength, file) < idxlength * sizeof(BlangIndex)) {
        g_pfuncs->fslib->fclose(file);
        g_pfuncs->cstdlib->free(indexes);
        return 0;
    }
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