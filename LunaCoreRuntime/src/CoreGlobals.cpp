#include "CoreGlobals.hpp"

#include "CoreConstants.hpp"

namespace Core {
struct __core_ver Version;
}

lua_State *Lua_global = NULL;
Core::Mutex Lua_Global_Mut;

bool patchEnabled = false;
int loadedScripts;
int loadedMods;

Core::Config G_config(CONFIG_FILE); // Just a placeholder, it doesn't load here
std::unordered_map<std::string, std::string> modPaths;

GameState_s GameState;
CTRPluginFramework::PluginMenu *gmenu;

CTRPluginFramework::MenuFolder* MenuModsFolder;