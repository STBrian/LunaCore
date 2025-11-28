#pragma once

#include "game/gstd/gstd_functions.h"

namespace gstd {
    inline void* malloc(size_t n) {
        return GameMemalloc(n);
    }

    inline void free(void* p) {
        GameFree(p);
    }

    inline size_t strlen(const char* s) {
        return GameStrlen(s);
    }

    inline void* calloc(size_t nitems, size_t size) {
        return GameCalloc(nitems, size);
    }
}