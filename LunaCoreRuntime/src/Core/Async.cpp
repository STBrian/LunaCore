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

static void TimeoutAsyncHook(lua_State *L, lua_Debug *ar) {
    if (timeoutAsyncTimer.Expired())
        luaL_error(L, "Async coroutine exceeded execution time (5000 ms)");
}

// ----------------------------------------------------------------------------

//$Async

// ----------------------------------------------------------------------------

/*
- Adds the function to the queue that will run apart from the game until the functions ends
## func: function
### Async.create
*/
static int l_Async_create(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_State *co = lua_newthread(L);
    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);

    lua_sethook(co, TimeoutAsyncHook, LUA_MASKCOUNT, 100);

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

bool Core::RegisterAsyncModule(lua_State *L)
{
    lua_newtable(L);
    lua_pushcfunction(L, l_Async_create);
    lua_setfield(L, -2, "create");
    lua_pushcfunction(L, l_Async_wait);
    lua_setfield(L, -2, "wait");
    lua_setglobal(L, "Async");
    return true;
}