#pragma once

#ifdef USE_CUSTOM_STRING
#include "string_utils.h"
#define STRING_CLASS SString
#else
#include <string>
#define STRING_CLASS std::string
#endif

#include <unordered_map>
#include "types.h"

#include "lua_common.h"
#include "Core/Utils/CustomMutex.hpp"

extern lua_State *Lua_global;
extern CustomMutex Lua_Global_Mut;

extern bool patchEnabled;
extern int loadedScripts;
extern int loadedMods;

extern std::unordered_map<STRING_CLASS, STRING_CLASS> G_config;
extern std::unordered_map<STRING_CLASS, STRING_CLASS> modPaths;