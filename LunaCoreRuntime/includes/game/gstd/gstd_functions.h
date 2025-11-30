#pragma once

/**
 * Some functions are based on
 * https://github.com/rairai6895/MC3DSPluginFramework/blob/main/Sources/gstd.cpp
 */

// If nostdlib, you must provide a minimal libc
#include <string.h>
#include <stdarg.h>
#include "game/types.h"

#define GameStrlen ((size_t(*)(const char*))(0x2fe990))
/* dst, src, size */
//#define GameMemcpy ((void(*)(void*, const void*, size_t))(0x12CE5C)) // unstable

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

#define GameSnprintf ((int(*)(char*,size_t,char*,...))(0x1000c0|1))
#define GameVsnprintf ((int(*)(char*,size_t,const char*,va_list))(0x100120|1))