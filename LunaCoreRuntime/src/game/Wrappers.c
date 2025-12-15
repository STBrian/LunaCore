#include "game/gstdio.h"
#include "game/gstring.h"

NAKED size_t __wrap_strlen(const char* str) {
    return GameStrlen(str);
}

/*NAKED void __wrap_memcpy(void* dst, void* src, size_t size) {
    GameMemcpy(dst, src, size);
}*/

NAKED int __wrap_snprintf(char* str, unsigned int size, const char* fmt, ...) {
    asm volatile (
        "ldr r12, =0x1000c1\n"
        "bx r12\n"
    );
}

NAKED int __wrap_vsnprintf(char *buffer, size_t size, const char *fmt, va_list vals) {
    return GameVsnprintf(buffer, size, fmt, vals);
}

/*
void* __real_malloc(size_t s);

void* __wrap_malloc(size_t size) {
    void* p = __real_malloc(size);
}

void __real_free(void* p);

void* __wrap_free(void* p) {
    __real_free(p);
}*/ 