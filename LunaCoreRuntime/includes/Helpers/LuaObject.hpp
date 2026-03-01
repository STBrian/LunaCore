#pragma once

#include <type_traits>
#include <functional>
#include <unordered_map>
#include <string.h>
#include "lua_common.h"
#include "types.h"

#include "Helpers/Allocation.hpp"

class DummyEmpty {};

template <typename T>
class LuaObject {
    public:
    T** ptr;

    /* Returns the stored pointer to the object data */
    T* get() const {
        return *ptr;
    }

    void clear() {
        *ptr = nullptr;
    }

    ~LuaObject() {}

    private:
    LuaObject(T** p) : ptr(p) {}
    LuaObject() {}

    friend class LuaObjectUtils;
};

class LuaObjectUtils {
    public:
    static void NewObject(lua_State* L, const char* objtype, void* addr) {
        luaL_getmetatable(L, objtype);
        if (lua_type(L, -1) == LUA_TNIL) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);
        void** objaddr = (void**)lua_newuserdata(L, sizeof(void*));
        *objaddr = addr;
        luaL_getmetatable(L, objtype);
        lua_setmetatable(L, -2);
    }

    /* Unlike NewObject, NewObjectCheck will push nil to the stack if addr is nullptr */
    static void NewObjectCheck(lua_State* L, const char* objtype, void* addr) {
        luaL_getmetatable(L, objtype);
        if (lua_type(L, -1) == LUA_TNIL) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);
        if (addr == nullptr) {
            lua_pushnil(L);
            return;
        }
        void** objaddr = (void**)lua_newuserdata(L, sizeof(void*));
        *objaddr = addr;
        luaL_getmetatable(L, objtype);
        lua_setmetatable(L, -2);
    }

    static const char* GetTypeName(lua_State* L, int narg) {
        if (lua_type(L, narg) == LUA_TUSERDATA) {
            if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
                return luaL_typename(L, narg);
            else {
                if (lua_type(L, -1) != LUA_TSTRING) {
                    lua_pop(L, 1);
                    return luaL_typename(L, narg);
                }
                const char* current = lua_tostring(L, -1);
                lua_pop(L, 1);
                return current;
            }
        } else
            return luaL_typename(L, narg);
    }

    static bool IsObject(lua_State* L, int narg, const char* objtype) {
        if (lua_type(L, narg) == LUA_TUSERDATA) {
            if (luaL_getmetafield(L, narg, "__name") == LUA_TNIL)
                return false;
            else {
                if (lua_type(L, -1) != LUA_TSTRING) {
                    lua_pop(L, 1);
                    return false;
                }
                const char* current = lua_tostring(L, -1);
                lua_pop(L, 1);
                bool valid = true;
                while (valid) {
                    if (std::strcmp(current, objtype) == 0) {
                        return true;
                    }
                    luaL_getmetatable(L, current);
                    if (lua_type(L, -1) == LUA_TNIL) {
                        lua_pop(L, 1); // pop current metatable
                        return false;
                    } else {
                        lua_getfield(L, -1, "__base");
                        if (lua_type(L, -1) != LUA_TSTRING) {
                            lua_pop(L, 1); // pop field __base
                            valid = false;
                        } else {
                            const char* nextBase = lua_tostring(L, -1);
                            lua_pop(L, 1); // pop field __base
                            luaL_getmetatable(L, nextBase);
                            if (lua_type(L, -1) == LUA_TNIL) {
                                lua_pop(L, 1); // pop nextBase metatable
                                valid = false;
                            } else {
                                lua_getfield(L, -1, "__name");
                                if (lua_type(L, -1) != LUA_TSTRING)
                                    valid = false;
                                else {
                                    current = lua_tostring(L, -1);
                                }
                                lua_pop(L, 2); // pop field __name and nextBase metatable
                            }
                        }
                        lua_pop(L, 1); // pop current metatable
                    }
                }
                return false;
            }
        } else
            return false;
    }

    /* Returns a wrapper to the object. Throws
    a Lua type error if the type is incorrect */
    template <typename T>
    static LuaObject<T> CheckObject(lua_State* L, int narg, const char* objtype) {
        if (IsObject(L, narg, objtype))
            return LuaObject<T>(static_cast<T**>(lua_touserdata(L, narg)));
        else {
            const char* typeName = GetTypeName(L, narg);
            lua_pushfstring(L, "bad argument #%d (\"%s\" expected, got \"%s\")", narg, objtype, typeName);
            lua_error(L);
            return LuaObject<T>(nullptr);
        }
    }
};

