#pragma once

#include <type_traits>
#include <utility>

#include "lua_common.h"
#include "types.h"

class LuaTableElement {
    lua_State* L;
    int ref;
    int idx;

    lua_Number checknumber() {
        lua_Number val = 0;
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_rawgeti(L, -1, idx);
        if (lua_isnumber(L, -1))
            val = lua_tonumber(L, -1);
        lua_pop(L, 2);
        return val;
    }

    public:
    LuaTableElement(lua_State *state, int ref, int i) : L(state), ref(ref), idx(i) {}

    bool istype(int luatype) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_rawgeti(L, -1, idx);
        bool ist = lua_type(L, -1) == luatype;
        lua_pop(L, 2);
        return ist;
    }

    operator uint32_t() {
        return checknumber();
    }

    operator int32_t() {
        return checknumber();
    }

    operator uint16_t() {
        return checknumber();
    }

    operator int16_t() {
        return checknumber();
    }

    operator uint8_t() {
        return checknumber();
    }

    operator int8_t() {
        return checknumber();
    }

    operator float() {
        return checknumber();
    }

    operator double() {
        return checknumber();
    }

    operator const char*() {
        const char* val = "";
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_rawgeti(L, -1, idx);
        if (lua_isstring(L, -1))
            val = lua_tostring(L, -1);
        lua_pop(L, 2);
        return val;
    }

    void set(lua_Number value) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushnumber(L, value);
        lua_rawseti(L, -1, idx);
        lua_pop(L, 1);
    }

    void set(const char* value) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushstring(L, value);
        lua_rawseti(L, -1, idx);
        lua_pop(L, 1);
    }

    LuaTableElement& operator=(uint32_t value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(int32_t value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(uint16_t value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(int16_t value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(uint8_t value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(int8_t value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(float value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(double value) {
        this->set(value);
        return *this;
    }

    LuaTableElement& operator=(const char* value) {
        this->set(value);
        return *this;
    }
};

class LuaTableIterator {
    int ref;
    int itValue;
    lua_State *LState;

    public:
    LuaTableIterator(lua_State *L, int r, int v) : itValue(v), LState(L), ref(r) {}

    std::pair<int, LuaTableElement> operator*() const {
        return std::pair<int, LuaTableElement>(itValue, LuaTableElement(LState, ref, itValue));
    }

    LuaTableIterator& operator++() {
        itValue++;
        return *this;
    }

    bool operator!=(const LuaTableIterator& o) const {
        return itValue != o.itValue;
    }
};

class LuaTable {
    int len;
    int ref;
    lua_State* LState;

    public:
    LuaTable(lua_State* L, int idx) {
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

    ~LuaTable() {
        luaL_unref(LState, LUA_REGISTRYINDEX, ref);
    }

    LuaTableElement operator[](int idx) {
        return LuaTableElement(LState, ref, idx);
    }

    LuaTableIterator begin() {
        return LuaTableIterator(LState, ref, 1);
    }

    LuaTableIterator end() {
        return LuaTableIterator(LState, ref, len+1);
    }
};