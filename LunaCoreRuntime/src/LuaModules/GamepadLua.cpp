#include "LuaModules.hpp"

#include <3ds.h>
#include <CTRPluginFramework.hpp>

#include "Core/Event.hpp"

namespace CTRPF = CTRPluginFramework;

// ----------------------------------------------------------------------------

//!include LunaCoreRuntime/src/LuaModules/LoadModules.cpp

/* @doc:v2
@module Game.Gamepad
@desc Gamepad input handling module
*/

/* @doc:v2
@alias KeyCode integer
*/

// ----------------------------------------------------------------------------

/* @doc:v2
@func Game.Gamepad.isPressed(keycode: KeyCode): boolean
@desc Returns true if the key is pressed (only counts for the first press, for button held use isDown)
@param keycode KeyCode
@return boolean
*/
static int l_Gamepad_isPressed(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    lua_pushboolean(L, CTRPF::Controller::IsKeyPressed(keycode));
    return 1;
}

/* @doc:v2
@func Game.Gamepad.isDown(keycode: KeyCode): boolean
@desc Returns true if the key is down
@param keycode KeyCode
@return boolean
*/
static int l_Gamepad_isDown(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    lua_pushboolean(L, CTRPF::Controller::IsKeyDown(keycode));
    return 1;
}

/* @doc:v2
@func Game.Gamepad.isReleased(keycode: KeyCode): boolean
@desc Returns true if the key just got released
@param keycode KeyCode
@return boolean
*/
static int l_Gamepad_isReleased(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    lua_pushboolean(L, CTRPF::Controller::IsKeyReleased(keycode));
    return 1;
}

/* @doc:v2
@func Game.Gamepad.pressButton(keycode: KeyCode)
@desc Performs a virtual button press
@param keycode KeyCode
*/
static int l_Gamepad_pressButton(lua_State *L)
{
    CTRPF::Key keycode = (CTRPF::Key)luaL_checknumber(L, 1);
    CTRPF::Controller::InjectKey(keycode);
    return 0;
}

