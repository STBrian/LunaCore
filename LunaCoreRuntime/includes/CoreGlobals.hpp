#pragma once

#if __STDC_HOSTED__
#include <CTRPluginFramework.hpp>

#include <atomic>
#include <unordered_map>
#endif

#include "types.h"
#include "lua_common.h"
#include "Helpers/Mutex.hpp"

namespace Core {
struct __core_ver {
    u8 major, minor, patch;
};

extern struct __core_ver Version;
}

extern lua_State *Lua_global;
extern Core::Mutex Lua_Global_Mut;

extern bool patchEnabled;
extern int loadedScripts;
extern int loadedMods;

#if __STDC_HOSTED__
extern std::unordered_map<std::string, std::string> G_config;
extern std::unordered_map<std::string, std::string> modPaths;

typedef struct {
    std::atomic<bool> MainMenuLoaded = false;
    std::atomic<bool> WorldLoaded = false;
    std::atomic<bool> LoadingItems = false;
    std::atomic<bool> SettingItemsTextures = false;
    std::atomic<bool> LoadingCreativeItems = false;
    std::atomic<bool> LoadingRecipes = false;
    std::atomic<bool> CoreLoaded = false;
} GameState_s;

extern GameState_s GameState;
extern CTRPluginFramework::PluginMenu *gmenu;

extern CTRPluginFramework::MenuFolder* MenuModsFolder;

#endif