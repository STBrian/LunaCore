#include "Core/Utils/ExtensionLoader.hpp"

#include <CTRPluginFramework.hpp>

#include "Core/Debug.hpp"

#include <cstdlib>
#include <new>

namespace CTRPF = CTRPluginFramework;

static int l_loadExtension(lua_State* L) {
    const char* filename = lua_tostring(L, 1);
    CTRPF::File extfile;
    CTRPF::File::Open(extfile, filename, CTRPF::File::READ);
    if (!extfile.IsOpen())
        return luaL_error(L, "Failed to load '%s'", filename);
    size_t size = extfile.GetSize();
    char* buffer = reinterpret_cast<char*>(::operator new(size, std::align_val_t(8)));
    extfile.Read(buffer, size);
    extfile.Close();
    Core::Debug::LogMessage(CTRPF::Utils::Format("Extension loaded at: %08X", buffer), true);
    return reinterpret_cast<int(*)(lua_State*, Core::SharedFunctions*)>(buffer + 0x10)(L, &SharedFunctions_obj);
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