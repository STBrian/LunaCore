#pragma once

#include "CoreGlobals.hpp"
#include "lua_common.h"

namespace Core {
    void InitCore();

    void LoadLuaEnv();

    bool LoadBuffer(const char *buffer, size_t size, const char* name);

    bool LoadScript(const STRING_CLASS& fp);

    void PreloadScripts();

    void LoadMods();
}