#pragma once

#include <atomic>
#include <unordered_map>
#include "types.h"

#include "lua_common.h"
#include "Core/Utils/CustomMutex.hpp"
#include "SharedFunctions.hpp"

extern Core::SharedFunctions SharedFunctions_obj;

extern lua_State *Lua_global;
extern CustomMutex Lua_Global_Mut;

extern bool patchEnabled;
extern int loadedScripts;
extern int loadedMods;

extern std::unordered_map<std::string, std::string> G_config;
extern std::unordered_map<std::string, std::string> modPaths;

typedef struct {
    std::atomic<bool> MainMenuLoaded = false;
    std::atomic<bool> WorldLoaded = false;
    std::atomic<bool> LoadingItems = false;
    std::atomic<bool> SettingItemsTextures = false;
    std::atomic<bool> LoadingCreativeItems = false;
    std::atomic<bool> CoreLoaded = false;
} GameState_s;

extern GameState_s GameState;