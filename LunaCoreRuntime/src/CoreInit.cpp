#include "CoreInit.hpp"

#include <CTRPluginFramework.hpp>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <FsLib/fslib.hpp>

#include "json.hpp"
#include "string_hash.hpp"

#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"
#include "Modules.hpp"
#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Config.hpp"
#include "Core/Event.hpp"

#define IS_VUSA_COMP(id, version) ((id) == 0x00040000001B8700LL && (version) == 9408) // 1.9.19 USA
#define IS_VEUR_COMP(id, version) ((id) == 0x000400000017CA00LL && (version) == 9392) // 1.9.19 EUR
#define IS_VJPN_COMP(id, version) ((id) == 0x000400000017FD00LL && (version) == 9424) // 1.9.19 JPN
#define IS_TARGET_ID(id) ((id) == 0x00040000001B8700LL || (id) == 0x000400000017CA00LL || (id) == 0x000400000017FD00LL)

using json = nlohmann::json;
namespace CTRPF = CTRPluginFramework;
using namespace Core;

extern int scriptsLoadedCount;
CTRPF::Clock timeoutLoadClock;
extern std::unordered_map<std::string, std::string> config;

void Core::InitCore() {
    u64 titleID = CTRPF::Process::GetTitleID();
        if (!IS_TARGET_ID(titleID))
            return;
    CustomLockGuard Lock(Lua_Global_Mut);

    if (!fslib::initialize()) {
        Core::Debug::LogError("Failed to initialize fslib");
        Core::Debug::LogRaw(std::string(fslib::getErrorString())+"\n");
    }

    if (!fslib::openExtData(u"extdata", static_cast<uint32_t>(titleID >> 8 & 0x00FFFFFF))) {
        Core::Debug::LogError("Failed to open extdata");
        Core::Debug::LogRaw(std::string(fslib::getErrorString())+"\n");
    }

    if (!CTRPF::Directory::IsExists(PLUGIN_FOLDER"/scripts"))
        CTRPF::Directory::Create(PLUGIN_FOLDER"/scripts");

    bool loadScripts = Core::Config::GetBoolValue(config, "enable_scripts", true);

    // Update configs
    if (!Core::Config::SaveConfig(CONFIG_FILE, config))
        Core::Debug::LogMessage("Failed to save configs", true);
    
    Core::Debug::LogMessage("Loading Lua environment", false);
    Lua_global = luaL_newstate();
    Core::LoadLuaEnv();

    std::string region;
    Core::Utils::getRegion(region);
    u16 gameVer = CTRPF::Process::GetVersion();
    if (Core::Utils::checkCompatibility()) {
        Core::Debug::LogMessage(CTRPF::Utils::Format("Game region is %s, version '%hu' (1.9.19) detected", region.c_str(), gameVer), false);
    } else if (CTRPF::System::IsCitra())
        Core::Debug::LogMessage("Emulator detected, some things may work differently than on console. Patching enabled by default", true);
    else
        Core::Debug::LogMessage(CTRPF::Utils::Format("Incompatible version detected: %s '%hu'! Patching disabled", region.c_str(), gameVer), true);
    Core::Debug::LogMessage("LunaCore runtime loaded", true);

    CrashHandler::core_state = CrashHandler::CORE_LOADING_MODS;
    Core::LoadMods();

    if (loadScripts) {
        CrashHandler::core_state = CrashHandler::CORE_LOADING_SCRIPTS;
        Core::PreloadScripts(); // Executes the scripts under the scripts folder
    }
    
    Event::TriggerEvent(Lua_global, "OnGameLoad");
}

void TimeoutLoadHook(lua_State *L, lua_Debug *ar)
{
    if (timeoutLoadClock.HasTimePassed(CTRPF::Milliseconds(20000)))
        luaL_error(L, "Script load exceeded execution time (20000 ms)");
}

void Core::LoadLuaEnv() {
    lua_State* L = Lua_global;
    Core::Debug::LogMessage("Loading standard libs", false);
    luaL_openlibs(L);
    Core::Debug::LogMessage("Loading core modules", false);
    Core::LoadModules(L);

    // Set Lua path
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char *current_path = lua_tostring(L, -1);
    lua_pop(L, 1);
    std::string newPath(current_path);
    newPath += ";" PLUGIN_FOLDER "/scripts/?.lua;" PLUGIN_FOLDER "/scripts/?/init.lua";
    lua_pushstring(L, newPath.c_str());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
    Core::Debug::LogMessage("Lua environment loaded", false);
}

