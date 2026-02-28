#include "Core/Utils/ExtensionLoader.hpp"

#include <CTRPluginFramework.hpp>
#include <FsLib/fslib.hpp>
#include <unordered_map>

#include "CoreGlobals.hpp"
#include "Core/Debug.hpp"
#include "Core/Filesystem.hpp"

#include "lua_utils.hpp"
#include <stdlib.h>

#include "Helpers/Allocation.hpp"

namespace CTRPF = CTRPluginFramework;
using namespace Core;

extern u32 __start__;

static std::unordered_map<std::string, void*> _sharedModulesSymbols;

#define MAKEVERSION(maj, min, pat) ((maj << 24)|(min << 16)|(pat << 8))

typedef struct {
    u32 magic;
    u32 lcrunver;
    u32 got_start, got_end;
} __moduleHeader;

namespace Core {
    ModuleEntryFun LoadExtension(const char* filename) {
        fslib::File extfile;
        extfile.open(path_from_string(filename), FS_OPEN_READ);
        if (!extfile.is_open())
            return nullptr;
        size_t fsize = extfile.get_size();
        auto buffer = UniqueAlloc::alloc_array_raw<char>(fsize);
        extfile.read(buffer.get(), fsize);
        extfile.close();
        Debug::Message(CTRPF::Utils::Format("Extension loaded at: %08X", buffer.get()));

        __moduleHeader* header = reinterpret_cast<__moduleHeader*>(buffer.get());
        if (header->lcrunver != MAKEVERSION(Core::Version.major, Core::Version.minor, Core::Version.patch))
            return nullptr;

        header->got_start += (u32)buffer.get(); // Adjust
        header->got_end += (u32)buffer.get(); // Adjust

        u32* got_start = reinterpret_cast<u32*>(header->got_start);
        u32* got_end = reinterpret_cast<u32*>(header->got_end);
        while (got_start < got_end) {
            if (*got_start < (u32)0x00100000)
                *got_start = *got_start + (u32)buffer.get();
            got_start++;
        }
        u32* entryPoint = reinterpret_cast<u32*>((u32)(buffer.release()) + sizeof(__moduleHeader));
        return reinterpret_cast<ModuleEntryFun>(entryPoint);
    }
}

USED void Core_ExportSymbol(const char* symName, void* symAddr) {
    _sharedModulesSymbols[symName] = symAddr;
}

USED void* Core_GetSymbol(const char* symName) {
    if (_sharedModulesSymbols.contains(symName))
        return _sharedModulesSymbols[symName];
    else
        return nullptr;
}

static int l_loadExtension(lua_State* L) {
    const char* filename = lua_tostring(L, 1);

    Core::ModuleEntryFun entryPoint;
    if ((entryPoint = Core::LoadExtension(filename)) == nullptr)
        luaL_error(L, "Failed to load \"%s\"", filename);
    else
        return entryPoint(L);

    return 0;
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