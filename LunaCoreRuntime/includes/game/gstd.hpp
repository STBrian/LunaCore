#pragma once

#include "game/gstd/gstd_functions.h"

namespace gstd {
    inline void* malloc(size_t n) {
        return GameMemalloc(n);
    }

    inline void* calloc(size_t nitems, size_t size) {
        return GameCalloc(nitems, size);
    }

    inline void* realloc(void* p, size_t newsize) {
        return GameRealloc(p, newsize);
    }

    inline void free(void* p) {
        GameFree(p);
    }

    inline size_t strlen(const char* s) {
        return GameStrlen(s);
    }

    /*inline void memcpy(void* dst, const void* src, size_t size) {
        GameMemcpy(dst, src, size);
    }*/

    inline int vsnprintf(char* buffer, size_t size, const char* fmt, va_list vals) {
        return GameVsnprintf(buffer, size, fmt, vals);
    }
}