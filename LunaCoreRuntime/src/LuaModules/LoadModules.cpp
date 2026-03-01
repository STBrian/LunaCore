#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreGlobals.hpp"

#include "Core/Async.hpp"
#include "Core/Debug.hpp"
#include "Core/Event.hpp"
#include "Core/Graphics/Graphics.hpp"

#include "Core/Utils/Utils.hpp"
#include "Core/Utils/FileLoader.hpp"
#include "Core/Utils/ExtensionLoader.hpp"

namespace Core {
    //Use global Game as entry point related to all game functions
    //$Game
    bool RegisterGameModule(lua_State *L)
    {
        lua_newtable(L);
        lua_setglobal(L, "Game");
        Core::Module::RegisterGamepadModule(L);
        Core::Module::RegisterWorldModule(L);
        Module::RegisterResourcesModule(L);
        Core::Module::RegisterLocalPlayerModule(L);
        Core::Module::RegisterItemsModule(L);
        Core::Module::RegisterEntityModule(L);
        Core::Module::RegisterRecipesModule(L);
        return true;
    }

    static int l_Core_getModpath(lua_State* L) {
        const char* modnamec = luaL_checkstring(L, 1);
        std::string modname(modnamec);
        Core::Utils::toLower(modname);
        if (modPaths.contains(modname))
            lua_pushstring(L, modPaths[modname].c_str());
        else
            lua_pushnil(L);
        return 1;
    }

    static int l_Core_getTitleId(lua_State* L) {
        std::string titleId;
        CTRPluginFramework::Process::GetTitleID(titleId);
        lua_pushstring(L, titleId.c_str());
        return 1;
    }

    //$Core
    /*
    - Returns the full path that corresponds to the modname if registered
    ## modname: string
    ## return: string?
    ### Core.getModpath
    */
    /*
    - Returns the title id formated in a hex string
    ## return: string
    ### Core.getTitleId
    */
    bool RegisterCoreModule(lua_State *L) {
        LOGDEBUG("Setup Core global table");
        lua_newtable(L);
        lua_pushfstring(L, "LunaCore %d.%d.%d", Core::Version.major, Core::Version.minor, Core::Version.patch);
        //=Core._VERSION = "LunaCore 0.15.0"
        lua_setfield(L, -2, "_VERSION");
        lua_pushcfunction(L, l_Core_getModpath);
        lua_setfield(L, -2, "getModpath");
        lua_pushcfunction(L, l_Core_getTitleId);
        lua_setfield(L, -2, "getTitleId");
        lua_setglobal(L, "Core");
        LOGDEBUG("Loading Debug module");
        Core::Module::RegisterDebugModule(L);
        LOGDEBUG("Loading Event module");
        Core::Module::RegisterEventModule(L);
        LOGDEBUG("Loading System module");
        Core::Module::RegisterSystemModule(L);
        LOGDEBUG("Loading Keyboard module");
        Core::Module::RegisterKeyboardModule(L);
        LOGDEBUG("Loading Filesystem module");
        Core::Module::RegisterFilesystemModule(L);
        LOGDEBUG("Loading Graphics module");
        Core::Module::RegisterGraphicsModule(L);
        LOGDEBUG("Loading Memory module");
        Core::Module::RegisterMemoryModule(L);
        LOGDEBUG("Loading Menu module");
        Core::Module::RegisterMenuModule(L);
        return true;
    }
}

void Core::LoadModules(lua_State *L)
{
    LOGDEBUG("Loading custom file loader");
    Core::RegisterCustomFileLoader(L);
    LOGDEBUG("Loading utils module");
    Core::RegisterUtilsModule(L);
    
    LOGDEBUG("Loading Async module");
    Core::RegisterAsyncModule(L);
    LOGDEBUG("Loading Core module");
    Core::RegisterCoreModule(L);
    //Core::RegisterExtensionLoader(L); not ready for release yet
    LOGDEBUG("Loading Game module");
    Core::RegisterGameModule(L);

    LOGDEBUG("Making Core and Game read-only");
    const char *lua_Code = R"(
        local realCore = readOnlyTable(Core, "Core")
        Core = nil
        local realGame = readOnlyTable(Game, "Game")
        Game = nil

        setmetatable(_G, {})
        getmetatable(_G).__index = function(_, key)
            if key == "Game" then
                return realGame
            elseif key == "Core" then
                return realCore
            else
                return rawget(_G, key)
            end
        end
        getmetatable(_G).__newindex = function(tbl, key, value)
            if key == "Game" then
                error("Cannot overwrite global 'Game'", 2)
            elseif key == "Core" then
                error("Cannot overwrite global 'Core'", 2)
            else
                rawset(tbl, key, value)
            end
        end
        getmetatable(_G).__metatable = "runtime protected"
    )";
    if (luaL_dostring(L, lua_Code)) {
        Core::Debug::ReportInternalError(lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    LOGDEBUG("Unregister utils module");
    Core::UnregisterUtilsModule(L);
}