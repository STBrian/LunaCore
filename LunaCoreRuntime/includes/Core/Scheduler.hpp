#pragma once

#include <vector>
#include <unordered_map>
#include "lua_common.h"

namespace Core {
    class SchedulerWaitHandler {
        public:

        /* Returns true when the async task is done */
        virtual bool WaitIsReady() = 0;

        /* Push the results to the Lua stack and returns the number of args */
        virtual int PushValues(lua_State* T) = 0;

        virtual ~SchedulerWaitHandler() = default;
    };

    class Scheduler {
        public:

        struct PendingTask {
            lua_State* thread;
            SchedulerWaitHandler* handler;
        };

        static Scheduler& getInstance();

        static bool IsThread(lua_State* T) {
            int result = lua_pushthread(T);
            lua_pop(T, 1);
            return (result == 0);
        }

        /* Important: The handler will be deleted after resuming, do not delete it manually!
        Leave handler as nullptr to make a wait_once */
        int CreateWait(lua_State* T, SchedulerWaitHandler* handler) {
            PendingTask task;
            task.thread = T;
            task.handler = handler;
            pending.push_back(task);
            return lua_yield(T, 0);
        }

        static void Update();

        /* Adds a thread to the scheduler (it leaves the thread in the Lua stack).
         Automatically creates a new wait_once */
        void AddTask(lua_State* L, int idx) {
            lua_State* T = lua_tothread(L, idx);
            lua_pushvalue(L, idx);
            tasks[T] = luaL_ref(L, LUA_REGISTRYINDEX);

            PendingTask task;
            task.thread = T;
            task.handler = nullptr;
            pending.push_back(task);
        }

        void RemoveTask(lua_State* L, lua_State* T) {
            if (tasks.contains(T)) {
                luaL_unref(L, LUA_REGISTRYINDEX, tasks[T]);
                tasks.erase(T);
            }
        }

        bool IsValidTask(lua_State* T) {
            return (IsThread(T) && tasks.contains(T));
        }

        private:
        Scheduler() {}
        ~Scheduler() {}
        
        std::vector<PendingTask> pending;
        std::unordered_map<lua_State*, int> tasks;
    };
} // namespace Core
