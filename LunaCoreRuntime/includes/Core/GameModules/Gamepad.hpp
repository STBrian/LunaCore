#pragma once

#include "lua_common.h"

typedef uint32_t u32;

namespace Core {
    namespace Module {
        bool RegisterGamepadModule(lua_State *L);
    }
}