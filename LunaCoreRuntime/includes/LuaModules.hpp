#pragma once

#include "lua_common.h"

namespace Core {
    void LoadModules(lua_State *L);
}

namespace Core::Module {
    bool RegisterDebugModule(lua_State *L);

    bool RegisterSystemModule(lua_State *L);

    bool RegisterFilesystemModule(lua_State *L);

    bool RegisterMemoryModule(lua_State *L);

    bool RegisterKeyboardModule(lua_State *L);

    bool RegisterMenuModule(lua_State *L);

    bool RegisterResourcesModule(lua_State *L);

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