#include "Core/Graphics/Graphics.hpp"

#include <string>
#include <mutex>

#include "lua_object.hpp"
#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "Core/Async.hpp"

#include "Core/Graphics/GraphicsManager.hpp"

namespace CTRPF = CTRPluginFramework;

using GraphicsFrameCallback = void(*)(void);
using GraphicsExitCallback = void(*)(void);

static int lua_callback = LUA_NOREF;
static bool graphicsOpen = false;
static bool shouldGraphicsClose = false;
static const CTRPF::Screen *currentScreen = NULL;
static GraphicsFrameCallback graphicsFrameCallback = NULL;
static GraphicsExitCallback graphicsExitCallback = NULL;

static void processEventHandler(CTRPF::Process::Event event) {
    if (event == CTRPF::Process::Event::SLEEP_ENTER)
        shouldGraphicsClose = true;
}

/* TODO: This graphic API is pretty bad, so it need to be fully changed */
void GraphicsHandlerMainloop() {
    if (gmenu->IsOpen())
        return; // wait until closed
    CTRPF::Process::Pause();
    CTRPF::Process::SetProcessEventCallback(processEventHandler);

    graphicsOpen = true;
    bool exit = false;
    while (!exit && !shouldGraphicsClose) {
        CTRPF::Controller::Update();

        Core::EventHandlerCallback();
        Core::Scheduler::Update();

        if (CTRPF::OSD::TryLock())
            continue;

        if (graphicsFrameCallback == NULL) {
            exit = true;
            continue;
        }
        
        graphicsFrameCallback();
        
        CTRPF::OSD::SwapBuffers();
        CTRPF::OSD::Unlock();

        if (CTRPF::Controller::IsKeyPressed(CTRPF::Key::Select)) {
            exit = true;
            gmenu->ForceOpen();
        }
    }

    // Exit Graphics Mainloop
    if (graphicsExitCallback != NULL)
        graphicsExitCallback();
    CTRPF::Process::SetProcessEventCallback(nullptr);
    *gmenu -= GraphicsHandlerMainloop;
    graphicsFrameCallback = NULL;
    graphicsExitCallback = NULL;
    graphicsOpen = false;
    shouldGraphicsClose = false;
    CTRPF::Process::Play();
}

void GraphicsOpen(GraphicsFrameCallback frameCallback, GraphicsExitCallback exitCallback) {
    graphicsFrameCallback = frameCallback;
    graphicsExitCallback = exitCallback;
    gmenu->Callback(GraphicsHandlerMainloop); // Better add it as Callback to avoid script exahustion
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
//@@Drawable
//$@@@GRect: Drawable
//$@@@GLabel: Drawable

// ----------------------------------------------------------------------------

/*
- Stops the game and allows to draw on screen. Until Core.Graphics.close is called the function will be executed every frame
- Other events and async tasks will continue running. If plugin menu is open, this will wait until it is closed
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
    GraphicsOpen(LuaGraphicsFrameCallback, LuaGraphicsExitCallback);
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
### Core.Graphics.newLabel
*/
static int l_Graphics_newLabel(lua_State *L) {
    short x = luaL_checknumber(L, 1);
    short y = luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);

    Core::Label* obj = new Core::Label(x, y, false);
    obj->setText(text);
    LuaObject::NewObject(L, "GLabel", obj);
    return 1;
}

/*
- Adds a Drawable object to render on screen
## obj: Drawable
### Core.Graphics.add
*/
static int l_Graphics_add(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    if (obj == nullptr)
        return 0;

    Core::GraphicsManager& mgr = Core::GraphicsManager::getInstance();
    mgr.addObject(obj);
    return 0;
}

/*
- Remove a Drawable object from the render
## obj: Drawable
### Core.Graphics.remove
*/
static int l_Graphics_remove(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    if (obj == nullptr)
        return 0;

    Core::GraphicsManager& mgr = Core::GraphicsManager::getInstance();
    mgr.removeObject(obj);
    return 0;
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
    {"newRect", l_Graphics_newRect},
    {"newLabel", l_Graphics_newLabel},
    {"add", l_Graphics_add},
    {"remove", l_Graphics_remove},
    {NULL, NULL}
};

// ----------------------------------------------------------------------------

#define ERROR_OBJECT_DESTROYED(L) luaL_error(L, "object was destroyed")

/*
- Sets if the object should be drawn
## visible: boolean
### Drawable:setVisible
*/
static int l_Graphics_Drawable_setVisible(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    bool visible = lua_toboolean(L, 2);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->visible = visible;
    return 0;
}

/*
- Sets in which screen should be drawn
## top: boolean
## bottom: boolean
### Drawable:setScreens
*/
static int l_Graphics_Drawable_setScreens(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    bool top = lua_toboolean(L, 2);
    bool bottom = lua_toboolean(L, 3);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->drawTop = top;
    obj->drawBottom = bottom;
    return 0;
}

/*
- Sets the position of the object in screen
## x: integer
## y: integer
### Drawable:setPosition
*/
static int l_Graphics_Drawable_setPosition(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    short x = luaL_checknumber(L, 2);
    short y = luaL_checknumber(L, 3);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->x = x;
    obj->y = y;
    return 0;
}

