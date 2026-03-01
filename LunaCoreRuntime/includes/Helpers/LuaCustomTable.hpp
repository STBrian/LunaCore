#pragma once

#include <string>

#include "Core/Debug.hpp"
#include "lua_common.h"
#include "types.h"

enum LuaCustomTableErrorCode {
    LUACT_ERROR = -1,
    LUACT_INVALID_KEY = -2
};

/* Checks the type of the value. If it doesn't match, it pushes the 
error string to the stack and makes the function to return -1  */
#define LUACT_CHECKTYPE(L, type, n) do {                                        \
    if (lua_type(L, n) != type) {                                               \
        lua_pushfstring(L, "unable to assign \"%s\" to a \"%s\" field",         \
            luaL_typename(L, n), lua_typename(L, type));                        \
        return LUACT_ERROR;                                                     \
    } } while (0)

class LuaCustomTable {
    public:
    /* Return values:
    >=0: Success
    -1: Error returned. Error string must be in stack
    -2: Invalid key */
    static int l_index(lua_State* L) {
        if (lua_isuserdata(L, 1))
            LOGDEBUG("Userdata in custom table index?");
        if (luaL_getmetafield(L, 1, "__ctname") == LUA_TNIL) {
            LOGDEBUG("Table but no __ctname?");
            return 0;
        }
        if (lua_type(L, -1) != LUA_TSTRING) {
            LOGDEBUG("__ctname is not a string?!");
            lua_pop(L, 1);
            return 0;
        }
        const char* ctname = lua_tostring(L, -1);
        lua_pop(L, 1);

        if (luaL_getmetafield(L, 1, "__callindex") == LUA_TNIL) {
            LOGDEBUG("No __callindex!");
            return 0;
        }
        if (lua_type(L, -1) != LUA_TFUNCTION) {
            LOGDEBUG("__callindex must be a function");
            lua_pop(L, 1);
            return 0;
        }
        lua_CFunction fun = lua_tocfunction(L, -1);
        lua_pop(L, 1);
        int result = fun(L);
        if (result == -1)
            return lua_error(L);
        else if (result == -2) {
            if (lua_type(L, 2) != LUA_TSTRING)
                return luaL_error(L, "field \"?\" is not a valid member of \"%s\"", ctname);
            else
                return luaL_error(L, "\"%s\" is not a valid member of \"%s\"", lua_tostring(L, 2), ctname);
        }
        return result;
    }
    static int l_newindex(lua_State* L) {
        if (luaL_getmetafield(L, 1, "__ctname") == LUA_TNIL)
            return 0;
        if (lua_type(L, -1) != LUA_TSTRING) {
            lua_pop(L, 1);
            return 0;
        }
        const char* ctname = lua_tostring(L, -1);
        lua_pop(L, 1);

        if (luaL_getmetafield(L, 1, "__callnewindex") == LUA_TNIL)
            return luaL_error(L, "'%s' is not editable", ctname);
        if (lua_type(L, -1) != LUA_TFUNCTION) {
            lua_pop(L, 1);
            return 0;
        }
        lua_CFunction fun = lua_tocfunction(L, -1);
        lua_pop(L, 1);
        int result = fun(L);
        if (result == -1)
            return lua_error(L);
        else if (result == -2) {
            if (lua_type(L, 2) != LUA_TSTRING)
                return luaL_error(L, "attempt to set field \"?\" of \"%s\"", ctname);
            else
                return luaL_error(L, "\"%s\" is not a valid member of \"%s\" or is a read-only value", lua_tostring(L, 2), ctname);
        }
        return result;
    }

    /* Creates a template to be used to create new custom tables. The resulting
    table won't be editable using scripts */
    static void RegisterNewCustomTable(lua_State* L, const std::string& tname) {
        luaL_newmetatable(L, (tname + "CustomTableMt").c_str());

        lua_newtable(L); // IndexTable
        lua_newtable(L); // IndexMetatable
        lua_pushcfunction(L, l_index);
        lua_setfield(L, -2, "__index");
        lua_pushstring(L, tname.c_str());
        lua_setfield(L, -2, "__ctname");
        lua_setmetatable(L, -2); // Set IndexTable metatable

        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_newindex);
        lua_setfield(L, -2, "__newindex");
        
