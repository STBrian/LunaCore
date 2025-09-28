#include "Core/Event.hpp"

#include <atomic>
#include <mutex>
#include <vector>

#include <CTRPluginFramework.hpp>

#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Game/Gamepad.hpp"
#include "Core/Utils/Utils.hpp"
#include "CoreGlobals.hpp"
#include "Core/Utils/GameState.hpp"

static inline void core_newevent(lua_State* L, const char* eventname) {
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "listeners");
    luaC_setmetatable(L, "EventClass");
    lua_setfield(L, -2, eventname);
}

namespace CTRPF = CTRPluginFramework;

CTRPF::Clock timeoutEventClock;
extern std::atomic<bool> graphicsIsTop;
extern GameState_s GameState;

void Core::EventRestartClock() {
    timeoutEventClock.Restart();
}

void TimeoutEventHook(lua_State *L, lua_Debug *ar)
{
    if (timeoutEventClock.HasTimePassed(CTRPluginFramework::Milliseconds(5000)))
        luaL_error(L, "Event listener exceeded execution time (5000 ms)");
}

void Core::Event::NewEvent(lua_State* L, const char* eventName) {
    core_newevent(L, eventName);
}

void Core::Event::TriggerEvent(lua_State* L, const std::string& eventName, unsigned int nargs) {
    int baseIdx = lua_gettop(L) - nargs;
    int argsIdx = baseIdx + 1;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_EVENT;

    std::vector<std::string> path;
    size_t start = 0;
    size_t pos = 0;

    while ((pos = eventName.find('.', start)) != std::string::npos) {
        path.push_back(eventName.substr(start, pos - start));
        start = pos + 1;
    }
    path.push_back(eventName.substr(start));

    lua_getglobal(L, path[0].c_str());
    for (int i = 1; i < path.size(); i++) {
        if (lua_istable(L, -1))
            lua_getfield(L, -1, path[i].c_str());
    }

    if (!lua_istable(L, -1)) {
        Core::Debug::LogError(CTRPF::Utils::Format("Core internal error: Tried to trigger event '%s' with parent of type '%s", eventName.c_str(), luaL_typename(L, -1)));
        lua_settop(L, baseIdx);
        return;
    }
    lua_getfield(L, -1, "Trigger");

    if (lua_isfunction(L, -1))
    {
        lua_pushvalue(L, -2);
        for (int i = 0; i < nargs; i++)
            lua_pushvalue(L, argsIdx + i);
        if (lua_pcall(L, 1 + nargs, 0, 0))
            Core::Debug::LogError(eventName + " error: " + std::string(lua_tostring(L, -1)));
    }
    else
        Core::Debug::LogError(eventName + ":Trigger error. Unexpected type");

    lua_settop(L, baseIdx);
}

void Core::EventHandlerCallback()
{
    std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
    lua_State *L = Lua_global;

    // KeyPressed Event
    u32 pressedKeys = CTRPF::Controller::GetKeysPressed();
    u32 downKeys = CTRPF::Controller::GetKeysDown();
    u32 releasedKeys = CTRPF::Controller::GetKeysReleased();
    if (pressedKeys > 0) {
        lua_pushnumber(L, pressedKeys);
        Core::Event::TriggerEvent(L, "Game.Gamepad.OnKeyPressed", 1);
    }

    if (downKeys > 0) {
        lua_pushnumber(L, downKeys);
        Core::Event::TriggerEvent(L, "Game.Gamepad.OnKeyDown", 1);
    }

    if (releasedKeys > 0) {
        lua_pushnumber(L, releasedKeys);
        Core::Event::TriggerEvent(L, "Game.Gamepad.OnKeyReleased", 1);
    }

    /*static float lastSlider = 0;
    float slider = osGet3DSliderState();
    if (lastSlider != slider) {
        CTRPF::OSD::Notify(CTRPF::Utils::Format("%f", slider));
        lastSlider = slider;
    }*/
}

// ----------------------------------------------------------------------------

//$Core.Event

// ----------------------------------------------------------------------------

//@@EventClass

/*
- Adds a function to call when this events fires. It also returns the function
## func: function
## return: function
### EventClass:Connect
*/
static int l_Event_BaseEvent_Connect(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_getfield(L, 1, "listeners");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, 1, "listeners");
    }

    int index = lua_objlen(L, -1) + 1;
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, index);
    
    lua_pop(L, 1);
    lua_pushvalue(L, 2);
    return 1;
}

