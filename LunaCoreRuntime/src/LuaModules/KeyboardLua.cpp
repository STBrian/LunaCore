#include "LuaModules.hpp"

#include <CTRPluginFramework.hpp>

#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "Core/Async.hpp"

namespace CTRPF = CTRPluginFramework;

// ----------------------------------------------------------------------------

//$Core.Keyboard

// ----------------------------------------------------------------------------

/*
- Opens the keyboard and returns the user input as string
## message: string?
## return: string?
### Core.Keyboard.getString
*/
static int l_Keyboard_getString(lua_State *L) {
    CTRPF::Keyboard keyboard;
    if (lua_gettop(L) >= 1) {
        const char *messageText = luaL_checkstring(L, 1);
        std::string &keyboardMsg = keyboard.GetMessage();
        keyboardMsg.assign(messageText);
        keyboard.DisplayTopScreen = true;
    }
    std::string inputText;
    int res = keyboard.Open(inputText);
    Core::EventRestartClock();
    Core::AsyncRestartClock();
    if (res == 0)
        lua_pushstring(L, inputText.c_str());
    else
        lua_pushnil(L);
    return 1;
}

/*
- Opens the keyboard and returns the user input as number
## message: string?
## return: number?
### Core.Keyboard.getNumber
*/
static int l_Keyboard_getNumber(lua_State *L) {
    CTRPF::Keyboard keyboard;
    if (lua_gettop(L) >= 1) {
        const char *messageText = luaL_checkstring(L, 1);
        std::string &keyboardMsg = keyboard.GetMessage();
        keyboardMsg.assign(messageText);
        keyboard.DisplayTopScreen = true;
    }
    float inputNumber;
    int res = keyboard.Open(inputNumber);
    Core::EventRestartClock();
    Core::AsyncRestartClock();
    if (res == 0)
        lua_pushnumber(L, inputNumber);
    else
        lua_pushnil(L);
    return 1;
}

/*
- Opens the keyboard and returns the user input as unsigned integer
## message: string?
## return: integer?
### Core.Keyboard.getInteger
*/
static int l_Keyboard_getInteger(lua_State *L) {
    CTRPF::Keyboard keyboard;
    if (lua_gettop(L) >= 1) {
        const char *messageText = luaL_checkstring(L, 1);
        std::string &keyboardMsg = keyboard.GetMessage();
        keyboardMsg.assign(messageText);
        keyboard.DisplayTopScreen = true;
    }
    u32 inputNumber;
    int res = keyboard.Open(inputNumber);
    Core::EventRestartClock();
    Core::AsyncRestartClock();
    if (res == 0)
        lua_pushnumber(L, inputNumber);
    else
        lua_pushnil(L);
    return 1;
}

/*
- Opens the keyboard and returns the user input as hexadecimal
## message: string?
## return: integer?
### Core.Keyboard.getHex
*/
static int l_Keyboard_getHex(lua_State *L) {
    CTRPF::Keyboard keyboard;
    if (lua_gettop(L) >= 1) {
        const char *messageText = luaL_checkstring(L, 1);
        std::string &keyboardMsg = keyboard.GetMessage();
        keyboardMsg.assign(messageText);
        keyboard.DisplayTopScreen = true;
    }
    u32 inputNumber;
    keyboard.IsHexadecimal(true);
    int res = keyboard.Open(inputNumber);
    Core::EventRestartClock();
    Core::AsyncRestartClock();
    if (res == 0)
        lua_pushnumber(L, inputNumber);
    else
        lua_pushnil(L);
    return 1;
}

static const luaL_Reg keyboard_functions[] =
{
    {"getString", l_Keyboard_getString},
    {"getNumber", l_Keyboard_getNumber},
    {"getInteger", l_Keyboard_getInteger},
    {"getHex", l_Keyboard_getHex},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterKeyboardModule(lua_State *L) {
    lua_getglobal(L, "Core");
    luaC_register_field(L, keyboard_functions, "Keyboard");
    lua_pop(L, 1);
    return true;
}