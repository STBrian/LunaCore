#pragma once

#include "game/types.h"

// If nostdlib, you must provide a minimal libc
#include <string.h>

#define GameMemalloc ((void*(*)(size_t))(0x11493c))
#define GameFree ((void(*)(void*))(0x1007D0|1))

inline size_t GameMemsize(void* p) {
    return *((u32*)p - 3);
}

inline void* GameCalloc(size_t nitems, size_t size) {
    if (nitems > 0 && size > 0) {
        if (void* dstPtr = GameMemalloc(nitems * size)) {
            memset(dstPtr, 0, nitems * size);
            return dstPtr;
        }
    }
    return NULL;
}

inline void* GameRealloc(void* p, size_t newsize) {
    if (p) {
        size_t oldsize = GameMemsize(p);

        if (oldsize == newsize)
            return p;

        if (void* newp = GameMemalloc(newsize)) {
            memcpy(newp, p, newsize < oldsize ? newsize : oldsize);
            GameFree(p);
            return newp;
        }
    }
    return NULL;
}

#ifdef NOSTDLIB_BUILD
    #ifdef __cplusplus
    extern "C" {
    #endif

    void* malloc(size_t size);
    void* calloc(size_t nitems, size_t size);
    void* realloc(void* p, size_t newsize);
    void free(void* p);

    #ifdef __cplusplus
    }
    #endif
#endif

#ifdef __cplusplus
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
}

#endif