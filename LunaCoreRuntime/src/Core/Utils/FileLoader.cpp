#include "Core/Utils/FileLoader.hpp"

#include <algorithm>

#include "Core/Utils/Utils.hpp"

static int l_custom_loadfile(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);

    {
    std::string scriptContent = Core::Utils::LoadFile(filename);
    if (scriptContent.empty()) {
        lua_pushfstring(L, "cannot open %s", filename);
        goto __error;
    }
    if (luaL_loadbuffer(L, scriptContent.c_str(), scriptContent.size(), filename) != 0)
        goto __error;
    return 1;
    }

    __error:
    return lua_error(L);
}

static int l_custom_dofile(lua_State *L) 
{
    lua_getglobal(L, "loadfile");
    lua_pushvalue(L, 1);
    if (lua_pcall(L, 1, 1, 0) != 0)
        return lua_error(L);

    if (!lua_isfunction(L, -1))
        return luaL_error(L, "Failed to load");

    lua_Debug ar;
    if (lua_getstack(L, 1, &ar) && lua_getinfo(L, "f", &ar)) {
        lua_getfenv(L, -1);
        lua_remove(L, -2);
    } else {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
    }
    lua_setfenv(L, -2);
    lua_remove(L, 1);

    int baseTop = lua_gettop(L);
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0)
        return lua_error(L);
    
    int newtop = lua_gettop(L);
    return newtop - baseTop + 1;
}

static int l_custom_loader(lua_State *L) 
{
    const char *module_name = luaL_checkstring(L, 1);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char *path_template = lua_tostring(L, -1);
    lua_pop(L, 2);

    {
    std::string module_path(module_name);
    std::replace(module_path.begin(), module_path.end(), '.', '/');

    std::string paths(path_template);
    size_t pos = 0;
    std::string error_msgs = "\n\tno file with name '"+std::string(module_name)+"' found";

    while (pos < paths.size()) {
        size_t next = paths.find(';', pos);
        std::string path = paths.substr(pos, next - pos);
        size_t qmark = path.find('?');
        if (qmark != std::string::npos)
            path.replace(qmark, 1, module_path);
        
        std::string scriptContent = Core::Utils::LoadFile(path);
        if (!scriptContent.empty()) {
            if (luaL_loadbuffer(L, scriptContent.c_str(), scriptContent.size(), path.c_str()) == 0)
                return 1;
            else
                goto __error;
        }

        if (next == std::string::npos)
            break;
        pos = next + 1;
    }

    lua_pushstring(L, error_msgs.c_str());
    goto __error;
    }

    __error:
    return lua_error(L);
}

bool Core::RegisterCustomFileLoader(lua_State *L)
{
    lua_pushcfunction(L, l_custom_loadfile);
    lua_setglobal(L, "loadfile");
    lua_pushcfunction(L, l_custom_dofile);
    lua_setglobal(L, "dofile");

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaders");
    if (lua_istable(L, -1)) {
        for (int i = lua_objlen(L, -1) + 1; i > 1; --i) {
            lua_rawgeti(L, -1, i - 1);
            lua_rawseti(L, -2, i);
        }
        lua_pushcfunction(L, l_custom_loader);
        lua_rawseti(L, -2, 1);
    }
    else {
        lua_pop(L, 2);
        return false;
    }
    lua_pop(L, 2);
    return true;
}