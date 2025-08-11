#include "lua_object.hpp"

#include <string.h>

std::unordered_map<std::string, std::unordered_map<std::string, LuaObject::ValueMetadata>> LuaObject::objsLayouts;

void LuaObject::RegisterNewObject(lua_State* L, const char* name, const LuaObjectField* fields) {
    luaL_newmetatable(L, name);
    lua_pushcfunction(L, LuaObject::l_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, LuaObject::l_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, LuaObject::l_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushstring(L, name);
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    objsLayouts[name] = {};
    for (const LuaObjectField* f = fields; f->key != NULL; f++)
        objsLayouts[name][f->key] = {f->offset, f->type, f->access};
}

void* LuaObject::CheckObject(lua_State* L, int narg, const char* objtype) {
    if (lua_type(L, narg) == LUA_TUSERDATA) {
        if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
            luaL_typerror(L, narg, objtype);
        else {
            if (lua_type(L, -1) != LUA_TSTRING) {
                lua_pop(L, 1);
                luaL_typerror(L, narg, objtype);
            }
            if (std::strcmp(objtype, lua_tostring(L, -1)) != 0) {
                lua_pushfstring(L, "bad argument #%d (%s expected, got %s)", narg, objtype, lua_tostring(L, -1));
                lua_remove(L, -2);
                lua_error(L);
            } else {
                lua_pop(L, 1);
                return lua_touserdata(L, narg);
            }
        }
    } else
        luaL_typerror(L, narg, objtype);
    return nullptr;
}

int LuaObject::l_index(lua_State* L) {
    if (lua_type(L, 1) != LUA_TUSERDATA)
        return 0;
    luaL_getmetafield(L, 1, "__name");
    if (lua_type(L, -1) != LUA_TSTRING) {
        lua_pop(L, 1);
        return 0;
    }
    std::string objName = lua_tostring(L, -1);
    lua_pop(L, 1);
    if (!objsLayouts.contains(objName))
        return 0;
    
    void* objOffset = *(void**)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;
    std::string key = lua_tostring(L, 2);
    if (!objsLayouts[objName].contains(key))
        return 0;

    void* dataOff = (void*)((u32)objOffset + objsLayouts[objName][key].offset);

    switch (objsLayouts[objName][key].type) {
        case OBJF_TYPE_CHAR:
            lua_pushnumber(L, *(char*)dataOff);
            break;
        case OBJF_TYPE_SHORT:
            lua_pushnumber(L, *(short*)dataOff);
            break;
        case OBJF_TYPE_INT:
            lua_pushnumber(L, *(int*)dataOff);
            break;
        case OBJF_TYPE_FLOAT:
            lua_pushnumber(L, *(float*)dataOff);
            break;
        case OBJF_TYPE_DOUBLE:
            lua_pushnumber(L, *(double*)dataOff);
            break;
        case OBJF_TYPE_STRING:
            lua_pushstring(L, *(char**)dataOff);
            break;
        case OBJF_TYPE_METHOD:
            lua_pushcfunction(L, (lua_CFunction)objsLayouts[objName][key].offset);
            break;
        default:
            lua_pushnil(L);
            break;
    }
    return 1;
}

int LuaObject::l_newindex(lua_State* L) {
    if (lua_type(L, 1) != LUA_TUSERDATA)
        return 0;
    luaL_getmetafield(L, 1, "__name");
    if (lua_type(L, -1) != LUA_TSTRING) {
        lua_pop(L, 1);
        return 0;
    }
    std::string objName = lua_tostring(L, -1);
    lua_pop(L, 1);
    if (!objsLayouts.contains(objName))
        return 0;
    
    void* objOffset = *(void**)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;
    std::string key = lua_tostring(L, 2);
    if (!objsLayouts[objName].contains(key))
        return 0;

    void* dataOff = (void*)((u32)objOffset + objsLayouts[objName][key].offset);
    if (objsLayouts[objName][key].access != OBJF_ACCESS_ALL)
        return luaL_error(L, "unable to assign value to read-only field %s", key.c_str());

    switch (objsLayouts[objName][key].type) {
        case OBJF_TYPE_CHAR:
            if (lua_type(L, 3) != LUA_TNUMBER)
                return luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, 3), "integer");
            *(char*)dataOff = (char)lua_tonumber(L, 3);
            break;
        case OBJF_TYPE_SHORT:
            if (lua_type(L, 3) != LUA_TNUMBER)
                return luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, 3), "integer");
            *(short*)dataOff = (short)lua_tonumber(L, 3);
            break;
        case OBJF_TYPE_INT:
            if (lua_type(L, 3) != LUA_TNUMBER)
                return luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, 3), "integer");
            *(int*)dataOff = (int)lua_tonumber(L, 3);
            break;
        case OBJF_TYPE_FLOAT:
            if (lua_type(L, 3) != LUA_TNUMBER)
                return luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, 3), lua_typename(L, LUA_TNUMBER));
            *(float*)dataOff = (float)lua_tonumber(L, 3);
            break;
        case OBJF_TYPE_DOUBLE:
            if (lua_type(L, 3) != LUA_TNUMBER)
                return luaL_error(L, "unable to assign %s to %s data type", luaL_typename(L, 3), lua_typename(L, LUA_TNUMBER));
            *(double*)dataOff = (double)lua_tonumber(L, 3);
            break;
        case OBJF_TYPE_STRING:
            return luaL_error(L, "unable to assign value to constant %s data type", "string");
            break;
        case OBJF_TYPE_METHOD:
            return luaL_error(L, "unable to assign value to constant %s data type", "method");
            break;
        default:
            return luaL_error(L, "unable to assign value to invalid field %s", key.c_str());
            break;
    }
    return 0;
}

int LuaObject::l_eq(lua_State *L) {
    void* pItem = *(void**)lua_touserdata(L, 1);
    void* pItem2 = *(void**)lua_touserdata(L, 2);
    lua_pushboolean(L, pItem == pItem2);
    return 1;
}