template<typename T>
struct LuaTypeName;

/* A helper class that wraps and makes it easier to create a Lua
wrapper class for a C/C++ struct or class */
template <typename T>
class ClassBuilder {
    public:
    struct PropertyBase {
        virtual int get(lua_State* L, void* obj) = 0;
        virtual int set(lua_State* L, void* obj) = 0;
        virtual ~PropertyBase() {}
    };

    template<typename A, typename M>
    struct Property : PropertyBase {
        M A::* member;
        bool readOnly;

        Property(M A::* m, bool ro = false) : member(m), readOnly(ro) {}
        
        int get(lua_State* L, void* obj) override {
            A* o = static_cast<A*>(obj);
            pushToLua(L, o->*member);
            return 1;
        }

        int set(lua_State* L, void* obj) override {
            if (readOnly)
                return luaL_error(L, "read-only field");
            if constexpr (std::is_arithmetic_v<M>) {
                A* o = static_cast<A*>(obj);
                o->*member = getFromLua<M>(L, 3);
                return 0;
            } else 
                return luaL_error(L, "constant field");
        }

        template <typename B>
        static void pushToLua(lua_State* L, B&& value) {
            if constexpr (std::is_arithmetic_v<std::decay_t<B>>) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            } else if constexpr (std::is_same_v<std::decay_t<B>, const char*> || std::is_same_v<std::decay_t<B>, char*>) {
                lua_pushstring(L, value);
            } else if constexpr (std::is_pointer_v<std::decay_t<B>>) {
                using Pointee = std::remove_pointer_t<std::decay_t<B>>;
                LuaObjectUtils::NewObjectCheck(L, LuaTypeName<Pointee>::value, value);
            } else {
                LuaObjectUtils::NewObjectCheck(L, LuaTypeName<std::decay_t<B>>::value, &value);
            }
        }