bool Core::LoadBuffer(const char *buffer, size_t size, const char* name) {
    lua_State* L = Lua_global;
    bool success = true;
    lua_newtable(L); // -
    lua_newtable(L); // --
    lua_getglobal(L, "_G"); // --
    lua_setfield(L, -2, "__index"); // --
    lua_getglobal(L, "_G"); // --
    lua_getmetatable(L, -1); // ---
    lua_getfield(L, -1, "__newindex"); // ----
    int ref = luaL_ref(L, LUA_REGISTRYINDEX); // ----
    lua_pop(L, 2); // --
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref); // --
    lua_setfield(L, -2, "__newindex"); // --
    lua_setmetatable(L, -2); // -
    luaL_unref(L, LUA_REGISTRYINDEX, ref); // -

    int status_code = luaL_loadbuffer(L, buffer, size, name);
    if (status_code)
    {
        Core::Debug::LogError("Script load error: "+std::string(lua_tostring(L, -1)));
        lua_pop(L, 2);
        success = false;
    }
    else
    {
        // Execute script on a private env
        lua_pushvalue(L, -2);
        lua_setfenv(L, -2);

        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        lua_remove(L, -2);

        int errfunc = lua_gettop(L);
        lua_insert(L, errfunc - 1);

        lua_sethook(L, TimeoutLoadHook, LUA_MASKCOUNT, 100);
        timeoutLoadClock.Restart();
        if (lua_pcall(L, 0, 0, errfunc - 1))
        {
            std::string error_msg(lua_tostring(L, -1));
            Core::Utils::Replace(error_msg, "\t", "    ");
            Core::Debug::LogError("Script load error: "+error_msg);
            lua_pop(L, 3);
            success = false;
        }
        else
        {
            lua_pop(L, 1);
            // If success copy state to global env
            lua_getglobal(L, "_G");
            lua_pushnil(L);
            while (lua_next(L, -3))
            {
                lua_pushvalue(L, -2);
                lua_insert(L, -2);
                lua_settable(L, -4);
            }
            lua_pop(L, 2);
        }
        lua_sethook(L, nullptr, 0, 0);
    }
    return success;
}

bool Core::LoadScript(const std::string& fp)
{
    lua_State* L = Lua_global;
    std::string fileContent = Core::Utils::LoadFile(fp);
    if (fileContent.empty())
    {
        Core::Debug::LogMessage("Failed to open file"+fp, false);
        return false;
    }
    return LoadBuffer(fileContent.c_str(), fileContent.size(), fp.c_str());
}

void Core::PreloadScripts()
{
    CTRPF::Directory dir;
    CTRPF::Directory::Open(dir, PLUGIN_FOLDER"/scripts");
    if (!dir.IsOpen())
        return;

    std::vector<std::string> files;
    dir.ListFiles(files, ".lua");
    if (files.size() > 0) {
        Core::Debug::LogMessage("Loading scripts", false);
        for (auto &file : files) {
            if (strcmp(file.c_str() + file.size() - 4, ".lua") != 0) // Double check to skip not .lua files
                continue;

            std::string fullPath("sdmc:" + dir.GetFullName() + "/" + file);
            Core::Debug::LogMessage("Loading script '"+fullPath+"'", false);
            if (LoadScript(fullPath.c_str()))
                scriptsLoadedCount++;
        }
        lua_gc(Lua_global, LUA_GCCOLLECT, 0); // If needed collect all garbage
    }
}