/* @doc:v2
@func Game.Gamepad.getTouch(): number
@desc Returns touch x and y position
@return number
@return number
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

    /* @doc:v2
    @class OnKeyPressed: EventClass
    @target Game.Gamepad.OnKeyPressed
    */
    Core::Event::NewEvent(L, "OnKeyPressed");

    /* @doc:v2
    @class OnKeyDown: EventClass
    @target Game.Gamepad.OnKeyDown
    */
    Core::Event::NewEvent(L, "OnKeyDown");

    /* @doc:v2
    @class OnKeyReleased: EventClass
    @target Game.Gamepad.OnKeyReleased
    */
    Core::Event::NewEvent(L, "OnKeyReleased");

    /* @doc:v2
    @table Game.Gamepad.KeyCodes
    @desc Button and key code constants
    @const Game.Gamepad.KeyCodes.A = 1
    @const Game.Gamepad.KeyCodes.B = 2
    @const Game.Gamepad.KeyCodes.SELECT = 4
    @const Game.Gamepad.KeyCodes.START = 8
    @const Game.Gamepad.KeyCodes.DPADRIGHT = 16
    @const Game.Gamepad.KeyCodes.DPADLEFT = 32
    @const Game.Gamepad.KeyCodes.DPADUP = 64
    @const Game.Gamepad.KeyCodes.DPADDOWN = 128
    @const Game.Gamepad.KeyCodes.R = 256
    @const Game.Gamepad.KeyCodes.L = 512
    @const Game.Gamepad.KeyCodes.X = 1024
    @const Game.Gamepad.KeyCodes.Y = 2048
    @const Game.Gamepad.KeyCodes.ZL = 16384
    @const Game.Gamepad.KeyCodes.ZR = 32768
    @const Game.Gamepad.KeyCodes.TOUCHPAD = 1048576
    @const Game.Gamepad.KeyCodes.CSTICKRIGHT = 16777216
    @const Game.Gamepad.KeyCodes.CSTICKLEFT = 33554432
    @const Game.Gamepad.KeyCodes.CSTICKUP = 67108864
    @const Game.Gamepad.KeyCodes.CSTICKDOWN = 134217728
    @const Game.Gamepad.KeyCodes.CPADRIGHT = 268435456
    @const Game.Gamepad.KeyCodes.CPADLEFT = 536870912
    @const Game.Gamepad.KeyCodes.CPADUP = 1073741824
    @const Game.Gamepad.KeyCodes.CPADDOWN = 2147483648
    @const Game.Gamepad.KeyCodes.UP = 1073741888
    @const Game.Gamepad.KeyCodes.DOWN = 2147483776
    @const Game.Gamepad.KeyCodes.LEFT = 536870944
    @const Game.Gamepad.KeyCodes.RIGHT = 268435472
    @const Game.Gamepad.KeyCodes.CPAD = 2952790016
    @const Game.Gamepad.KeyCodes.CSTICK = 184549376
    */
    lua_newtable(L);
    luaC_setfield_integer(L, (KEY_A), "A");
    luaC_setfield_integer(L, (KEY_B), "B");
    luaC_setfield_integer(L, (KEY_SELECT), "SELECT");
    luaC_setfield_integer(L, (KEY_START), "START");
    luaC_setfield_integer(L, (KEY_DRIGHT), "DPADRIGHT");
    luaC_setfield_integer(L, (KEY_DLEFT), "DPADLEFT");
    luaC_setfield_integer(L, (KEY_DUP), "DPADUP");
    luaC_setfield_integer(L, (KEY_DDOWN), "DPADDOWN");
    luaC_setfield_integer(L, (KEY_R), "R");
    luaC_setfield_integer(L, (KEY_L), "L");
    luaC_setfield_integer(L, (KEY_X), "X");
    luaC_setfield_integer(L, (KEY_Y), "Y");
    luaC_setfield_integer(L, (KEY_ZL), "ZL");
    luaC_setfield_integer(L, (KEY_ZR), "ZR");
    luaC_setfield_integer(L, (KEY_TOUCH), "TOUCHPAD");
    luaC_setfield_integer(L, (KEY_CSTICK_RIGHT), "CSTICKRIGHT");
    luaC_setfield_integer(L, (KEY_CSTICK_LEFT), "CSTICKLEFT");
    luaC_setfield_integer(L, (KEY_CSTICK_UP), "CSTICKUP");
    luaC_setfield_integer(L, (KEY_CSTICK_DOWN), "CSTICKDOWN");
    luaC_setfield_integer(L, (KEY_CPAD_RIGHT), "CPADRIGHT");
    luaC_setfield_integer(L, (KEY_CPAD_LEFT), "CPADLEFT");
    luaC_setfield_integer(L, (KEY_CPAD_UP), "CPADUP");
    luaC_setfield_integer(L, (KEY_CPAD_DOWN), "CPADDOWN");
    luaC_setfield_integer(L, (KEY_DUP | KEY_CPAD_UP), "UP");
    luaC_setfield_integer(L, (KEY_DDOWN | KEY_CPAD_DOWN), "DOWN");
    luaC_setfield_integer(L, (KEY_DLEFT | KEY_CPAD_LEFT), "LEFT");
    luaC_setfield_integer(L, (KEY_DRIGHT | KEY_CPAD_RIGHT), "RIGHT");
    luaC_setfield_integer(L, (KEY_CPAD_DOWN | KEY_CPAD_LEFT | KEY_CPAD_RIGHT | KEY_CPAD_UP), "CPAD");
    luaC_setfield_integer(L, (KEY_CSTICK_DOWN | KEY_CSTICK_LEFT | KEY_CSTICK_RIGHT | KEY_CSTICK_UP), "CSTICK");
    lua_setfield(L, -2, "KeyCodes");

    lua_setfield(L, -2, "Gamepad");
    lua_pop(L, 1);
    return true;
}