/*
- Removes a listener previously connected to this event
## func: function
### EventClass:Disconnect
*/

/*
- Fire this event
### EventClass:Trigger
*/
static int l_Event_BaseEvent_Trigger(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int argc = lua_gettop(L) - 1;

    lua_getfield(L, 1, "listeners");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return luaL_error(L, "'listeners' is not a table");
    }
    int listenersIdx = lua_gettop(L);
    int len = lua_objlen(L, listenersIdx);

    for (int i = len; i >= 1; --i) {
        lua_rawgeti(L, listenersIdx, i);
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            continue;
        }

        for (int j = 0; j < argc; ++j) 
            lua_pushvalue(L, 2 + j);

        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        lua_remove(L, -2);

        int errfunc = lua_gettop(L);

        lua_insert(L, errfunc - (argc + 1));

        lua_sethook(L, TimeoutEventHook, LUA_MASKCOUNT, 100); // Timeout hook
        timeoutEventClock.Restart();
        
        if (lua_pcall(L, argc, 0, errfunc - (argc + 1))) {
            std::string errMsg(lua_tostring(L, -1));
            Core::Utils::Replace(errMsg, "\t", "    ");
            Core::Debug::LogError("Event error: "+errMsg);
            lua_pop(L, 1);
            lua_remove(L, errfunc - (argc + 1));

            // Remove listener with table.remove
            lua_getglobal(L, "table");
            lua_getfield(L, -1, "remove");
            lua_remove(L, -2);
            lua_pushvalue(L, listenersIdx);
            lua_pushinteger(L, i);
            if (lua_pcall(L, 2, 1, 0))
                Core::Debug::LogError(CTRPF::Utils::Format("Core::Event::BaseEvent::Trigger error: %s", lua_tostring(L, -1)));
            lua_pop(L, 1); // Remove either error string or returned value
            lua_gc(L, LUA_GCCOLLECT, 0);
        } else {
            lua_remove(L, errfunc - (argc + 1));
        }

        lua_sethook(L, nullptr, 0, 0); // Disable after use
    }
    lua_pop(L, 1); // Pop listeners
    return 0;
}

// ----------------------------------------------------------------------------

bool Core::Module::RegisterEventModule(lua_State *L)
{
    lua_getglobal(L, "Core");
    lua_newtable(L); // Core.Event

    //$@@@Core.Event.BaseEvent: EventClass
    lua_newtable(L);
    lua_pushcfunction(L, l_Event_BaseEvent_Connect);
    lua_setfield(L, -2, "Connect");
    lua_pushcfunction(L, l_Event_BaseEvent_Trigger);
    lua_setfield(L, -2, "Trigger");
    lua_newtable(L);
    lua_setfield(L, -2, "listeners");
    lua_setfield(L, -2, "BaseEvent");

    lua_setfield(L, -2, "Event");
    lua_pop(L, 1);

    luaL_newmetatable(L, "EventClass");
    int metatableIdx = lua_gettop(L);
    lua_pushcfunction(L, luaC_invalid_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_getglobal(L,"Core");
    lua_getfield(L, -1, "Event");
    lua_getfield(L, -1, "BaseEvent");
    lua_setfield(L, metatableIdx, "__index");
    lua_pop(L, 3);
    
    lua_getglobal(L, "Core");
    lua_getfield(L, -1, "Event");
  
    //$@@@Core.Event.OnGameEntitySpawnStart: EventClass
    //core_newevent(L, "OnGameEntitySpawnStart");
  
    //$@@@Core.Event.OnGameEntitySpawn: EventClass
    //core_newevent(L, "OnGameEntitySpawn");

    lua_pop(L, 2);

    const char *lua_Code = R"(
        function Core.Event.BaseEvent:Disconnect(f)
            for i, v in ipairs(self.listeners) do
                if v == f then
                    table.remove(self.listeners, i)
                end
            end
        end

        Core.Event.BaseEvent = readOnlyTable(Core.Event.BaseEvent, "BaseEvent")
    )";
    if (luaL_dostring(L, lua_Code))
    {
        Core::Debug::LogError("Core::Event::Load error: " + std::string(lua_tostring(L, -1)));
        lua_pop(L, 1);
        return false;
    }
    return true;
}