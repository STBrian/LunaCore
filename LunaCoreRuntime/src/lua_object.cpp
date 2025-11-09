#include "lua_object.hpp"

#include <cstring>

std::unordered_map<std::string, std::unordered_map<std::string, LuaObject::ValueMetadata>> LuaObject::objsLayouts;
std::unordered_map<std::string, std::string> LuaObject::parents;

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
        objsLayouts[name][f->key] = {f->offset, f->type, f->access, f->flags, f->objtype};
}

void** LuaObject::CheckObject(lua_State* L, int narg, const char* objtype) {
    if (lua_type(L, narg) == LUA_TUSERDATA) {
        if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
            luaL_typerror(L, narg, objtype);
        else {
            if (lua_type(L, -1) != LUA_TSTRING) {
                lua_pop(L, 1);
                luaL_typerror(L, narg, objtype);
            }
            std::string* curIns = new std::string(lua_tostring(L, -1));
            while (!curIns->empty()) {
                if (*curIns == objtype) {
                    lua_pop(L, 1);
                    delete curIns;
                    return (void**)lua_touserdata(L, narg);
                }
                if (parents.contains(*curIns))
                    *curIns = parents[*curIns];
                else
                    curIns->clear();
            }
            delete curIns;
            lua_pop(L, 1);
            lua_pushfstring(L, "bad argument #%d (%s expected, got %s)", narg, objtype, lua_tostring(L, -1));
            lua_remove(L, -2);
            lua_error(L);
        }
    } else
        luaL_typerror(L, narg, objtype);
    return nullptr;
}

bool LuaObject::IsObject(lua_State* L, int narg, const char* objtype) {
    if (lua_type(L, narg) == LUA_TUSERDATA) {
        if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
            return false;
        else {
            if (lua_type(L, -1) != LUA_TSTRING) {
                lua_pop(L, 1);
                return false;
            }
            std::string curIns = lua_tostring(L, -1);
            while (!curIns.empty()) {
                if (curIns == objtype) {
                    lua_pop(L, 1);
                    return true;
                }
                if (parents.contains(curIns))
                    curIns = parents[curIns];
                else
                    curIns.clear();
            }
            lua_pop(L, 1);
            return false;
        }
    } else
        return false;
    return false;
}

int LuaObject::l_index(lua_State* L) {
    if (lua_type(L, 1) != LUA_TUSERDATA)
        return 0;
    luaL_getmetafield(L, 1, "__name");
    if (lua_type(L, -1) != LUA_TSTRING) {
        lua_pop(L, 1);
        return 0;
    }
    std::string objName(lua_tostring(L, -1));
    lua_pop(L, 1);
    if (!objsLayouts.contains(objName))
        return 0;
    
    void* objOffset = *(void**)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;
    
    std::string currClass = objName;
    std::string key(lua_tostring(L, 2));
    bool valid = false;
    while (!currClass.empty() && !valid) {
        if (!objsLayouts[currClass].contains(key)) {
            if (parents.contains(currClass))
                currClass = parents[currClass];
            else
                currClass.clear();
            continue;
        }

        valid = true;
        ValueMetadata& md = objsLayouts[currClass][key];
        u32 offset = md.offset;
        const char* objtype = md.objtype;
        void* dataOff;
        if (md.flags & OBJF_FLAG_ABS)
            dataOff = (void*)offset;
        else
            dataOff = (void*)((u32)objOffset + offset);
        

        switch (md.type) {
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
            case OBJF_TYPE_METHOD: case OBJF_TYPE_FUNCTION:
                lua_pushcfunction(L, (lua_CFunction)offset);
                break;
            case OBJF_TYPE_OBJECT:
                LuaObject::NewObject(L, objtype, dataOff);
                break;
            case OBJF_TYPE_OBJECT_POINTER:
                if (*(void**)dataOff) {
                    LuaObject::NewObject(L, objtype, *(void**)dataOff);
                } else
                    lua_pushnil(L);
                break;
        }
    }
    return valid;
}

int LuaObject::l_newindex(lua_State* L) {
    if (lua_type(L, 1) != LUA_TUSERDATA)
        return 0;
    luaL_getmetafield(L, 1, "__name");
    if (lua_type(L, -1) != LUA_TSTRING) {
        lua_pop(L, 1);
        return 0;
    }
    const char* objName = lua_tostring(L, -1);
    lua_pop(L, 1);
    if (!objsLayouts.contains(objName))
        return 0;
    
    void* objOffset = *(void**)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return 0;

    const char* key = lua_tostring(L, 2);
    std::string* currClassPtr = new std::string(objName); // I made a ptr so I can manually delete it when doing a lua_error, maybe there is a better way of doing this
    bool valid = false;
    while (!currClassPtr->empty()) {
        if (!objsLayouts[*currClassPtr].contains(key)) {
            if (parents.contains(*currClassPtr))
                *currClassPtr = parents[*currClassPtr];
            else
                currClassPtr->clear();
            continue;
        }

        if (objsLayouts[*currClassPtr][key].access != OBJF_ACCESS_ALL) {
            delete currClassPtr;
            return luaL_error(L, "unable to assign value to read-only field %s", key);
        }
        ValueMetadata& md = objsLayouts[*currClassPtr][key];
        delete currClassPtr;
        valid = true;
            
        u32 offset = md.offset;
        void* dataOff;
        if (md.flags & OBJF_FLAG_ABS)
            dataOff = (void*)offset;
        else
            dataOff = (void*)((u32)objOffset + offset);

        switch (md.type) {
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
            case OBJF_TYPE_FUNCTION:
                return luaL_error(L, "unable to assign value to constant %s data type", "function");
                break;
            case OBJF_TYPE_OBJECT: case OBJF_TYPE_OBJECT_POINTER:
                return luaL_error(L, "unable to assign value to constant %s data type", "object");
                break;
        }
    }
    if (!valid)
        return luaL_error(L, "unable to assign value to invalid field %s", key);
    return 0;
}

int LuaObject::l_eq(lua_State *L) {
    void* pItem = *(void**)lua_touserdata(L, 1);
    void* pItem2 = *(void**)lua_touserdata(L, 2);
    lua_pushboolean(L, pItem == pItem2);
    return 1;
}