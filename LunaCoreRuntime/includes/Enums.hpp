#pragma once

#include "lua_common.h"

#include <unordered_map>
#include <string>

#include "Helpers/LuaObject.hpp"
#include "Helpers/Allocation.hpp"

namespace Core {
    class EnumItem {
        public: 
        const char* name;
        int value;
        const char* enumType;

        EnumItem(const char* n, int v, const char* t) : name(n), value(v), enumType(t) {}
    };

    class EnumGroup {
        public:
        const char* enumType;
        std::unordered_map<std::string, EnumItem*> items;

        EnumGroup(const char* et) : enumType(et) {}

        EnumItem* addItem(const char* pname, int pvalue) {
            items[pname] = Core::alloc<EnumItem>(pname, pvalue, enumType);
            return items[pname];
        }

        EnumItem* getItemByName(const char* pname) {
            if (items.contains(pname)) {
                return items[pname];
            }
            return nullptr;
        }

        EnumItem* getItemByValue(int pvalue) {
            for (auto& item : items) {
                if (item.second->value == pvalue)
                    return item.second;
            }
            return nullptr;
        }
    };

    class EnumGroups {
        public:
        std::unordered_map<std::string, EnumGroup> groups;

        static EnumGroups& getInstance();

        EnumGroup& addGroup(const char* name) {
            groups.emplace(std::string(name), EnumGroup(name));
            return groups.at(name);
        }

        EnumGroup& getGroup(const char* name) {
            return groups.at(name);
        }

        private:
        EnumGroups() {}
    };

    class EnumItemUtils {
        public:
        /* If invalid type, returns nullptr and optionally pushes the error string to the
        lua stack */
        static EnumItem* LuaToEnumItemOrNull(lua_State* L, int narg, const char* typeName, bool pushError = false) {
            EnumItem* val = nullptr;
            if (LuaObjectUtils::IsObject(L, narg, typeName))
                val = LuaObjectUtils::CheckObject<EnumItem>(L, 3, typeName).get();
            else if (lua_isstring(L, 3)) {
                const char* tname = lua_tostring(L, 3);
                EnumGroup& group = EnumGroups::getInstance().getGroup(typeName);
                val = group.getItemByName(tname);
                if (!val && pushError)
                    lua_pushfstring(L, "Invalid \"%s\": \"%s\"", typeName, tname);
            } else if (lua_isnumber(L, 3)) {
                int tvalue = lua_tonumber(L, 3);
                EnumGroup& group = EnumGroups::getInstance().getGroup(typeName);
                val = group.getItemByValue(tvalue);
                if (!val && pushError)
                    lua_pushfstring(L, "Invalid \"%s\" value: \"%d\"", typeName, tvalue);
            }
            else {
                if (pushError)
                    lua_pushfstring(L, "unable to assign \"%s\" to a \"%s\" field", luaL_typename(L, narg), typeName); 
            }
            return val;
        }
    };
}