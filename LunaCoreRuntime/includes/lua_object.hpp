#pragma once

#include "CoreGlobals.hpp"

#include <unordered_map>
#include "lua_common.h"
#include "types.h"

typedef enum {
    OBJF_TYPE_CHAR,
    OBJF_TYPE_SHORT,
    OBJF_TYPE_INT,
    OBJF_TYPE_FLOAT,
    OBJF_TYPE_DOUBLE,
    OBJF_TYPE_STRING,
    OBJF_TYPE_METHOD,
    OBJF_TYPE_NIL,
} LuaValueType;

typedef enum {
    OBJF_ACCESS_INDEX,
    OBJF_ACCESS_ALL,
} LuaFieldAccess;

typedef struct {
    const char* key;
    LuaValueType type;
    u32 offset;
    LuaFieldAccess access = OBJF_ACCESS_ALL;
} LuaObjectField;

class LuaObject {
    public:
    typedef struct {
        u32 offset;
        LuaValueType type;
        LuaFieldAccess access;
    } ValueMetadata;
    
    static std::unordered_map<std::string, std::unordered_map<std::string, ValueMetadata>> objsLayouts;

    static void RegisterNewObject(lua_State* L, const char* name, const LuaObjectField* fields);

    static void SetGCObjectField(lua_State* L, const char* objtype, lua_CFunction gcfuncion) {
        if (!objsLayouts.contains(objtype))
            return;
        luaL_getmetatable(L, objtype);
        lua_pushcfunction(L, gcfuncion);
        lua_setfield(L, -2, "__gc");
        lua_pop(L, 1);
    }

    static void NewObject(lua_State* L, const char* objtype, void* addr) {
        if (!objsLayouts.contains(objtype)) {
            lua_pushnil(L);
            return;
        }
        void** objaddr = (void**)lua_newuserdata(L, sizeof(void*));
        *objaddr = addr;
        luaC_setmetatable(L, objtype);
    }

    static void* CheckObject(lua_State* L, int narg, const char* objtype);

    static int l_index(lua_State* L);
    static int l_newindex(lua_State* L);
    static int l_eq(lua_State* L);
};