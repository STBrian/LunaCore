#pragma once

#include <string>
#include <map>
#include <any>

#include "lua_common.h"

namespace Core {
    void EventRestartClock();

    void EventHandlerCallback();

    namespace Event {
        void TriggerEvent(lua_State* L, const std::string& eventName);
        std::map<std::string, std::any> TriggerEvent(lua_State* L, const std::string& eventName, const std::map<std::string, std::any>& eventObject);
    }

    namespace Module {
        bool RegisterEventModule(lua_State *L);
    }
}