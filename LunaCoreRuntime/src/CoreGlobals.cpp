#include "CoreGlobals.hpp"

lua_State *Lua_global = NULL;
Core::Mutex Lua_Global_Mut;

bool patchEnabled = false;
int loadedScripts;
int loadedMods;

std::unordered_map<std::string, std::string> G_config;
std::unordered_map<std::string, std::string> modPaths;

GameState_s GameState;

CTRPluginFramework::Clock timeoutAsyncClock;