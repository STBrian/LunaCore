#include "CoreInit.hpp"

#include <CTRPluginFramework.hpp>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <string>
#include <FsLib/fslib.hpp>

#include "json.hpp"
#include "string_hash.hpp"

#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Config.hpp"
#include "Core/Event.hpp"
#include "Core/Filesystem.hpp"

#include "lua_common.h"
#include "LuaModules.hpp"
#include "CoreConstants.hpp"
#include "CoreGlobals.hpp"
#include "ExtendedHeap.hpp"

#include "game/gmalloc.h"

using json = nlohmann::ordered_json;
namespace CTRPF = CTRPluginFramework;
using namespace Core;

CTRPF::Clock timeoutLoadClock;

#define HEAP_PLUGIN 0
#define HEAP_GAME_APP 1
#define HEAP_GAME_LINEAR 2

u32 pluginHeapUsage = 0;
#define PLUGIN_HEAP_MAX (1 * 1024 * 1024) // 1 Mb

void* custom_lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (nsize == 0 && ptr != nullptr) {
        // ExtendedHeapFree(ptr);
        u32* heapType = ((u32*)ptr - 2);
        if (*heapType == HEAP_PLUGIN) {
            pluginHeapUsage -= osize + 8;
            free(heapType);
        } else
            gstd_free(heapType);
        return NULL;
    } else {
        if (ptr == NULL) {
            if (pluginHeapUsage >= PLUGIN_HEAP_MAX) {
                // NOTE: As far as I know, the game's allocator returns memory from the app heap
                // if it's called from outside a game's thread. That makes glinearAlloc the same
                // as gstd_malloc in this context
                void* mem = glinearMemAlign(nsize + 8, 8);
                u32* p = (u32*)mem;
                *p = HEAP_GAME_APP;
                return p + 2;
            } else {
                void* mem = malloc(nsize + 8);
                if (!mem) return nullptr;
                pluginHeapUsage += nsize + 8;
                u32* p = (u32*)mem;
                *p = HEAP_PLUGIN;
                return p + 2;
            }
        }
        else {
            u32* heapType = ((u32*)ptr - 2);
            if (*heapType == HEAP_PLUGIN) {
                void* mem = realloc(heapType, nsize + 8);
                if (!mem) return nullptr;
                pluginHeapUsage += (ssize_t)(nsize + 8) - (ssize_t)(osize + 8);
                return (u32*)mem + 2;
            } else
                return (u32*)custom_glinearReallocAligned(heapType, nsize + 8, 8) + 2;
        }
    }
}

void* extended_lua_allocator_extended(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (nsize == 0 && ptr != nullptr) {
        ExtendedHeapFree(ptr);
        return NULL;
    } else {
        if (ptr == NULL) {
            return ExtendedHeapMalloc(nsize);
        } else {
            return ExtendedHeapRealloc(ptr, nsize);
        }
    }
}

static void readString(Core::File& file, std::string& line) {
    s8 chr;
    while ((chr = file.getByte()) != '\0' && chr != -1) {
        line += chr;
        file.seek(1, SEEK_CUR);
    }
    file.seek(1, SEEK_CUR);
    return;
}

extern "C" Result  PLGLDR__GetPluginPath(char *path);

void Core::GetCoreInfo(std::string& plgTitle, std::string& plgAuthor, std::string& plgSummary, std::string& plgDescription) {
    /* I was looking into how to get plugin info from plugin loader or framework
    but apparently the loader just ignore them, so... manual work */
    char path[255] = {0};
    PLGLDR__GetPluginPath(path);
    std::string fullPath = "sdmc:" + std::string(path);
    Core::File pluginFile(fullPath, FS_OPEN_READ);
    if (!pluginFile.isOpen()) return;

    pluginFile.seek(0x94, SEEK_SET);
    readString(pluginFile, plgTitle);
    readString(pluginFile, plgAuthor);
    readString(pluginFile, plgSummary);
    readString(pluginFile, plgDescription);
    pluginFile.close();
}

