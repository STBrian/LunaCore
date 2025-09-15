#include "CoreGlobals.hpp"

Core::SharedFunctions SharedFunctions_obj;

lua_State *Lua_global = NULL;
CustomMutex Lua_Global_Mut;

bool patchEnabled = false;
int loadedScripts;
int loadedMods;

std::unordered_map<std::string, std::string> G_config;
std::unordered_map<std::string, std::string> modPaths;