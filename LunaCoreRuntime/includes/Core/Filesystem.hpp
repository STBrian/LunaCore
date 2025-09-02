#pragma once

#include <FsLib/fslib.hpp>

#include "CoreGlobals.hpp"
#include "lua_common.h"

namespace Core {
    namespace Module {
        bool RegisterFilesystemModule(lua_State *L);
    }
}

fslib::Path path_from_string(const STRING_CLASS& str);

STRING_CLASS path_to_string(const std::u16string &path);