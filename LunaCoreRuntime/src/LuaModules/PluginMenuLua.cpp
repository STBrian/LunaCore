#include "LuaModules.hpp"

#include "CoreGlobals.hpp"

#include "Core/Debug.hpp"

#include "Helpers/Allocation.hpp"
#include "Helpers/LuaObject.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/LuaModules/LoadModules.cpp
//$Core.Menu
//@@MenuFolder

// ----------------------------------------------------------------------------

/*
- Returns a reference to the mods plugin menu folder
## return: MenuFolder
### Core.Menu.getMenuFolder
*/
static int l_Menu_getMenuFolder(lua_State* L) {
    LuaObjectUtils::NewObject(L, "MenuFolder", MenuModsFolder);
    return 1;
}

/*
- Allows to show a message in the menu
## msg: string
### Core.Menu.showMessageBox
*/
static int l_Menu_showMessageBox(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    CTRPF::MessageBox(msg, CTRPF::DialogType::DialogOk)();
    return 0;
}

/*
- When used, an ask message box will appear on screen. Returns the user selection as boolean
## msg: string
## return: boolean
### Core.Menu.showAskMessageBox
*/
static int l_Menu_showAskMessageBox(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    lua_pushboolean(L, CTRPF::MessageBox(msg, CTRPF::DialogType::DialogYesNo)());
    return 1;
}

/*
- Returns if the plugin menu is currently open. This won't be useful, only in specific cases
## return: boolean
### Core.Menu.isOpen
*/
static int l_Menu_isOpen(lua_State* L) {
    lua_pushboolean(L, gmenu->IsOpen());
    return 1;
}

static const luaL_Reg menu_functions[] =
{
    {"getMenuFolder", l_Menu_getMenuFolder},
    {"showMessageBox", l_Menu_showMessageBox},
    {"showAskMessageBox", l_Menu_showAskMessageBox},
    {"isOpen", l_Menu_isOpen},
    {NULL, NULL}
};

/*
- Creates and appends a new folder to the current folder, and then it returns a reference to the new folder
## name: string
## return: MenuFolder
### MenuFolder:newFolder
*/
static int l_MenuFolder_newFolder(lua_State* L) {
    CTRPF::MenuFolder* obj = LuaObjectUtils::CheckObject<CTRPF::MenuFolder>(L, 1, "MenuFolder").get();
    const char* name = luaL_checkstring(L, 2);
    auto folder = alloc<CTRPF::MenuFolder>(name);
    LuaObjectUtils::NewObject(L, "MenuFolder", folder);
    obj->Append(folder);
    return 1;
}

/*
- Creates and appends a new entry to the current folder. When selected in menu it will execute the callback function
## name: string
## callback: function
### MenuFolder:newEntry
*/
static int l_MenuFolder_newEntry(lua_State* L) {
    CTRPF::MenuFolder* obj = LuaObjectUtils::CheckObject<CTRPF::MenuFolder>(L, 1, "MenuFolder").get();
    const char* name = luaL_checkstring(L, 2);
    if (!lua_isfunction(L, 3)) {
        return luaL_error(L, "expected function");
    }
    lua_pushvalue(L, 3);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    auto entry = alloc<CTRPF::MenuEntry>(name, nullptr, [](CTRPF::MenuEntry* entry) {
        int funref = (int)entry->GetArg();
        if (!Lua_Global_Mut.try_lock())
            return;
        lua_rawgeti(Lua_global, LUA_REGISTRYINDEX, funref);
        if (lua_pcall(Lua_global, 0, 0, 0)) {
            std::string error_msg(lua_tostring(Lua_global, -1));
            Core::Debug::LogError("Script error: "+error_msg);
            lua_pop(Lua_global, 1);
        }
        Lua_Global_Mut.unlock();
    });
    entry->SetArg((void*)ref);
    obj->Append(entry);
    return 0;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterMenuModule(lua_State *L) {
    ClassBuilder<DummyEmpty>(L, "MenuFolder")
        .method("newFolder", l_MenuFolder_newFolder)
        .method("newEntry", l_MenuFolder_newEntry)
        .build();
    lua_getglobal(L, "Core");
    luaC_register_field(L, menu_functions, "Menu");
    lua_pop(L, 1);
    return true;
}