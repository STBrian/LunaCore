#include "LuaModules.hpp"
#include "CoreGlobals.hpp"
#include "Core/Debug.hpp"
#include "Core/ModSettings.hpp"
#include "Core/Utils/Utils.hpp"
#include "Helpers/LuaObject.hpp"

using namespace Core;

// ----------------------------------------------------------------------------
// ModSettingsManager implementation
// ----------------------------------------------------------------------------

ModSettingsManager& ModSettingsManager::getInstance() {
    static ModSettingsManager instance;
    return instance;
}

ModSettingsConfig* ModSettingsManager::getOrCreateConfig(const std::string& modId) {
    std::string lowerId = modId;
    Core::Utils::toLower(lowerId);
    auto it = mConfigs.find(lowerId);
    if (it == mConfigs.end()) {
        mConfigs[lowerId] = ModSettingsConfig{lowerId, {}, LUA_NOREF};
    }
    return &mConfigs[lowerId];
}

ModSettingsConfig* ModSettingsManager::getConfig(const std::string& modId) {
    std::string lowerId = modId;
    Core::Utils::toLower(lowerId);
    auto it = mConfigs.find(lowerId);
    if (it != mConfigs.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ModSettingsManager::hasSettings(const std::string& modId) {
    ModSettingsConfig* cfg = getConfig(modId);
    return cfg != nullptr && cfg->hasSettings();
}

void ModSettingsManager::notifyClose(const std::string& modId) {
    ModSettingsConfig* cfg = getConfig(modId);
    if (!cfg || cfg->onCloseCallbackRef == LUA_NOREF || !Lua_global) {
        return;
    }

    if (!Lua_Global_Mut.try_lock()) {
        return;
    }

    lua_rawgeti(Lua_global, LUA_REGISTRYINDEX, cfg->onCloseCallbackRef);
    if (lua_isfunction(Lua_global, -1)) {
        lua_newtable(Lua_global);
        for (const auto& item : cfg->items) {
            switch (item.type) {
                case SettingType::Toggle:
                    lua_pushboolean(Lua_global, item.boolValue);
                    lua_setfield(Lua_global, -2, item.id.c_str());
                    break;
                case SettingType::Slider:
                    lua_pushnumber(Lua_global, (lua_Number)item.floatValue);
                    lua_setfield(Lua_global, -2, item.id.c_str());
                    break;
                case SettingType::Dropdown:
                    if (item.selectedIndex >= 0 && item.selectedIndex < (int)item.options.size()) {
                        lua_pushstring(Lua_global, item.options[item.selectedIndex].c_str());
                    } else {
                        lua_pushnil(Lua_global);
                    }
                    lua_setfield(Lua_global, -2, item.id.c_str());
                    break;
                case SettingType::Button:
                    break;
            }
        }

        if (lua_pcall(Lua_global, 1, 0, 0) != 0) {
            std::string err = lua_tostring(Lua_global, -1);
            Core::Debug::LogError("ModSettings onClose error: " + err);
            lua_pop(Lua_global, 1);
        }
    } else {
        lua_pop(Lua_global, 1);
    }

    Lua_Global_Mut.unlock();
}

void ModSettingsManager::triggerButton(int callbackRef) {
    if (callbackRef == LUA_NOREF || !Lua_global) {
        return;
    }

    if (!Lua_Global_Mut.try_lock()) {
        return;
    }

    lua_rawgeti(Lua_global, LUA_REGISTRYINDEX, callbackRef);
    if (lua_isfunction(Lua_global, -1)) {
        if (lua_pcall(Lua_global, 0, 0, 0) != 0) {
            std::string err = lua_tostring(Lua_global, -1);
            Core::Debug::LogError("ModSettings button callback error: " + err);
            lua_pop(Lua_global, 1);
        }
    } else {
        lua_pop(Lua_global, 1);
    }

    Lua_Global_Mut.unlock();
}

// ----------------------------------------------------------------------------
// Lua bindings for ModSettings instance
// ----------------------------------------------------------------------------

static int l_ModSettings_addToggle(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    const char* id = luaL_checkstring(L, 2);
    const char* label = luaL_checkstring(L, 3);
    bool defaultVal = false;
    if (lua_gettop(L) >= 4) {
        defaultVal = lua_toboolean(L, 4);
    }

    SettingItem item;
    item.type = SettingType::Toggle;
    item.id = id;
    item.label = label;
    item.boolValue = defaultVal;
    obj->items.push_back(item);

    lua_pushvalue(L, 1);
    return 1;
}

static int l_ModSettings_addSlider(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    const char* id = luaL_checkstring(L, 2);
    const char* label = luaL_checkstring(L, 3);
    float minVal = (float)luaL_checknumber(L, 4);
    float maxVal = (float)luaL_checknumber(L, 5);
    float defaultVal = minVal;
    if (lua_gettop(L) >= 6 && !lua_isnil(L, 6)) {
        defaultVal = (float)lua_tonumber(L, 6);
    }
    float step = 1.0f;
    if (lua_gettop(L) >= 7 && !lua_isnil(L, 7)) {
        step = (float)lua_tonumber(L, 7);
    }

    SettingItem item;
    item.type = SettingType::Slider;
    item.id = id;
    item.label = label;
    item.minValue = minVal;
    item.maxValue = maxVal;
    item.floatValue = defaultVal;
    item.step = step;
    obj->items.push_back(item);

    lua_pushvalue(L, 1);
    return 1;
}

static int l_ModSettings_addDropdown(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    const char* id = luaL_checkstring(L, 2);
    const char* label = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    int defaultIndex = 1;
    if (lua_gettop(L) >= 5 && !lua_isnil(L, 5)) {
        defaultIndex = (int)luaL_checkinteger(L, 5);
    }

    std::vector<std::string> options;
    int tableLen = lua_objlen(L, 4);
    for (int i = 1; i <= tableLen; i++) {
        lua_rawgeti(L, 4, i);
        if (lua_isstring(L, -1)) {
            options.push_back(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
    }

    SettingItem item;
    item.type = SettingType::Dropdown;
    item.id = id;
    item.label = label;
    item.options = options;
    item.selectedIndex = (defaultIndex > 0 && defaultIndex <= (int)options.size()) ? (defaultIndex - 1) : 0;
    obj->items.push_back(item);

    lua_pushvalue(L, 1);
    return 1;
}

static int l_ModSettings_addButton(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    const char* id = luaL_checkstring(L, 2);
    const char* label = luaL_checkstring(L, 3);
    if (!lua_isfunction(L, 4)) {
        return luaL_error(L, "Expected function callback for button");
    }

    lua_pushvalue(L, 4);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    SettingItem item;
    item.type = SettingType::Button;
    item.id = id;
    item.label = label;
    item.luaCallbackRef = ref;
    obj->items.push_back(item);

    lua_pushvalue(L, 1);
    return 1;
}

static int l_ModSettings_onClose(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "Expected function callback for onClose");
    }

    if (obj->onCloseCallbackRef != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, obj->onCloseCallbackRef);
    }

    lua_pushvalue(L, 2);
    obj->onCloseCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    return 1;
}

static int l_ModSettings_get(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    const char* id = luaL_checkstring(L, 2);
    SettingItem* item = obj->findItem(id);
    if (!item) {
        lua_pushnil(L);
        return 1;
    }

    switch (item->type) {
        case SettingType::Toggle:
            lua_pushboolean(L, item->boolValue);
            return 1;
        case SettingType::Slider:
            lua_pushnumber(L, item->floatValue);
            return 1;
        case SettingType::Dropdown:
            if (item->selectedIndex >= 0 && item->selectedIndex < (int)item->options.size()) {
                lua_pushstring(L, item->options[item->selectedIndex].c_str());
            } else {
                lua_pushnil(L);
            }
            return 1;
        case SettingType::Button:
            lua_pushnil(L);
            return 1;
    }
    return 0;
}

static int l_ModSettings_set(lua_State* L) {
    ModSettingsConfig* obj = LuaObjectUtils::CheckObject<ModSettingsConfig>(L, 1, "ModSettings").get();
    if (!obj) return luaL_error(L, "Invalid ModSettings object");

    const char* id = luaL_checkstring(L, 2);
    SettingItem* item = obj->findItem(id);
    if (!item) {
        return luaL_error(L, "Setting '%s' not found", id);
    }

    switch (item->type) {
        case SettingType::Toggle:
            item->boolValue = lua_toboolean(L, 3);
            break;
        case SettingType::Slider:
            item->floatValue = (float)luaL_checknumber(L, 3);
            if (item->floatValue < item->minValue) item->floatValue = item->minValue;
            if (item->floatValue > item->maxValue) item->floatValue = item->maxValue;
            break;
        case SettingType::Dropdown:
            if (lua_isnumber(L, 3)) {
                int idx = (int)lua_tointeger(L, 3) - 1;
                if (idx >= 0 && idx < (int)item->options.size()) item->selectedIndex = idx;
            } else if (lua_isstring(L, 3)) {
                std::string val = lua_tostring(L, 3);
                for (size_t i = 0; i < item->options.size(); i++) {
                    if (item->options[i] == val) {
                        item->selectedIndex = (int)i;
                        break;
                    }
                }
            }
            break;
        case SettingType::Button:
            break;
    }

    lua_pushvalue(L, 1);
    return 1;
}

// ----------------------------------------------------------------------------
// Core.ModSettings static functions
// ----------------------------------------------------------------------------

static int l_ModSettings_createSettings(lua_State* L) {
    std::string modId;
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        modId = lua_tostring(L, 1);
    } else {
        modId = currentLoadingMod;
    }

    if (modId.empty()) {
        return luaL_error(L, "Mod name could not be determined for ModSettings");
    }

    ModSettingsConfig* cfg = ModSettingsManager::getInstance().getOrCreateConfig(modId);
    LuaObjectUtils::NewObject(L, "ModSettings", cfg);
    return 1;
}