/*
- Set the main color of the object
## color: integer
### Drawable:setColor
*/
static int l_Graphics_Drawable_setColor(lua_State* L) {
    Core::Drawable* obj = *(Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    u32 color = luaL_checknumber(L, 2);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->color = color;
    return 0;
}

/*
- Destroys the reference. Do NOT use the object after calling this
### Drawable:destroy
*/
static int l_Graphics_Drawable_gc(lua_State* L) {
    Core::Drawable** obj = (Core::Drawable**)LuaObject::CheckObject(L, 1, "Drawable");
    if (*obj != nullptr) {
        Core::GraphicsManager& ins = Core::GraphicsManager::getInstance();
        ins.removeObject(*obj);
        delete *obj;
        *obj = nullptr;
    }
    return 0;
}

/*
- Sets if the rect should be filled
## filled: boolean
### GRect:setFilled
*/
static int l_Graphics_GRect_setFilled(lua_State* L) {
    Core::Rect* obj = *(Core::Rect**)LuaObject::CheckObject(L, 1, "GRect");
    bool filled = lua_toboolean(L, 2);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->filled = filled;
    return 0;
}

/*
- Set the width and height of the rect
## width: integer
## height: integer
### GRect:setSize
*/
static int l_Graphics_GRect_setSize(lua_State* L) {
    Core::Rect* obj = *(Core::Rect**)LuaObject::CheckObject(L, 1, "GRect");
    short width = luaL_checknumber(L, 2);
    short height = luaL_checknumber(L, 3);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->width = width;
    obj->height = height;
    return 0;
}

/*
- Set if the label should use SystemFont. Enabling it will disable background
## useSystemFont: boolean
### GLabel:setSystemFont
*/
static int l_Graphics_GLabel_setSystemFont(lua_State* L) {
    Core::Label* obj = *(Core::Label**)LuaObject::CheckObject(L, 1, "GLabel");
    bool useSystemFont = lua_toboolean(L, 2);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->useSystemFont = useSystemFont;
    return 0;
}

/*
- Set the bg color that will be used if the label doesn't use system font
## color: integer
### GLabel:setBgColor
*/
static int l_Graphics_GLabel_setBgColor(lua_State* L) {
    Core::Label* obj = *(Core::Label**)LuaObject::CheckObject(L, 1, "GLabel");
    u32 color = luaL_checknumber(L, 2);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->setBgColor(color);
    return 0;
}

/*
- Set the label text
## text: string
### GLabel:setText
*/
static int l_Graphics_GLabel_setText(lua_State* L) {
    Core::Label* obj = *(Core::Label**)LuaObject::CheckObject(L, 1, "GLabel");
    const char* text = luaL_checkstring(L, 2);
    if (obj == nullptr)
        return ERROR_OBJECT_DESTROYED(L);

    obj->setText(text);
    return 0;
}

static const LuaObjectField DrawableFields[] = {
    {"setVisible", OBJF_TYPE_METHOD, (u32)l_Graphics_Drawable_setVisible},
    {"setScreens", OBJF_TYPE_METHOD, (u32)l_Graphics_Drawable_setScreens},
    {"setPosition", OBJF_TYPE_METHOD, (u32)l_Graphics_Drawable_setPosition},
    {"setColor", OBJF_TYPE_METHOD, (u32)l_Graphics_Drawable_setColor},
    {"destroy", OBJF_TYPE_METHOD, (u32)l_Graphics_Drawable_gc},
    {NULL, OBJF_TYPE_NIL, 0}
};

static const LuaObjectField GRectFields[] = {
    {"setFilled", OBJF_TYPE_METHOD, (u32)l_Graphics_GRect_setFilled},
    {"setSize", OBJF_TYPE_METHOD, (u32)l_Graphics_GRect_setSize},
    {NULL, OBJF_TYPE_NIL, 0}
};

static const LuaObjectField GLabelFields[] = {
    {"setSystemFont", OBJF_TYPE_METHOD, (u32)l_Graphics_GLabel_setSystemFont},
    {"setBgColor", OBJF_TYPE_METHOD, (u32)l_Graphics_GLabel_setBgColor},
    {"setText", OBJF_TYPE_METHOD, (u32)l_Graphics_GLabel_setText},
    {NULL, OBJF_TYPE_NIL, 0}
};

// ----------------------------------------------------------------------------

bool Core::Module::RegisterGraphicsModule(lua_State *L)
{
    LuaObject::RegisterNewObject(L, "Drawable", DrawableFields);

    LuaObject::RegisterNewObject(L, "GRect", GRectFields);
    LuaObject::SetParent("GRect", "Drawable");
    LuaObject::SetGCObjectField(L, "GRect", l_Graphics_Drawable_gc);

    LuaObject::RegisterNewObject(L, "GLabel", GLabelFields);
    LuaObject::SetParent("GLabel", "Drawable");
    LuaObject::SetGCObjectField(L, "GLabel", l_Graphics_Drawable_gc);

    lua_getglobal(L, "Core");
    luaC_register_field(L, graphics_functions, "Graphics");
    lua_getfield(L, -1, "Graphics");
    //$@@@Core.Graphics.OnNewFrame: EventClass
    Event::NewEvent(L, "OnNewFrame");
    lua_pop(L, 1);
    return true;
}