void Core::ParseVersion(u32 ver) {
    Core::Version.major = (ver >> 24);
    Core::Version.minor = (ver >> 16) & 0xFF;
    Core::Version.patch = (ver >> 8) & 0xFF;
}

void Core::InitCore() {
    u64 titleID = CTRPF::Process::GetTitleID();
    std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);

    if (!Core::Filesystem::DirectoryExists(LC_FS"/scripts"))
        Core::Filesystem::CreateDirectory(LC_FS"/scripts");

    bool loadScripts = G_config.getBool("enable_scripts", true);
    
    Core::Debug::LogInfo("Loading Lua environment");
    #ifdef EXPERIMENTAL
    Lua_global = lua_newstate(extended_lua_allocator_extended, NULL);
    #else
    Lua_global = lua_newstate(custom_lua_allocator, NULL);
    // Lua_global = luaL_newstate();
    #endif
    Core::LoadLuaEnv();

    std::string region;
    Core::Utils::getRegion(region);
    u16 gameVer = CTRPF::Process::GetVersion();
    if (Core::Utils::checkCompatibility()) {
        Core::Debug::LogInfo(CTRPF::Utils::Format("Game region is %s, version '%hu' (1.9.19) detected", region.c_str(), gameVer));
    } else if (CTRPF::System::IsCitra())
        Core::Debug::LogWarn("Emulator detected, some things may work differently than on console. Patching enabled by default");
    else
        Core::Debug::LogWarn(CTRPF::Utils::Format("Incompatible version detected: %s '%hu'! Patching disabled", region.c_str(), gameVer));
    Core::Debug::Message("LunaCore runtime loaded");

    CrashHandler::core_state = CrashHandler::CORE_LOADING_MODS;
    Core::LoadMods();

    if (loadScripts) {
        CrashHandler::core_state = CrashHandler::CORE_LOADING_SCRIPTS;
        Core::PreloadScripts(); // Executes the scripts under the scripts folder
    }
}

void TimeoutLoadHook(lua_State *L, lua_Debug *ar)
{
    if (timeoutLoadClock.HasTimePassed(CTRPF::Seconds(60)))
        luaL_error(L, "Script load exceeded execution time (60000 ms)");
}

void Core::LoadLuaEnv() {
    lua_State* L = Lua_global;
    Core::Debug::LogInfo("Loading standard libs");
    luaL_openlibs(L);
    Core::Debug::LogInfo("Loading core modules");
    Core::LoadModules(L);

    // Set Lua path
    LOGDEBUG("Adding Lua path");
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char *current_path = lua_tostring(L, -1);
    lua_pop(L, 1);
    std::string newPath(current_path);
    newPath += ";" PLUGIN_FOLDER "/scripts/?.lua;" PLUGIN_FOLDER "/scripts/?/init.lua";
    lua_pushstring(L, newPath.c_str());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);

    // Remove debug.getmetatable
    LOGDEBUG("Removing debug.getmetatable");
    lua_getglobal(L, "debug");
    lua_pushnil(L);
    lua_setfield(L, -2, "getmetatable");
    lua_pop(L, 1);

    Core::Debug::LogInfo("Lua environment loaded");
}

