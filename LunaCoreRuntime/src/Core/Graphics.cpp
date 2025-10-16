#include "Core/Graphics.hpp"

#include <string>

#include <mutex>

#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "Core/Async.hpp"

namespace CTRPF = CTRPluginFramework;

extern CTRPF::PluginMenu *gmenu;
static int lua_callback = LUA_NOREF;
static bool graphicsOpen = false;
static bool shouldGraphicsClose = false;
static const CTRPF::Screen *currentScreen = NULL;
static Core::GraphicsFrameCallback graphicsFrameCallback = NULL;
static Core::GraphicsExitCallback graphicsExitCallback = NULL;

static void processEventHandler(CTRPF::Process::Event event) {
    if (event == CTRPF::Process::Event::SLEEP_ENTER)
        shouldGraphicsClose = true;
}

void Core::GraphicsOpen(GraphicsFrameCallback frameCallback, GraphicsExitCallback exitCallback) {
    graphicsFrameCallback = frameCallback;
    graphicsExitCallback = exitCallback;
    gmenu->Callback(Core::GraphicsHandlerMainloop); // Better add it as Callback to avoid script exahustion
}

bool Core::GraphicsHandlerCallback(const CTRPF::Screen& screen) {
    if (graphicsOpen)
        return false;

    //CTRPF::Process::Pause();
    //if (CTRPF::OSD::TryLock())
        //return false;
    graphicsOpen = true;

    std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
    currentScreen = &screen;
    lua_pushstring(Lua_global, screen.IsTop ? "top" : "bottom");
    Event::TriggerEvent(Lua_global, "Core.Graphics.OnNewFrame", 1);
    graphicsOpen = false;
    //CTRPF::Process::Play();
    return false;
}

void Core::GraphicsHandlerMainloop() {
    CTRPF::Process::Pause();
    CTRPF::Process::SetProcessEventCallback(processEventHandler);

    graphicsOpen = true;
    bool exit = false;
    while (!exit && !shouldGraphicsClose) {
        CTRPF::Controller::Update();

        Core::EventHandlerCallback();
        Core::AsyncHandlerCallback();

        if (CTRPF::OSD::TryLock())
            continue;

        if (graphicsFrameCallback == NULL) {
            exit = true;
            continue;
        }
        
        graphicsFrameCallback();
        
        CTRPF::OSD::SwapBuffers();
        CTRPF::OSD::Unlock();

        if (CTRPF::Controller::IsKeyReleased(CTRPF::Key::Select)) {
            exit = true;
            gmenu->ForceOpen();
        }
    }

    // Exit Graphics Mainloop
    if (graphicsExitCallback != NULL)
        graphicsExitCallback();
    CTRPF::Process::SetProcessEventCallback(nullptr);
    *gmenu -= Core::GraphicsHandlerMainloop;
    graphicsFrameCallback = NULL;
    graphicsExitCallback = NULL;
    graphicsOpen = false;
    shouldGraphicsClose = false;
    CTRPF::Process::Play();
}

static void LuaGraphicsFrameCallback() {
    if (lua_callback == LUA_NOREF) {
        shouldGraphicsClose = true;
        return;
    }
    lua_State *L = Lua_global;
    const CTRPF::Screen& topScreen = CTRPF::OSD::GetTopScreen();
    currentScreen = &topScreen;
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_callback);
    lua_pushstring(L, "top");
    if (lua_pcall(L, 1, 0, 0))
        lua_pop(L, 1);

    const CTRPF::Screen& bottomScreen = CTRPF::OSD::GetBottomScreen();
    currentScreen = &bottomScreen;
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_callback);
    lua_pushstring(L, "bottom");
    if (lua_pcall(L, 1, 0, 0))
        lua_pop(L, 1);
}

static void LuaGraphicsExitCallback() {
    lua_State *L = Lua_global;
    luaL_unref(L, LUA_REGISTRYINDEX, lua_callback);
    lua_callback = LUA_NOREF;
}

// ----------------------------------------------------------------------------

//$Core.Graphics

// ----------------------------------------------------------------------------

/*
- Stops the game and allows to draw on screen. Until Core.Graphics.close is called the function will be executed every frame
- Other events and async tasks will continue running
## func: function
### Core.Graphics.open
*/
static int l_Graphics_open(lua_State *L) {
    if (!lua_isfunction(L, 1))
        return luaL_typerror(L, 1, "function");
    
    if (graphicsOpen)
        return 0;

    if (lua_callback != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, lua_callback);
        lua_callback = LUA_NOREF;
    }
    
    lua_pushvalue(L, 1);
    lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);
    Core::GraphicsOpen(LuaGraphicsFrameCallback, LuaGraphicsExitCallback);
    return 0;
}

/*
- Resumes the game, the callback function will no longer be called and draw functions will not work
### Core.Graphics.close
*/
static int l_Graphics_close(lua_State *L) {
    if (!graphicsOpen)
        return 0;
    shouldGraphicsClose = true;
    return 0;
}

