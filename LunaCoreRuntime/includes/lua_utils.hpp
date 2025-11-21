#pragma once

#include <CTRPluginFramework.hpp>
#include "lua_common.h"

#include "Core/Debug.hpp"

inline void lua_table_remove(lua_State* L, int tblIdx, int vidx, const char* errorMsg = nullptr) {
    int top = lua_gettop(L);
    lua_pushvalue(L, tblIdx);
    int tblIdx2 = lua_gettop(L);

    lua_getglobal(L, "table");
    lua_getfield(L, -1, "remove");
    lua_remove(L, -2);
    lua_pushvalue(L, tblIdx2);
    lua_pushinteger(L, vidx);
    if (lua_pcall(L, 2, 1, 0)) {
        if (errorMsg)
            Core::Debug::LogError(CTRPluginFramework::Utils::Format("%s: %s", errorMsg, lua_tostring(L, -1)));
        else
            Core::Debug::LogError(CTRPluginFramework::Utils::Format("Core error: %s", lua_tostring(L, -1)));
    }

    lua_settop(L, top); // Remove all used
}