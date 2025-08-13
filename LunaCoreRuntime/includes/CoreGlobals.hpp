#pragma once

#include <string>
#include <unordered_map>
#include "types.h"

#include "lua_common.h"
#include "Core/Utils/CustomMutex.hpp"

extern lua_State *Lua_global;
extern CustomMutex Lua_Global_Mut;

extern bool patchEnabled;
extern int loadedScripts;
extern int loadedMods;

extern std::unordered_map<std::string, std::string> G_config;
extern std::unordered_map<std::string, std::string> modPaths;