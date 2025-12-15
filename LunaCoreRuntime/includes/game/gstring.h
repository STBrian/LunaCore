#pragma once

#include "game/types.h"

#define GameStrlen ((size_t(*)(const char*))(0x2fe990))
/* dst, src, size */
//#define GameMemcpy ((void(*)(void*, const void*, size_t))(0x12CE5C)) // unstable

#ifdef NOSTDLIB_BUILD
    #ifdef __cplusplus
    extern "C" {
    #endif

    size_t strlen(const char* s);

    #ifdef __cplusplus
    }
    #endif
#endif

#ifdef __cplusplus
namespace gstd {
    inline size_t strlen(const char* s) {
        return GameStrlen(s);
    }

    /*inline void memcpy(void* dst, const void* src, size_t size) {
        GameMemcpy(dst, src, size);
    }*/
}

#endif