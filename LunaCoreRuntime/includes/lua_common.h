#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <cmath>
#include <cstring>

#define STRINGIFY(x) #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)

#define luaC_register_global(L, reg, globalName) ({\
        lua_newtable(L);\
        luaL_register(L, NULL, reg);\
        lua_setglobal(L, (globalName));\
    })

#define luaC_register_field(L, reg, fieldName) ({\
        lua_newtable(L);\
        luaL_register(L, NULL, reg);\
        lua_setfield(L, -2, (fieldName));\
    })

#define luaC_setfield_integer(L, num, field) ({\
        lua_pushnumber(L, (num));\
        lua_setfield(L, -2, (field));\
    })

#define luaC_setmetatable(L, name) ({\
        luaL_getmetatable(L, (name));\
        lua_setmetatable(L, -2);\
    })

static inline int luaC_invalid_newindex(lua_State *L) {
    return luaL_error(L, "Attempt to modify read-only table");
}

static inline void* luaC_funccheckudata(lua_State *L, int narg, const char* uname) {
    if (lua_type(L, narg) == LUA_TUSERDATA) {
        if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
            luaL_typerror(L, narg, uname);
        else {
            if (lua_type(L, -1) != LUA_TSTRING) {
                lua_pop(L, 1);
                luaL_typerror(L, narg, uname);
            }
            if (std::strcmp(uname, lua_tostring(L, -1)) != 0) {
                lua_pushfstring(L, "bad argument #%d (%s expected, got %s)", narg, uname, lua_tostring(L, -1));
                lua_remove(L, -2);
                lua_error(L);
            } else {
                lua_pop(L, 1);
                return lua_touserdata(L, narg);
            }
        }
    } else
        luaL_typerror(L, narg, uname);
    return nullptr;
}

// The msg must be a formatted string with two %s, the first is the detected type and the second is uname
static inline void* luaC_checkudata(lua_State *L, int narg, const char* uname, const char* msg) {
    if (lua_type(L, narg) == LUA_TUSERDATA) {
        if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
            luaL_error(L, msg, luaL_typename(L, narg), uname);
        else {
            if (lua_type(L, -1) != LUA_TSTRING) {
                lua_pop(L, 1);
                luaL_typerror(L, narg, uname);
            }
            if (std::strcmp(uname, lua_tostring(L, -1)) != 0) {
                lua_pushfstring(L, msg, lua_tostring(L, -1), uname);
                lua_remove(L, -2);
                lua_error(L);
            } else {
                lua_pop(L, 1);
                return lua_touserdata(L, narg);
            }
        }
    } else
        luaL_error(L, msg, luaL_typename(L, narg), uname);
    return nullptr;
}

static inline void luaC_indexchecktype(lua_State *L, int narg, int type) {
    if (lua_type(L, narg) != type)
        luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, narg), lua_typename(L, type));
}

static inline lua_Number luaC_indexchecknumber(lua_State *L, int narg) {
    if (lua_type(L, narg) != LUA_TNUMBER)
        luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, narg), lua_typename(L, LUA_TNUMBER));
    else
        return lua_tonumber(L, narg);
    return 0;
}

#ifdef __cplusplus
}
#endif