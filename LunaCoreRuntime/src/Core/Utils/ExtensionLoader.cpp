#include "Core/Utils/ExtensionLoader.hpp"

#include <CTRPluginFramework.hpp>
#include <FsLib/fslib.hpp>

#include "Core/Debug.hpp"
#include "Core/Filesystem.hpp"

#include "lua_utils.hpp"
#include <stdlib.h>
#include <new>

namespace CTRPF = CTRPluginFramework;

extern u32 __start__;

static int l_loadExtension(lua_State* L) {
    const char* filename = lua_tostring(L, 1);

    {
    fslib::File extfile;
    extfile.open(path_from_string(filename), FS_OPEN_READ);
    if (!extfile.is_open())
        LUAUTILS_ERRORF(L, "Failed to load \"%s\"", filename);
    size_t size = extfile.get_size();
    char* buffer = reinterpret_cast<char*>(malloc(size));
    extfile.read(buffer, size);
    extfile.close();
    Core::Debug::Message(CTRPF::Utils::Format("Extension loaded at: %08X", buffer));
    u32* got_start = (u32*)(*(u32*)((u32)buffer + 0x10) + (u32)buffer);
    u32* got_end = (u32*)(*(u32*)((u32)buffer + 0x14) + (u32)buffer);
    if (got_start != got_end) {
        while (got_start < got_end) {
            if (*got_start < (u32)&__start__)
                *got_start = *got_start + (u32)buffer;
            got_start++;
        }
    }
    return reinterpret_cast<int(*)(lua_State*)>(buffer + 0x18)(L);
    }

    LUAUTILS_SET_ERROR_HANDLER(L);
}

namespace Core {
    bool RegisterExtensionLoader(lua_State* L) {
        lua_getglobal(L, "Core");
        lua_pushcfunction(L, l_loadExtension);
        lua_setfield(L, -2, "loadExtension");
        lua_pop(L, 1);
        return true;
    }
}