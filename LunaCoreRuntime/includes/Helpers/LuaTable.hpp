#pragma once

#include <type_traits>

#include "lua_common.h"
#include "types.h"

template <typename T>
requires std::is_arithmetic_v<T>
class LuaNumberTableIterator {
    int ref;
    int itValue;
    lua_State *LState;

    public:
    LuaNumberTableIterator(lua_State *L, int r, int v) : itValue(v), LState(L), ref(r) {}

    T operator*() const {
        lua_rawgeti(LState, LUA_REGISTRYINDEX, ref);
        lua_rawgeti(LState, -1, itValue);
        T num = 0;
        if (lua_isnumber(LState, -1))
            num = (T)lua_tonumber(LState, -1);
        lua_pop(LState, 2);
        return num;
    }

    LuaNumberTableIterator& operator++() {
        itValue++;
        return *this;
    }

    bool operator!=(const LuaNumberTableIterator& o) const {
        return itValue != o.itValue;
    }
};

template <typename T>
requires std::is_arithmetic_v<T>
class LuaNumberTable {
    int len;
    int ref;
    lua_State* LState;

    public:
    LuaNumberTable(lua_State* L, int idx) {
        if (!lua_istable(L, idx)) {
            len = 0;
        } else {
            lua_pushvalue(L, idx);
            len = lua_objlen(L, -1);
            ref = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        LState = L;
    }

    int size() {
        return len;
    }

    ~LuaNumberTable() {
        luaL_unref(LState, LUA_REGISTRYINDEX, ref);
    }

    LuaNumberTableIterator<T> begin() {
        return LuaNumberTableIterator<T>(LState, ref, 1);
    }

    LuaNumberTableIterator<T> end() {
        return LuaNumberTableIterator<T>(LState, ref, len+1);
    }
};