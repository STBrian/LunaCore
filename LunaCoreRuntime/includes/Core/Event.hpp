#pragma once

#include "CoreGlobals.hpp"
#include "lua_common.h"

namespace Core {
    void EventRestartClock();

    void EventHandlerCallback();

    namespace Event {
        void TriggerEvent(lua_State* L, const std::string& eventName, unsigned int nargs = 0);
    }

    namespace Module {
        bool RegisterEventModule(lua_State *L);
    }
}
