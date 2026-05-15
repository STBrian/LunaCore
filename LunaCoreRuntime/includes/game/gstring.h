#pragma once

#include "./types.h"

#define _GameStrlenPTR ((size_t(*)(const char*))(0x2fe990))

#define GameStrlen(s) _GameStrlenPTR(s)

#define gstd_strlen(s) GameStrlen(s)
/* dst, src, size */
//#define GameMemcpy ((void(*)(void*, const void*, size_t))(0x12CE5C)) // unstable

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NOSTDLIB_BUILD
    size_t strlen(const char* s);

    #ifdef GSTDLIB_MAIN
    size_t strlen(const char* s) {
        return gstd_strlen(s);
    }
    #endif
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace gstd {
    inline size_t strlen(const char* s) {
        return gstd_strlen(s);
    }

    /*inline void memcpy(void* dst, const void* src, size_t size) {
        GameMemcpy(dst, src, size);
    }*/
}
#endif