bool Core::LoadBuffer(const char *buffer, size_t size, const char* name) {
    lua_State* L = Lua_global;
    int topIdx = lua_gettop(L);
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
        Core::Debug::LogError("Script load error: " + std::string(lua_tostring(L, -1)));
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
    lua_settop(L, topIdx);
    if (!success) {
        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
    return success;
}

bool Core::LoadScript(const std::string& fp, const std::string& name)
{
    lua_State* L = Lua_global;
    std::string fileContent = Core::Utils::LoadFile(fp);
    if (fileContent.empty())
    {
        Core::Debug::LogInfo("Failed to open file "+fp);
        return false;
    }
    return LoadBuffer(fileContent.c_str(), fileContent.size(), name.c_str());
}

void Core::PreloadScripts()
{
    std::vector<std::string> files;
    Core::Filesystem::GetDirectoryEntries(PLUGIN_FOLDER"/scripts", files);
    if (files.size() > 0) {
        Core::Debug::LogInfo("Loading scripts");
        for (auto &file : files) {
            if (!file.ends_with(".lua"))
                continue;

            std::string fullPath(PLUGIN_FOLDER"/scripts/" + file);
            if (!Core::Filesystem::FileExists(fullPath)) {
                continue;
            }

            Core::Debug::LogInfo("Loading script "+fullPath);
            if (LoadScript(fullPath, "scripts/" + file))
                loadedScripts++;
        }
        lua_gc(Lua_global, LUA_GCCOLLECT, 0); // If needed collect all garbage
    }
}

typedef struct {
    int major, minor, patch;
} SemVer;

static bool parse_semver(const char* v_str, SemVer* out) {
    return sscanf(v_str, "%d.%d.%d", &out->major, &out->minor, &out->patch) == 3;
}

bool LoadMod(std::string modName, std::unordered_map<std::string, std::string>& modsAvailable, std::vector<u32> &modsLoading, std::vector<u32> &modsLoaded, std::vector<u32> &modsDiscarded)
{
    std::vector<u32>::iterator it;
    json j;

    Core::Utils::toLower(modName);
    if (std::find(modsDiscarded.begin(), modsDiscarded.end(), hash(modName.c_str())) != modsDiscarded.end() || 
    std::find(modsLoading.begin(), modsLoading.end(), hash(modName.c_str())) != modsLoading.end())
        return false;
    if (std::find(modsLoaded.begin(), modsLoaded.end(), hash(modName.c_str())) != modsLoaded.end())
        return true;

    modsLoading.emplace_back(hash(modName.c_str()));
    std::string fileContent = Core::Utils::LoadFile(PLUGIN_FOLDER "/mods/" + modsAvailable[modName] + "/mod.json");
    if (fileContent.empty()) {
        Core::Debug::LogError(CTRPF::Utils::Format("Failed to load '%s'. Failed to load 'mod.json'", modName.c_str()));
        goto errorDiscard;
    }
    
    j = json::parse(std::string(fileContent), nullptr, false);
    fileContent.clear();
    if (j.is_discarded()) {
        Core::Debug::LogErrorf("Failed to load '%s'. Failed to parse 'mod.json'", modName.c_str());
        goto errorDiscard;
    }

    if (j.contains("min_core_version")) {
        bool compatible = true;
        auto& value = j["min_core_version"];
        SemVer min_core_ver = (SemVer){0};
        if (value.is_array() && value.size() == 3) {
            if (value[0].is_number_integer())
                min_core_ver.major = value[0];
            if (value[1].is_number_integer())
                min_core_ver.minor = value[1];
            if (value[2].is_number_integer())
                min_core_ver.patch = value[2];
        } else if (value.is_string() && parse_semver(value.get<std::string>().c_str(), &min_core_ver)) {
        } else {
            Core::Debug::LogErrorf("Failed to load '%s'. The mod config file has an invalid 'min_core_version'", modName.c_str());
            goto errorDiscard;
        }
        if (Core::Version.major < min_core_ver.major) {
            compatible = false;
        } else if (Core::Version.major == min_core_ver.major) {
            if (Core::Version.minor < min_core_ver.minor) {
                compatible = false;
            } else if (Core::Version.minor == min_core_ver.minor) {
                if (Core::Version.patch < min_core_ver.patch) {
                    compatible = false;
                }
            }
        }

        if (!compatible) {
            Core::Debug::LogErrorf("Failed to load '%s'. '%s' expects LunaCore >= \"%d.%d.%d\"", modName.c_str(), modName.c_str(), min_core_ver.major, min_core_ver.minor, min_core_ver.patch);
            goto errorDiscard;
        }
    }

    if (j.contains("dependencies") && j["dependencies"].is_array()) {
        for (const auto& modDependency : j["dependencies"]) {
            if (modDependency.is_string()) {
                std::string modDNameLower = Core::Utils::convertToLower(modDependency);
                bool sucess = false;
                if (modsAvailable.contains(modDNameLower))
                    sucess = LoadMod(modDNameLower, modsAvailable, modsLoading, modsLoaded, modsDiscarded);

                if (!sucess) {
                    Core::Debug::LogErrorf("Failed to load '%s'. Failed to load dependency '%s'", modName.c_str(), modDNameLower.c_str());
                    goto errorDiscard;
                }
            }
        }
    }

    if (j.contains("optional_dependencies") && j["optional_dependencies"].is_array()) {
        for (const auto& modDependency : j["optional_dependencies"]) {
            if (modDependency.is_string()) {
                std::string modDNameLower = Core::Utils::convertToLower(modDependency);
                if (modsAvailable.contains(modDNameLower))
                    LoadMod(modDNameLower, modsAvailable, modsLoading, modsLoaded, modsDiscarded);
            }
        }
    }

    if (!Core::Filesystem::FileExists(PLUGIN_FOLDER "/mods/" + modsAvailable[modName] + "/init.lua")) {
        Core::Debug::LogErrorf("Failed to load '%s'. Required 'init.lua' but it's missing", modName.c_str());
        goto errorDiscard;
    }
    currentLoadingMod = modName;
    modPaths[modName] = PLUGIN_FOLDER "/mods/" + modsAvailable[modName];
    if (!Core::LoadScript(PLUGIN_FOLDER "/mods/" + modsAvailable[modName] + "/init.lua", "mods/" + modsAvailable[modName] + "/init.lua")) {
        Core::Debug::LogErrorf("Failed to load '%s'. Failed to load 'init.lua'", modName.c_str());
        modPaths.erase(modName);
        goto errorDiscard;
    }

    it = std::find(modsLoading.begin(), modsLoading.end(), hash(modName.c_str()));
    if (it != modsLoading.end())
        modsLoading.erase(it);
    modsLoaded.emplace_back(hash(modName.c_str()));

    return true;

    errorDiscard:
    modsDiscarded.emplace_back(hash(modName.c_str()));
    return false;
}

void Core::LoadMods() 
{
    std::unordered_map<std::string, std::string> modsAvailable;
    std::vector<u32> modsLoading;
    std::vector<u32> modsLoaded;
    std::vector<u32> modsDiscarded;
    std::vector<std::string> mods;
    Core::Filesystem::GetDirectoryEntries(PLUGIN_FOLDER "/mods", mods);
    
    for (auto& dir : mods) {
        if (!Core::Filesystem::DirectoryExists(PLUGIN_FOLDER "/mods/" + dir))
            continue;
            
        std::string fileContent = Core::Utils::LoadFile(PLUGIN_FOLDER "/mods/" + dir + "/mod.json");
        if (fileContent.empty()) {
            Core::Debug::LogErrorf("Failed to load '%s'. Failed to load 'mod.json'", dir.c_str());
            continue;
        }
        
        json j = json::parse(std::string(fileContent), nullptr, false);
        if (j.is_discarded()) {
            Core::Debug::LogErrorf("Failed to load '%s'. Failed to parse 'mod.json'", dir.c_str());
            continue;
        }

        if (j.contains("name") && j["name"].is_string()) {
            modsAvailable[Core::Utils::convertToLower(j["name"])] = dir;
        } else {
            Core::Debug::LogErrorf("Failed to load '%s'. Missing name in 'mod.json'", dir.c_str());
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
        // This takes the name of the mod from modsAvailable key
        if (LoadMod(it->first, modsAvailable, modsLoading, modsLoaded, modsDiscarded))
            modsLoadedCount++;
    }
    if (modsLoadedCount == 0)
        Core::Debug::LogInfo("No mods loaded");
    modsLoaded.clear();
    modsDiscarded.clear();
    modsLoading.clear();
    modsAvailable.clear();
    currentLoadingMod = "";
}