#pragma once

#include <stdarg.h>
#include "./types.h"

#define _GameSnprintfPTR ((int(*)(char*,size_t,char*,...))(0x1000c0|1))
#define _GameVsnprintfPTR ((int(*)(char*,size_t,const char*,va_list))(0x100120|1))

#define GameSnprintf(buffer, size, fmt, ...) _GameSnprintfPTR(buffer, size, fmt, ##__VA_ARGS__)
#define GameVsnprintf(buffer, size, fmt, vals) _GameVsnprintfPTR(buffer, size, fmt, vals)

#define gstd_snprintf(buffer, size, fmt, ...) _GameSnprintfPTR(buffer, size, fmt, ##__VA_ARGS__)
#define gstd_vsnprintf(buffer, size, fmt, vals) GameVsnprintf(buffer, size, fmt, vals)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NOSTDLIB_BUILD
    int snprintf(char* str, unsigned int size, const char* fmt, ...);
    int vsnprintf(char* buffer, size_t size, const char* fmt, va_list vals);
    int sprintf(char* str, const char* format, ...);

    #ifdef GSTDLIB_MAIN
    NAKED int snprintf(char* str, unsigned int size, const char* fmt, ...) {
        asm volatile (
            "ldr r12, =%0\n"
            "bx r12\n"
            : : "i"(_GameSnprintfPTR)
        );
    }

    int vsnprintf(char* buffer, size_t size, const char* fmt, va_list vals) {
        return gstd_vsnprintf(buffer, size, fmt, vals);
    }

    int sprintf(char* str, const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        int ret = vsnprintf(str, (size_t)-1, format, ap);
        va_end(ap);
        return ret;
    }
    #endif
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace gstd {
    NAKED inline int snprintf(char* str, unsigned int size, const char* fmt, ...) {
        asm volatile (
            "ldr r12, =%0\n"
            "bx r12\n"
            : : "i"(_GameSnprintfPTR)
        );
    }

    inline int vsnprintf(char* buffer, size_t size, const char* fmt, va_list vals) {
        return gstd_vsnprintf(buffer, size, fmt, vals);
    }
}

#endif