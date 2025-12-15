#pragma once

#include <stdarg.h>
#include "game/types.h"

#define GameSnprintf ((int(*)(char*,size_t,char*,...))(0x1000c0|1))
#define GameVsnprintf ((int(*)(char*,size_t,const char*,va_list))(0x100120|1))

#ifdef NOSTDLIB_BUILD
    #ifdef __cplusplus
    extern "C" {
    #endif

    int snprintf(char* str, unsigned int size, const char* fmt, ...);
    int vsnprintf(char* buffer, size_t size, const char* fmt, va_list vals);
    int sprintf(char* str, const char* format, ...);

    #ifdef __cplusplus
    }
    #endif
#endif

#ifdef __cplusplus
namespace gstd {
    NAKED inline int snprintf(char* str, unsigned int size, const char* fmt, ...) {
        asm volatile (
            "ldr r12, =0x1000c1\n"
            "bx r12\n"
        );
    }

    inline int vsnprintf(char* buffer, size_t size, const char* fmt, va_list vals) {
        return GameVsnprintf(buffer, size, fmt, vals);
    }
}

#endif