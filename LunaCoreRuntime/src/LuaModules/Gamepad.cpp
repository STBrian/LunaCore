#include "LuaModules.hpp"

#include <3ds.h>
#include <CTRPluginFramework.hpp>

#include "Core/Event.hpp"

namespace CTRPF = CTRPluginFramework;

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/Core/Event.cpp
//!include LunaCoreRuntime/src/Modules.cpp
//$Game.Gamepad
//@KeyCode: integer

// ----------------------------------------------------------------------------

/*
- Returns true if the key is pressed (only counts for the first press, for button held use isDown)
## keycode: KeyCode
## return: boolean
### Game.Gamepad.isPressed
*/
static int l_Gamepad_isPressed(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    lua_pushboolean(L, CTRPF::Controller::IsKeyPressed(keycode));
    return 1;
}

/*
- Returns true if the key is down
## keycode: KeyCode
## return: boolean
### Game.Gamepad.isDown
*/
static int l_Gamepad_isDown(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    lua_pushboolean(L, CTRPF::Controller::IsKeyDown(keycode));
    return 1;
}

/*
- Returns true if the key just got released
## keycode: KeyCode
## return: boolean
### Game.Gamepad.isReleased
*/
static int l_Gamepad_isReleased(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    lua_pushboolean(L, CTRPF::Controller::IsKeyReleased(keycode));
    return 1;
}

/*
- Performs a virtual button press
## keycode: KeyCode
### Game.Gamepad.pressButton
*/
static int l_Gamepad_pressButton(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    CTRPF::Controller::InjectKey(keycode);
    return 0;
}

/*
- Returns touch x and y position
## return: number
## return: number
### Game.Gamepad.getTouch
*/
static int l_Gamepad_getTouch(lua_State *L)
{
    touchPosition touch;
    hidTouchRead(&touch);
    lua_pushnumber(L, touch.px);
    lua_pushnumber(L, touch.py);
    return 2;
}

