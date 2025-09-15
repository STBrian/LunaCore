#pragma once

#include <FsLib/fslib.hpp>

typedef struct {
    fslib::File *filePtr;
    int mode;
    size_t size;
} FilesystemFile;