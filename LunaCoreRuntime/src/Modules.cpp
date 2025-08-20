#include "Modules.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreGlobals.hpp"

#include "Core/System.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Keyboard.hpp"
#include "Core/Async.hpp"
#include "Core/Debug.hpp"
#include "Core/Memory.hpp"
#include "Core/Game/Gamepad.hpp"
#include "Core/Game/World.hpp"
#include "Core/Game/Entity.hpp"
#include "Core/Game/Player/Player.hpp"
#include "Core/Game/Items.hpp"
#include "Core/Event.hpp"
#include "Core/Graphics.hpp"

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
        Core::Module::RegisterLocalPlayerModule(L);
        Core::Module::RegisterItemsModule(L);
        Core::Module::RegisterEntityModule(L);
        Core::Module::RegisterEventModule(L);

        const char *lua_Code = R"(
            local realGame = readOnlyTable(Game, "Game")
            Game = nil

            setmetatable(_G, {
                __index = function(_, key)
                    if key == "Game" then
                        return realGame
                    else
                        return rawget(_G, key)
                    end
                end,
                __newindex = function(tbl, key, value)
                    if key == "Game" then
                        error("Cannot overwrite global 'Game'", 2)
                    else
                        rawset(tbl, key, value)
                    end
                end
            })
        )";
        if (luaL_dostring(L, lua_Code))
        {
            Core::Debug::LogError("Core::Load error: " + STRING_CLASS(lua_tostring(L, -1)));
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    static int l_Core_getModpath(lua_State* L) {
        STRING_CLASS modname(luaL_checkstring(L, 1));
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
        lua_newtable(L);
        lua_pushcfunction(L, l_Core_getModpath);
        lua_setfield(L, -2, "getModpath");
        lua_pushcfunction(L, l_Core_getTitleId);
        lua_setfield(L, -2, "getTitleId");
        lua_setglobal(L, "Core");
        Core::Module::RegisterDebugModule(L);
        Core::Module::RegisterSystemModule(L);
        Core::Module::RegisterKeyboardModule(L);
        Core::Module::RegisterFilesystemModule(L);
        Core::Module::RegisterGraphicsModule(L);
        Core::Module::RegisterMemoryModule(L);
        return true;
    }
}

void Core::LoadModules(lua_State *L)
{
    Core::RegisterCustomFileLoader(L);
    Core::RegisterUtilsModule(L);
    
    Core::RegisterAsyncModule(L);
    Core::RegisterCoreModule(L);
    Core::RegisterExtensionLoader(L);
    Core::RegisterGameModule(L);

    Core::UnregisterUtilsModule(L);
}