        template <typename C>
        static C getFromLua(lua_State* L, int idx) {
            if constexpr (std::is_arithmetic_v<C>) {
                if (lua_type(L, idx) != LUA_TNUMBER)
                    return luaL_error(L, "unable to set \"%s\" to a \"number\" field", luaL_typename(L, idx));
                return static_cast<C>(lua_tonumber(L, idx));
            } else if constexpr (std::is_pointer_v<C>) {
                luaL_error(L, "not implemented");
                return nullptr;
            } else {
                static_assert(sizeof(C) == 0, "Unsupported type in getFromLua");
            }
        }
    };

    template<typename A, typename R>
    struct PropertyGetter : PropertyBase {
        std::function<A(const R&)> getter;

        PropertyGetter(std::function<A(const R&)> g) : getter(g) {}

        int get(lua_State* L, void* obj) override {
            R* o = static_cast<R*>(obj);
            pushToLua(L, getter(*o));
            return 1;
        }

        int set(lua_State* L, void* obj) override {
            return luaL_error(L, "read-only field");
        }

        template <typename B>
        static void pushToLua(lua_State* L, B&& value) {
            if constexpr (std::is_arithmetic_v<std::decay_t<B>>) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            } else if constexpr (std::is_same_v<std::decay_t<B>, const char*> || std::is_same_v<std::decay_t<B>, char*>) {
                lua_pushstring(L, value);
            } else if constexpr (std::is_pointer_v<std::decay_t<B>>) {
                using Pointee = std::remove_pointer_t<std::decay_t<B>>;
                LuaObjectUtils::NewObjectCheck(L, LuaTypeName<Pointee>::value, value);
            } else {
                LuaObjectUtils::NewObjectCheck(L, LuaTypeName<std::decay_t<B>>::value, &value);
            }
        }
    };

    lua_State* L;
    const char* className;

    ClassBuilder(lua_State* L, const char* name) : L(L), className(name) {
        luaL_newmetatable(L, className);

        lua_newtable(L);
        lua_setfield(L, -2, "__properties");

        lua_newtable(L);
        lua_setfield(L, -2, "__methods");

        lua_pushstring(L, className);
        lua_setfield(L, -2, "__name");
    }

    ~ClassBuilder() {}

    ClassBuilder& setBase(const char* bname) {
        luaL_getmetatable(L, bname);
        if (lua_type(L, -1) == LUA_TNIL) {
            lua_pop(L, 1);
            return *this;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "__properties"); // Get __properties from current metatable
        luaL_getmetatable(L, bname);
        lua_newtable(L);
        lua_getfield(L, -2, "__properties");  // __properties from the base metatable
        lua_setfield(L, -2, "__index");
        lua_remove(L, -2); // Remove bname metatable so we can setmetatable to __properties
        lua_setmetatable(L, -2);
        lua_pop(L, 1);

        lua_getfield(L, -1, "__methods"); // Get __methods from current metatable
        luaL_getmetatable(L, bname);
        lua_newtable(L);
        lua_getfield(L, -2, "__methods");  // __methods from the base metatable
        lua_setfield(L, -2, "__index");
        lua_remove(L, -2); // Remove bname metatable so we can setmetatable to __methods
        lua_setmetatable(L, -2);
        lua_pop(L, 1);

        lua_pushstring(L, bname);
        lua_setfield(L, -2, "__base");

        return *this;
    }

    template <typename M>
    ClassBuilder& property(const char* name, M T::* member, bool readOnly = false) {
        lua_getfield(L, -1, "__properties");
        PropertyBase* prop = Core::alloc<Property<T,M>>(member, readOnly);
        lua_pushlightuserdata(L, prop);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
        return *this;
    }

    template <typename O, typename R>
    ClassBuilder& property_get(const char* name, std::function<R(const O&)> getter) {
        lua_getfield(L, -1, "__properties");
        PropertyBase* prop = Core::alloc<PropertyGetter<R,O>>(getter);
        lua_pushlightuserdata(L, prop);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
        return *this;
    }

    ClassBuilder& method(const char* name, lua_CFunction func) {
        lua_getfield(L, -1, "__methods");
        lua_pushcfunction(L, func);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
        return *this;
    }

    ClassBuilder& gc(lua_CFunction func) {
        lua_pushcfunction(L, func);
        lua_setfield(L, -2, "__gc");
        return *this;
    }

    static int indexDispatcher(lua_State* L) {
        void* obj = *(void**)lua_touserdata(L, 1);
        if (lua_type(L, 2) != LUA_TSTRING)
            return 0;

        const char* key = lua_tostring(L, 2);
        luaL_getmetafield(L, 1, "__properties");
        lua_getfield(L, -1, key);

        if (!lua_isnil(L, -1)) {
            PropertyBase* prop = (PropertyBase*)lua_touserdata(L, -1);
            lua_pop(L, 2);
            return prop->get(L, obj);
        }
        lua_pop(L, 2);

        // Search in methods 
        luaL_getmetafield(L, 1, "__methods");
        lua_getfield(L, -1, key);
        lua_remove(L, -2);
        return 1;
    }

    static int newindexDispatcher(lua_State* L) {
        void* obj = *(void**)lua_touserdata(L, 1);
        if (lua_type(L, 2) != LUA_TSTRING)
            return luaL_error(L, "attempt to set invalid \"?\" field");

        const char* key = lua_tostring(L, 2);
        luaL_getmetafield(L, 1, "__properties");
        lua_getfield(L, -1, key);

        if (!lua_isnil(L, -1)) {
            PropertyBase* prop = (PropertyBase*)lua_touserdata(L, -1);
            lua_pop(L, 2);
            return prop->set(L, obj);
        }
        lua_pop(L, 2);

        return luaL_error(L, "attempt to set invalid \"%s\" field", key);
    }

    static int l_eq(lua_State *L) {
        void* pItem = *(void**)lua_touserdata(L, 1);
        void* pItem2 = *(void**)lua_touserdata(L, 2);
        lua_pushboolean(L, pItem == pItem2);
        return 1;
    }

    void build() {
        lua_pushcfunction(L, indexDispatcher);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, newindexDispatcher);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, l_eq);
        lua_setfield(L, -2, "__eq");
        
        lua_pushstring(L, "protected");
        lua_setfield(L, -2, "__metatable");
        
        lua_pop(L, 1); // Pop metatable
    }
};