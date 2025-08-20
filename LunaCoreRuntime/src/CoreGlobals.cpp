#include "CoreGlobals.hpp"

lua_State *Lua_global = NULL;
CustomMutex Lua_Global_Mut;

bool patchEnabled = false;
int loadedScripts;
int loadedMods;

std::unordered_map<STRING_CLASS, STRING_CLASS> G_config;
std::unordered_map<STRING_CLASS, STRING_CLASS> modPaths;