static const luaL_Reg gamepad_functions[] =
{
    {"isPressed", l_Gamepad_isPressed},
    {"isDown", l_Gamepad_isDown},
    {"isReleased", l_Gamepad_isReleased},
    {"pressButton", l_Gamepad_pressButton},
    {"getTouch", l_Gamepad_getTouch},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterGamepadModule(lua_State *L)
{
    lua_getglobal(L, "Game");
    lua_newtable(L); // Gamepad

    luaL_register(L, NULL, gamepad_functions);

    //$@@@Game.Gamepad.OnKeyPressed: EventClass
    Core::Event::NewEvent(L, "OnKeyPressed");

    //$@@@Game.Gamepad.OnKeyDown: EventClass
    Core::Event::NewEvent(L, "OnKeyDown");

    //$@@@Game.Gamepad.OnKeyReleased: EventClass
    Core::Event::NewEvent(L, "OnKeyReleased");

    //$Game.Gamepad.KeyCodes
    lua_newtable(L);
    //=Game.Gamepad.KeyCodes.A = 1
    luaC_setfield_integer(L, (KEY_A), "A");
    //=Game.Gamepad.KeyCodes.B = 2
    luaC_setfield_integer(L, (KEY_B), "B");
    //=Game.Gamepad.KeyCodes.SELECT = 4
    luaC_setfield_integer(L, (KEY_SELECT), "SELECT");
    //=Game.Gamepad.KeyCodes.START = 8
    luaC_setfield_integer(L, (KEY_START), "START");
    //=Game.Gamepad.KeyCodes.DPADRIGHT = 16
    luaC_setfield_integer(L, (KEY_DRIGHT), "DPADRIGHT");
    //=Game.Gamepad.KeyCodes.DPADLEFT = 32
    luaC_setfield_integer(L, (KEY_DLEFT), "DPADLEFT");
    //=Game.Gamepad.KeyCodes.DPADUP = 64
    luaC_setfield_integer(L, (KEY_DUP), "DPADUP");
    //=Game.Gamepad.KeyCodes.DPADDOWN = 128
    luaC_setfield_integer(L, (KEY_DDOWN), "DPADDOWN");
    //=Game.Gamepad.KeyCodes.R = 256
    luaC_setfield_integer(L, (KEY_R), "R");
    //=Game.Gamepad.KeyCodes.L = 512
    luaC_setfield_integer(L, (KEY_L), "L");
    //=Game.Gamepad.KeyCodes.X = 1024
    luaC_setfield_integer(L, (KEY_X), "X");
    //=Game.Gamepad.KeyCodes.Y = 2048
    luaC_setfield_integer(L, (KEY_Y), "Y");
    //=Game.Gamepad.KeyCodes.ZL = 16384
    luaC_setfield_integer(L, (KEY_ZL), "ZL");
    //=Game.Gamepad.KeyCodes.ZR = 32768
    luaC_setfield_integer(L, (KEY_ZR), "ZR");
    //=Game.Gamepad.KeyCodes.TOUCHPAD = 1048576
    luaC_setfield_integer(L, (KEY_TOUCH), "TOUCHPAD");
    //=Game.Gamepad.KeyCodes.CSTICKRIGHT = 16777216
    luaC_setfield_integer(L, (KEY_CSTICK_RIGHT), "CSTICKRIGHT");
    //=Game.Gamepad.KeyCodes.CSTICKLEFT = 33554432
    luaC_setfield_integer(L, (KEY_CSTICK_LEFT), "CSTICKLEFT");
    //=Game.Gamepad.KeyCodes.CSTICKUP = 67108864
    luaC_setfield_integer(L, (KEY_CSTICK_UP), "CSTICKUP");
    //=Game.Gamepad.KeyCodes.CSTICKDOWN = 134217728
    luaC_setfield_integer(L, (KEY_CSTICK_DOWN), "CSTICKDOWN");
    //=Game.Gamepad.KeyCodes.CPADRIGHT = 268435456
    luaC_setfield_integer(L, (KEY_CPAD_RIGHT), "CPADRIGHT");
    //=Game.Gamepad.KeyCodes.CPADLEFT = 536870912
    luaC_setfield_integer(L, (KEY_CPAD_LEFT), "CPADLEFT");
    //=Game.Gamepad.KeyCodes.CPADUP = 1073741824
    luaC_setfield_integer(L, (KEY_CPAD_UP), "CPADUP");
    //=Game.Gamepad.KeyCodes.CPADDOWN = 2147483648
    luaC_setfield_integer(L, (KEY_CPAD_DOWN), "CPADDOWN");
    //=Game.Gamepad.KeyCodes.UP = 1073741888
    luaC_setfield_integer(L, (KEY_DUP | KEY_CPAD_UP), "UP");
    //=Game.Gamepad.KeyCodes.DOWN = 2147483776
    luaC_setfield_integer(L, (KEY_DDOWN | KEY_CPAD_DOWN), "DOWN");
    //=Game.Gamepad.KeyCodes.LEFT = 536870944
    luaC_setfield_integer(L, (KEY_DLEFT | KEY_CPAD_LEFT), "LEFT");
    //=Game.Gamepad.KeyCodes.RIGHT = 268435472
    luaC_setfield_integer(L, (KEY_DRIGHT | KEY_CPAD_RIGHT), "RIGHT");
    //=Game.Gamepad.KeyCodes.CPAD = 2952790016
    luaC_setfield_integer(L, (KEY_CPAD_DOWN | KEY_CPAD_LEFT | KEY_CPAD_RIGHT | KEY_CPAD_UP), "CPAD");
    //=Game.Gamepad.KeyCodes.CSTICK = 184549376
    luaC_setfield_integer(L, (KEY_CSTICK_DOWN | KEY_CSTICK_LEFT | KEY_CSTICK_RIGHT | KEY_CSTICK_UP), "CSTICK");
    lua_setfield(L, -2, "KeyCodes");

    lua_setfield(L, -2, "Gamepad");
    lua_pop(L, 1);
    return true;
}