        lua_pushstring(L, tname.c_str());
        lua_setfield(L, -2, "__ctname");
        
        lua_pushstring(L, "protected");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1); // Pop metatable
    }

    /* Creates a template to be used to create new custom tables.
    Note: index and newindex callbacks can return -1 to indicate an error. 
    The error string must be passed to the stack. Newindex callback can return
    -2 to indicate an invalid key error */
    static void RegisterNewCustomTable(lua_State* L, const std::string& tname, lua_CFunction indexfun, lua_CFunction newindexfun) {
        luaL_newmetatable(L, (tname + "CustomTableMt").c_str());

        lua_newtable(L); // IndexTable
        lua_newtable(L); // IndexMetatable
        lua_pushcfunction(L, l_index);
        lua_setfield(L, -2, "__index");
        lua_pushstring(L, tname.c_str());
        lua_setfield(L, -2, "__ctname");
        lua_pushcfunction(L, indexfun);
        lua_setfield(L, -2, "__callindex");
        lua_setmetatable(L, -2); // Set IndexTable metatable

        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, indexfun);
        lua_setfield(L, -2, "__callindex");

        lua_pushcfunction(L, l_newindex);
        lua_setfield(L, -2, "__newindex");
        
        lua_pushcfunction(L, newindexfun);
        lua_setfield(L, -2, "__callnewindex");
        
        lua_pushstring(L, tname.c_str());
        lua_setfield(L, -2, "__ctname");
        
        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1); // Pop metatable
    }

    /* Creates a template to be used to create new custom tables. The resulting
    custom table won't be editable using scripts.
    Note: index callback can return -1 to indicate an error. 
    The error string must be passed to the stack */
    static void RegisterNewCustomTable(lua_State* L, const std::string& tname, lua_CFunction indexfun) {
        luaL_newmetatable(L, (tname + "CustomTableMt").c_str());

        lua_newtable(L); // IndexTable
        lua_newtable(L); // IndexMetatable
        lua_pushcfunction(L, l_index);
        lua_setfield(L, -2, "__index");
        lua_pushstring(L, tname.c_str());
        lua_setfield(L, -2, "__ctname");
        lua_pushcfunction(L, indexfun);
        lua_setfield(L, -2, "__callindex");
        lua_setmetatable(L, -2); // Set IndexTable metatable

        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, indexfun);
        lua_setfield(L, -2, "__callindex");

        lua_pushcfunction(L, l_newindex);
        lua_setfield(L, -2, "__newindex");
        
        lua_pushstring(L, tname.c_str());
        lua_setfield(L, -2, "__ctname");
        
        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1); // Pop metatable
    }

    /* Puts the table that is used for normal index on the stack */
    static void GetIndexTable(lua_State* L, const std::string& tname) {
        luaL_getmetatable(L, (tname + "CustomTableMt").c_str());
        lua_getfield(L, -1, "__index");
        lua_remove(L, -2);
    }

    /* Creates a new custom table and puts it on the stack. 
    tname: Name of the registered custom table to use as template */
    static void NewCustomTable(lua_State* L, const std::string& tname) {
        lua_newuserdata(L, 4); // Use userdata to protect table from rawset
        luaL_getmetatable(L, (tname + "CustomTableMt").c_str());
        lua_setmetatable(L, -2);
    }

    /* Creates a new custom table and appends it to a table with 
    the same name as tname. 
    idx: The index of the table to append to
    tname: Name of the registered custom table to use as template */
    static void SetNewCustomTable(lua_State* L, int idx, const std::string& tname) {
        lua_pushvalue(L, idx);
        lua_newuserdata(L, 4); // Use userdata to protect table from rawset
        luaL_getmetatable(L, (tname + "CustomTableMt").c_str());
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, tname.c_str());
        lua_pop(L, 1);
    }
};