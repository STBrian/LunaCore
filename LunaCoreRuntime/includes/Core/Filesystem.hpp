#pragma once

#include "CoreGlobals.hpp"
#include "lua_common.h"
#include "Core/FilesystemFile.hpp"

namespace Core {
    namespace Module {
        bool RegisterFilesystemModule(lua_State *L);
    }
}

namespace Filesystem {
    FilesystemFile* fopen(const char* filename, const char* mode);

    void fclose(FilesystemFile* file);

    size_t fseek(FilesystemFile* file, long off, int ori);

    size_t ftell(FilesystemFile* file);

    void rewind(FilesystemFile* file);

    size_t fwrite(const void* buffer, size_t size, size_t n, FilesystemFile* file);

    size_t fread(void* buffer, size_t size, size_t n, FilesystemFile* file);
}

fslib::Path path_from_string(const std::string& str);

std::string path_to_string(const std::u16string &path);