#include "Core/Graphics/Graphics.hpp"

#include <string>
#include <mutex>

#include "lua_object.hpp"
#include "CoreGlobals.hpp"
#include "Core/Event.hpp"
#include "Core/Async.hpp"

#include "Core/Graphics/GraphicsManager.hpp"

namespace CTRPF = CTRPluginFramework;

void Core::GraphicsHandlerCallback() {
    std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
    Event::TriggerEvent(Lua_global, "Core.Graphics.OnNewFrame");
}

// ----------------------------------------------------------------------------

//$Core.Graphics
//@@Drawable
//$@@@GRect: Drawable
//$@@@GLabel: Drawable

// ----------------------------------------------------------------------------

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
### GRect:destroy
*/
static int l_Graphics_GRect_gc(lua_State* L) {
    Core::Rect** obj = (Core::Rect**)LuaObject::CheckObject(L, 1, "GRect");
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
- Destroys the reference. Do NOT use the object after calling this
### GLabel:destroy
*/
static int l_Graphics_GLabel_gc(lua_State* L) {
    Core::Label** obj = (Core::Label**)LuaObject::CheckObject(L, 1, "GLabel");
    if (*obj != nullptr) {
        Core::GraphicsManager& ins = Core::GraphicsManager::getInstance();
        ins.removeObject(*obj);
        delete *obj;
        *obj = nullptr;
    }
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
    {NULL, OBJF_TYPE_NIL, 0}
};

static const LuaObjectField GRectFields[] = {
    {"destroy", OBJF_TYPE_METHOD, (u32)l_Graphics_GRect_gc},
    {"setFilled", OBJF_TYPE_METHOD, (u32)l_Graphics_GRect_setFilled},
    {"setSize", OBJF_TYPE_METHOD, (u32)l_Graphics_GRect_setSize},
    {NULL, OBJF_TYPE_NIL, 0}
};

static const LuaObjectField GLabelFields[] = {
    {"destroy", OBJF_TYPE_METHOD, (u32)l_Graphics_GLabel_gc},
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
    LuaObject::SetGCObjectField(L, "GRect", l_Graphics_GRect_gc);

    LuaObject::RegisterNewObject(L, "GLabel", GLabelFields);
    LuaObject::SetParent("GLabel", "Drawable");
    LuaObject::SetGCObjectField(L, "GLabel", l_Graphics_GLabel_gc);

    lua_getglobal(L, "Core");
    luaC_register_field(L, graphics_functions, "Graphics");
    lua_getfield(L, -1, "Graphics");
    //$@@@Core.Graphics.OnNewFrame: EventClass
    Event::NewEvent(L, "OnNewFrame");
    lua_pop(L, 1);
    return true;
}