#pragma once

#include "lua_common.h"

#include "Helpers/Timer.hpp"
#include "Core/Scheduler.hpp"

namespace Core {
    bool RegisterAsyncModule(lua_State *L);

    class AsyncWaitHandler : public SchedulerWaitHandler {
        private:
        Timer _timer;

        public:
        AsyncWaitHandler(float sec) : _timer(CTRPluginFramework::Seconds(sec)) {}

        bool WaitIsReady() override {
            return _timer.Expired();
        }

        int PushValues(lua_State* T) override {
            lua_pushboolean(T, true);
            return 1;
        }
    };
}