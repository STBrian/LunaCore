#include "game/gstd/gstd_functions.h"

extern "C" NAKED size_t __wrap_strlen(const char* str) {
    return GameStrlen(str);
}

extern "C" NAKED void __wrap_memcpy(void* dst, void* src, size_t size) {
    GameMemcpy(dst, src, size);
}

extern "C" NAKED int __wrap_snprintf(char* str, unsigned int size, const char* fmt, ...) {
    asm volatile (
        "ldr r12, =0x1000c1\n"
        "bx r12\n"
    );
}

extern "C" NAKED int __wrap_vsnprintf(char *buffer, size_t size, const char *fmt, va_list vals) {
    return GameVsnprintf(buffer, size, fmt, vals);
}