static int l_ModSettings_getSettings(lua_State* L) {
    std::string modId;
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        modId = lua_tostring(L, 1);
    } else {
        modId = currentLoadingMod;
    }

    ModSettingsConfig* cfg = ModSettingsManager::getInstance().getConfig(modId);
    if (!cfg) {
        lua_pushnil(L);
        return 1;
    }

    LuaObjectUtils::NewObject(L, "ModSettings", cfg);
    return 1;
}

static int l_ModSettings_hasSettings(lua_State* L) {
    std::string modId;
    if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        modId = lua_tostring(L, 1);
    } else {
        modId = currentLoadingMod;
    }

    bool has = ModSettingsManager::getInstance().hasSettings(modId);
    lua_pushboolean(L, has);
    return 1;
}

static const luaL_Reg mod_settings_functions[] =
{
    {"createSettings", l_ModSettings_createSettings},
    {"create", l_ModSettings_createSettings},
    {"getSettings", l_ModSettings_getSettings},
    {"get", l_ModSettings_getSettings},
    {"hasSettings", l_ModSettings_hasSettings},
    {NULL, NULL}
};

bool Core::Module::RegisterModSettingsModule(lua_State *L) {
    ClassBuilder<ModSettingsConfig>(L, "ModSettings")
        .method("addToggle", l_ModSettings_addToggle)
        .method("addSlider", l_ModSettings_addSlider)
        .method("addDropdown", l_ModSettings_addDropdown)
        .method("addButton", l_ModSettings_addButton)
        .method("onClose", l_ModSettings_onClose)
        .method("get", l_ModSettings_get)
        .method("set", l_ModSettings_set)
        .build();

    lua_getglobal(L, "Core");
    luaC_register_field(L, mod_settings_functions, "ModSettings");
    lua_pop(L, 1);
    return true;
}