bool LoadMod(const std::string& modName, std::unordered_map<std::string, std::string> &modsAvailable, std::vector<u32> &modsLoading, std::vector<u32> &modsLoaded, std::vector<u32> &modsDiscarded)
{
    if (std::find(modsDiscarded.begin(), modsDiscarded.end(), hash(modName.c_str())) != modsDiscarded.end() || 
    std::find(modsLoading.begin(), modsLoading.end(), hash(modName.c_str())) != modsLoading.end())
        return false;
    if (std::find(modsLoaded.begin(), modsLoaded.end(), hash(modName.c_str())) != modsLoaded.end())
        return true;

    modsLoading.emplace_back(hash(modName.c_str()));
    std::string fileContent = Core::Utils::LoadFile(PLUGIN_FOLDER "/mods/" + modsAvailable[modName] + "/mod.json");
    if (fileContent.empty()) {
        Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to open 'mod.json'", modsAvailable[modName].c_str()));
        modsDiscarded.emplace_back(hash(modName.c_str()));
        return false;
    }
    
    json j = json::parse(fileContent, nullptr, false);
    fileContent.clear();
    if (j.is_discarded()) {
        Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to parse 'mod.json'", modsAvailable[modName].c_str()));
        modsDiscarded.emplace_back(hash(modName.c_str()));
        return false;
    }

    if (j.contains("dependencies") && j["dependencies"].is_array()) {
        for (const auto& modDependency : j["dependencies"]) {
            bool sucess = false;
            if (modDependency.is_string() && modsAvailable.contains(modDependency))
                sucess = LoadMod(modsAvailable[modDependency], modsAvailable, modsLoading, modsLoaded, modsDiscarded);

            if (!sucess) {
                Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to load dependency '%s'", modsAvailable[modName].c_str(), std::string(modDependency).c_str()));
                modsDiscarded.emplace_back(hash(modName.c_str()));
                return false;
            }
        }
    }

    if (j.contains("optional_dependencies") && j["optional_dependencies"].is_array()) {
        for (const auto& modDependency : j["optional_dependencies"]) {
            if (modDependency.is_string() && modsAvailable.contains(modDependency))
                LoadMod(modsAvailable[modDependency], modsAvailable, modsLoading, modsLoaded, modsDiscarded);
        }
    }

    if (!CTRPF::File::Exists(PLUGIN_FOLDER "/mods/" + modsAvailable[modName] + "/init.lua")) {
        Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to open 'init.lua'", modsAvailable[modName].c_str()));
        modsDiscarded.emplace_back(hash(modName.c_str()));
        return false;
    }
    modPaths[modName] = PLUGIN_FOLDER "/mods/" + modsAvailable[modName];
    if (!Core::LoadScript(PLUGIN_FOLDER "/mods/" + modsAvailable[modName] + "/init.lua")) {
        Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to load 'init.lua'", modsAvailable[modName].c_str()));
        modsDiscarded.emplace_back(hash(modName.c_str()));
        modPaths.erase(modName);
        return false;
    }
    auto it = std::find(modsLoading.begin(), modsLoading.end(), hash(modName.c_str()));
    if (it != modsLoading.end())
        modsLoading.erase(it);
    modsLoaded.emplace_back(hash(modName.c_str()));
    return true;
}

void Core::LoadMods() 
{
    CTRPF::Directory modsDir;
    CTRPF::Directory::Open(modsDir, PLUGIN_FOLDER "/mods");
    if (!modsDir.IsOpen()) {
        Core::Debug::LogMessage("No mods loaded", false);
        return;
    }

    std::unordered_map<std::string, std::string> modsAvailable;
    std::vector<u32> modsLoading;
    std::vector<u32> modsLoaded;
    std::vector<u32> modsDiscarded;

    std::vector<std::string> mods;
    modsDir.ListDirectories(mods);
    for (auto& dir : mods) {
        std::string fileContent = Core::Utils::LoadFile(PLUGIN_FOLDER "/mods/" + dir + "/mod.json");
        if (fileContent.empty()) {
            Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to open 'mod.json'", dir.c_str()));
            continue;
        }
        
        json j = json::parse(fileContent, nullptr, false);
        if (j.is_discarded()) {
            Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to parse 'mod.json'", dir.c_str()));
            continue;
        }

        if (j.contains("name") && j["name"].is_string()) {
            modsAvailable[j["name"]] = dir;
        } else {
            Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Missing name in 'mod.json'", dir.c_str()));
            continue;
        }
    }

    int modsLoadedCount = 0;
    for (auto it = modsAvailable.begin(); it != modsAvailable.end(); it++) {
        if (std::find(modsDiscarded.begin(), modsDiscarded.end(), hash(it->first.c_str())) != modsDiscarded.end())
            continue;
        if (std::find(modsLoaded.begin(), modsLoaded.end(), hash(it->first.c_str())) != modsLoaded.end()) {
            modsLoadedCount++;
            continue;
        }
        if (LoadMod(it->first, modsAvailable, modsLoading, modsLoaded, modsDiscarded))
            modsLoadedCount++;
    }
    if (modsLoadedCount == 0)
        Core::Debug::LogMessage("No mods loaded", false);
    modsLoaded.clear();
    modsDiscarded.clear();
    modsLoading.clear();
    modsAvailable.clear();
}