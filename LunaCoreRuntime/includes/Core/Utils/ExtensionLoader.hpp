#pragma once

#include "lua_common.h"

namespace Core {
    typedef int (*ModuleEntryFun)(lua_State*);

    ModuleEntryFun LoadExtension(const char* filename);

    bool RegisterExtensionLoader(lua_State* L);
}

extern "C" {
    void Core_ExportSymbol(const char* symName, void* symAddr);

    void* Core_GetSymbol(const char* symName);
}