#include "Core/Scheduler.hpp"

#include <CTRPluginFramework.hpp>
#include <mutex>

#include "Core/Debug.hpp"
#include "Core/Async.hpp"

#include "CoreGlobals.hpp"
#include "Helpers/Timer.hpp"
#include "Helpers/Mutex.hpp"

using namespace Core;
namespace CTRPF = CTRPluginFramework;

Scheduler& Scheduler::getInstance() {
    static Scheduler s;
    return s;
}

/* Checks the thread status code. Prints the error to the log if any and/or removes the task if ended */
static inline void checkThreadStatus(Scheduler& ins, std::unordered_map<lua_State*, int>& tasks, lua_State* L, int call_result, lua_State* T) {
    if (call_result != 0 && call_result != LUA_YIELD) { // Ended with error
        const char *errMsg = lua_tostring(T, -1);
        
        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        lua_remove(L, -2);
        int coIdx = tasks[T];
        lua_rawgeti(L, LUA_REGISTRYINDEX, coIdx);
        lua_pushstring(L, errMsg);

        if (lua_pcall(L, 2, 1, 0)) {
            Core::Debug::ReportInternalError(lua_tostring(L, -1));
        } else {
            std::string traceback(lua_tostring(L, -1));
            Core::Debug::LogError("Async task error: " + traceback);
        }
        lua_pop(L, 1);
        ins.RemoveTask(L, T);
    } else if (call_result == 0) { // Ended successfully
        ins.RemoveTask(L, T);
    }
}

void Scheduler::Update() {
    std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
    Scheduler& ins = Scheduler::getInstance();

    for (int i = 0, count = ins.pending.size(); count > 0; count--) {
        auto task = ins.pending[i];

        if (task.handler && task.handler->WaitIsReady()) {
            lua_State* T = task.thread;
            int nresults = task.handler->PushValues(T);
            delete task.handler;
            Core::AsyncRestartClock();
            int call_result = lua_resume(T, nresults);
            checkThreadStatus(ins, ins.tasks, Lua_global, call_result, T);
            ins.pending.erase(ins.pending.begin() + i);
        } else if (!task.handler) {
            lua_State* T = task.thread;
            Core::AsyncRestartClock();
            int call_result = lua_resume(T, 0);
            checkThreadStatus(ins, ins.tasks, Lua_global, call_result, T);
            ins.pending.erase(ins.pending.begin() + i);
        } else {
            i++;
        }
    }
}