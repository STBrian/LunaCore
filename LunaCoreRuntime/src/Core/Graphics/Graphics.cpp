#include "Core/Graphics/Graphics.hpp"

#include <string>
#include <mutex>

#include "lua_object.hpp"
#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "Core/Async.hpp"

#include "Core/Graphics/GraphicsManager.hpp"

namespace CTRPF = CTRPluginFramework;

extern CTRPF::PluginMenu *gmenu;
static int lua_callback = LUA_NOREF;
static bool graphicsOpen = false;
static bool shouldGraphicsClose = false;
static const CTRPF::Screen *currentScreen = NULL;
static Core::GraphicsFrameCallback graphicsFrameCallback = NULL;
static Core::GraphicsExitCallback graphicsExitCallback = NULL;

bool Core::GraphicsHandlerCallback(const CTRPF::Screen& screen) {
    if (graphicsOpen || !Lua_Global_Mut.try_lock())
        return false;

    graphicsOpen = true;
    currentScreen = &screen;
    lua_pushstring(Lua_global, screen.IsTop ? "top" : "bottom");
    Event::TriggerEvent(Lua_global, "Core.Graphics.OnNewFrame", 1);
    graphicsOpen = false;
    Lua_Global_Mut.unlock();
    return false;
}

// ----------------------------------------------------------------------------

//$Core.Graphics
//@@Drawable
//$@@@GRect: Drawable
//$@@@GLabel: Drawable

// ----------------------------------------------------------------------------

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

/*
- Creates a new instance of a rectangle drawable object
## x: integer
## y: integer
## width: integer
## height: integer
## return: GRect
### Core.Graphics.newRect
*/
static int l_Graphics_newRect(lua_State *L) {
    short x = luaL_checknumber(L, 1);
    short y = luaL_checknumber(L, 2);
    short width = luaL_checknumber(L, 3);
    short height = luaL_checknumber(L, 4);

    Core::Rect* obj = new Core::Rect(x, y, width, height, false);
    LuaObject::NewObject(L, "GRect", obj);
    return 1;
}

/*
- Creates a new instance of a label drawable object
## x: integer
## y: integer
## text: string
## return: GLabel
### Core.Graphics.newRect
*/
static int l_Graphics_newRect(lua_State *L) {
    short x = luaL_checknumber(L, 1);
    short y = luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);

    Core::Label* obj = new Core::Label(x, y, false);
    LuaObject::NewObject(L, "GRect", obj);
    return 1;
}

static const luaL_Reg graphics_functions[] =
{
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

/*
- Sets if the object should be drawn
## visible: boolean
### Drawable:setVisible
*/
static int l_Graphics_Drawable_setVisible(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    bool visible = lua_toboolean(L, 2);
    obj->visible = visible;
    return 0;
}

/*
- Destroys the reference. Do NOT use the object after calling this
### GRect:destroy
*/
static int l_Graphics_GRect_gc(lua_State* L) {
    Core::Rect** obj = (Core::Rect**)LuaObject::CheckObject(L, 1, "GRect");
    if (*obj != nullptr) {
        Core::GraphicsManager& ins = Core::GraphicsManager::getInstance();
        ins.Lock();
        ins.removeObject(*obj);
        delete *obj;
        *obj = nullptr;
    }
    return 0;
}

static const LuaObjectField DrawableFields[] = {
    {"setVisible", OBJF_TYPE_METHOD, (u32)l_Graphics_Drawable_setVisible},
    {NULL, OBJF_TYPE_NIL, 0}
};

static const LuaObjectField GRectFields[] = {
    {"destroy", OBJF_TYPE_METHOD, (u32)l_Graphics_GRect_gc},
    {NULL, OBJF_TYPE_NIL, 0}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterGraphicsModule(lua_State *L)
{
    LuaObject::RegisterNewObject(L, "Drawable", DrawableFields);
    LuaObject::RegisterNewObject(L, "GRect", GRectFields);
    LuaObject::SetParent("GRect", "Drawable");
    lua_getglobal(L, "Core");
    luaC_register_field(L, graphics_functions, "Graphics");
    lua_getfield(L, -1, "Graphics");
    //$@@@Core.Graphics.OnNewFrame: EventClass
    Event::NewEvent(L, "OnNewFrame");
    lua_pop(L, 1);
    return true;
}