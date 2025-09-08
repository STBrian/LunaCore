#pragma once

#include <FsLib/fslib.hpp>

#include "CoreGlobals.hpp"
#include "lua_common.h"

namespace Core {
    namespace Module {
        bool RegisterFilesystemModule(lua_State *L);
    }
}

typedef struct {
    fslib::File *filePtr;
    int mode;
    size_t size;
} FilesystemFile;

namespace Filesystem {
    FilesystemFile* fopen(const char* filename, const char* mode);

    void fclose(FilesystemFile* file);

    size_t fseek(FilesystemFile* file, long off, int ori);

    size_t ftell(FilesystemFile* file);

    void rewind(FilesystemFile* file);

    size_t fwrite(const void* buffer, size_t size, size_t n, FilesystemFile* file);

    size_t fread(void* buffer, size_t size, size_t n, FilesystemFile* file);
}

fslib::Path path_from_string(const STRING_CLASS& str);

STRING_CLASS path_to_string(const std::u16string &path);