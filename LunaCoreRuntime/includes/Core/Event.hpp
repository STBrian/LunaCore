#pragma once

#include <string>
#include <vector>

#include "lua_common.h"

enum EventDataFieldType {
    EVENT_DATA_FLOAT,
    EVENT_DATA_INT,
    EVENT_DATA_BOOL,
    EVENT_DATA_STRING,
    EVENT_DATA_POINTER
};

struct EventDataField {
    const char* name;
    EventDataFieldType type;
    union {
        float* f;
        int* i;
        bool* b;
        const char** s;
        void** p;
    } ptr;
};

struct EventData {
    int field_count;
    EventDataField fields[8];
};

void l_Push_EventData(lua_State* L, const std::vector<EventDataField>& fields);

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