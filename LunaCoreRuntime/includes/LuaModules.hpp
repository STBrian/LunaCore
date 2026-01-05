#pragma once

#include "lua_common.h"

namespace Core {
    void LoadModules(lua_State *L);
}

namespace Core::Module {
    bool RegisterLocalPlayerModule(lua_State *L);

    namespace LocalPlayer {
        bool RegisterCameraModule(lua_State *L);

        bool RegisterInventoryModule(lua_State *L);
    }

    bool RegisterItemsModule(lua_State *L);

    bool RegisterEntityModule(lua_State *L);

    bool RegisterGamepadModule(lua_State *L);

    bool RegisterRecipesModule(lua_State *L);

    bool RegisterWorldModule(lua_State *L);
}