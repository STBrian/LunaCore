#pragma once

#include "lua_common.h"

#include <CTRPluginFramework.hpp>

namespace Core {
    namespace Module {
        bool RegisterGraphicsModule(lua_State *L);
    }
}