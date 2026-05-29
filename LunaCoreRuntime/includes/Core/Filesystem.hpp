#pragma once

#include "lua_common.h"
#include <FsLib/fslib.hpp>

fslib::Path path_from_string(const std::string& str);

std::string path_to_string(const std::u16string &path);