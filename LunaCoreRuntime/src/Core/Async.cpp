#include "Core/Async.hpp"

#include <CTRPluginFramework.hpp>
#include <mutex>

#include "Core/Debug.hpp"
#include "Core/Utils/Utils.hpp"
#include "CoreGlobals.hpp"

#include "Helpers/Timer.hpp"

namespace CTRPF = CTRPluginFramework;

static Core::Timer timeoutAsyncTimer(CTRPF::Milliseconds(5000));

void Core::AsyncRestartClock() {
    timeoutAsyncTimer.Restart();
}

void Core::_TimeoutAsyncHook(lua_State *L, lua_Debug *ar) {
    if (timeoutAsyncTimer.Expired())
        luaL_error(L, "Async coroutine exceeded execution time (5000 ms)");
}

// ----------------------------------------------------------------------------

//$Async
//@@AsyncTask

// ----------------------------------------------------------------------------

/*
- Returns a new task with the given function that can be connected to game events
## func: function
## return: AsyncTask
### Async.create
*/
static int l_Async_create(lua_State *L)
{
    /* This only creates a reference to the function. The coroutine must be
    created when you need to use it. This also allows to call this task many times */
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_newtable(L);
    lua_pushvalue(L, 1); // save function reference
    lua_setfield(L, -2, "_taskF");
    luaC_setmetatable(L, "AsyncTask");
    return 1;
}

/*
- Adds the function to the queue that will run alongside the game until the functions ends
## func: function
### Async.run
*/
static int l_Async_run(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_State *co = lua_newthread(L);
    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);

    lua_sethook(co, Core::_TimeoutAsyncHook, LUA_MASKCOUNT, 100);

    Core::Scheduler::getInstance().AddTask(L, -1);
    lua_pop(L, 1); // Pop co
    return 0;
}

/*
- Yeilds the current task until time has passed. Always returns true
## seconds: number?
## return: boolean
### Async.wait
*/
static int l_Async_wait(lua_State *L) {
    if (!Core::Scheduler::getInstance().IsValidTask(L)) {
        return luaL_error(L, "use an async task to execute this function");
    }
    int nargs = lua_gettop(L);
    if (nargs > 0) {
        if (lua_isnumber(L, 1)) {
            float sec = lua_tonumber(L, 1);
            if (sec < 0)
                return luaL_error(L, "bad time value");
            else
                return Core::Scheduler::getInstance().CreateWait(L, new Core::AsyncWaitHandler(sec));
        } else {
            return luaL_error(L, "bad #1 argument type (expected number)");
        }
    } else {
        return Core::Scheduler::getInstance().CreateWait(L, new Core::AsyncWaitHandler(0));
    }
}

// ----------------------------------------------------------------------------

static inline void RegisterAsyncTaskMetatable(lua_State *L)
{
    luaL_newmetatable(L, "AsyncTask");
    lua_newtable(L);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaC_invalid_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushstring(L, "AsyncTask");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);
}

bool Core::RegisterAsyncModule(lua_State *L)
{
    RegisterAsyncTaskMetatable(L);

    lua_newtable(L);
    lua_pushcfunction(L, l_Async_run);
    lua_setfield(L, -2, "run");
    lua_pushcfunction(L, l_Async_create);
    lua_setfield(L, -2, "create");
    lua_pushcfunction(L, l_Async_wait);
    lua_setfield(L, -2, "wait");
    lua_setglobal(L, "Async");
    return true;
}