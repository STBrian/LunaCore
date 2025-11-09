#pragma once

#include "lua_common.h"

#include <CTRPluginFramework.hpp>

namespace Core {

    void GraphicsHandlerCallback();

    namespace Module {
        bool RegisterGraphicsModule(lua_State *L);
    }
}