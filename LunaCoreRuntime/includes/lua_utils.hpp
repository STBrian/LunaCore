#pragma once

#include <CTRPluginFramework.hpp>
#include "lua_common.h"

#include "Core/Debug.hpp"


#define LUAUTILS_INIT_ERROR_HANDLER() const char* __genericerrorMsg = ""

#define LUAUTILS_SET_ERROR_HANDLER(L)  __lua_genericerror_handler:              \
    return luaL_error(L, __genericerrorMsg)



#define LUAUTILS_SET_CUSTOMERROR_HANDLER(L)  __lua_emptyerror_handler:          \
    return lua_error(L)



#define LUAUTILS_INIT_TYPEERROR_HANDLER() const char* __expectedType = "";      \
    const char* __gotType = "";                                                 \
    const char* __typeerrorMsg = "expected type \"%s\" (got \"%s\")"

#define LUAUTILS_SET_TYPEERROR_MESSAGE(msg) __typeerrorMsg = msg

#define LUAUTILS_SET_TYPEERROR_HANDLER(L)  __lua_typeerror_handler:             \
    return luaL_error(L, __typeerrorMsg, __expectedType, __gotType)



#define LUAUTILS_ERROR(msg) do {                                                \
    __genericerrorMsg = msg;                                                    \
    goto __lua_genericerror_handler;                                            \
    } while (0)

#define LUAUTILS_CUSTOMERROR(L, fmt, ...) do {                                  \
    lua_pushfstring(L, fmt, ##__VA_ARGS__);                                     \
    goto __lua_emptyerror_handler;                                              \
    } while (0)

#define LUAUTILS_CHECKTYPE(L, type, n) do {                                     \
    if (lua_type(L, n) != type) {                                               \
        __expectedType = lua_typename(L, type);                                 \
        __gotType = luaL_typename(L, n);                                        \
        goto __lua_typeerror_handler;                                           \
    } } while (0)

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