/*
- Returns if Graphics are open
### Core.Graphics.isOpen
*/
static int l_Graphics_isOpen(lua_State *L) {
    lua_pushboolean(L, graphicsOpen);
    return 1;
}

/*
- Draws a rect on screen. Only can be used when Core.Graphics.open was called
## x: integer
## y: integer
## width: integer
## height: integer
## color: integer
### Core.Graphics.drawRect
*/
static int l_Graphics_drawRect(lua_State *L)
{
    int x = luaL_checknumber(L, 1);
    int y = luaL_checknumber(L, 2);
    int width = luaL_checknumber(L, 3);
    int height = luaL_checknumber(L, 4);
    u32 color = (u32)luaL_checknumber(L, 5);

    if (!graphicsOpen)
        return 0;

    if (x < 0 || y < 0) {
        return luaL_error(L, "position must be greater than 0");
    }
    if (width < 0 || height < 0) {
        return luaL_error(L, "size must be greater than position");
    }

    currentScreen->DrawRect(x, y, width, height, color, false);
    
    return 0;
}

/*
- Draws a solid rect on screen. Only can be used when Core.Graphics.open was called
## x: integer
## y: integer
## width: integer
## height: integer
## color: integer
### Core.Graphics.drawRectFill
*/
static int l_Graphics_drawRectFill(lua_State *L)
{
    int x = luaL_checknumber(L, 1);
    int y = luaL_checknumber(L, 2);
    int width = luaL_checknumber(L, 3);
    int height = luaL_checknumber(L, 4);
    u32 color = (u32)luaL_checknumber(L, 5);

    if (!graphicsOpen)
        return 0;

    if (x < 0 || y < 0)
        return luaL_error(L, "x and y must be greater than 0");
    if ((width < 0 || height < 0))
        return luaL_error(L, "width and height must be greater than 0");

    currentScreen->DrawRect(x, y, width, height, color, true);
    
    return 0;
}

/*
- Draws a text on screen. Only can be used when Core.Graphics.open was called
## text: string
## x: integer
## y: integer
## color: integer
### Core.Graphics.drawText
*/
static int l_Graphics_drawText(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int x = luaL_checknumber(L, 2);
    int y = luaL_checknumber(L, 3);
    u32 color = (u32)luaL_checknumber(L, 4);

    if (!graphicsOpen)
        return 0;

    if (x < 0 || y < 0) {
        return luaL_error(L, "position must be greater than 0");
    }

    currentScreen->DrawSysfont(text, x, y, color);
    return 0;
}

/*
- Returns the pixel width of the string
## text: string
## return: integer
### Core.Graphics.getTextWidth
*/
static int l_Graphics_getTextWidth(lua_State *L) {
    const char* text = luaL_checkstring(L, 1);
    lua_pushnumber(L, CTRPF::OSD::GetTextWidth(true, text));
    return 1;
}

/*
- Returns a color with the r, g, b values
## r: integer
## g: integer
## b: integer
## return: integer
### Core.Graphics.colorRGB
*/
static int l_Graphics_colorRGB(lua_State *L) {
    u8 r = luaL_checknumber(L, 1);
    u8 g = luaL_checknumber(L, 2);
    u8 b = luaL_checknumber(L, 3);

    CTRPF::Color newColor(r, g, b);
    lua_pushnumber(L, newColor.raw);
    return 1;
}

/*
- Returns a color with the r, g, b, a values
## r: integer
## g: integer
## b: integer
## a: integer
## return: integer
### Core.Graphics.colorRGBA
*/
static int l_Graphics_colorRGBA(lua_State *L) {
    u8 r = luaL_checknumber(L, 1);
    u8 g = luaL_checknumber(L, 2);
    u8 b = luaL_checknumber(L, 3);
    u8 a = luaL_checknumber(L, 4);

    CTRPF::Color newColor(r, g, b, a);
    lua_pushnumber(L, newColor.raw);
    return 1;
}

static const luaL_Reg graphics_functions[] =
{
    {"open", l_Graphics_open},
    {"close", l_Graphics_close},
    {"isOpen", l_Graphics_isOpen},
    {"drawText", l_Graphics_drawText},
    {"drawRect", l_Graphics_drawRect},
    {"drawRectFill", l_Graphics_drawRectFill},
    {"colorRGB", l_Graphics_colorRGB},
    {"colorRGBA", l_Graphics_colorRGBA},
    {"getTextWidth", l_Graphics_getTextWidth},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterGraphicsModule(lua_State *L)
{
    lua_getglobal(L, "Core");
    luaC_register_field(L, graphics_functions, "Graphics");
    lua_getfield(L, -1, "Graphics");
    //$@@@Core.Graphics.OnNewFrame: EventClass
    Event::NewEvent(L, "OnNewFrame");
    lua_pop(L, 1);
    return true;
}