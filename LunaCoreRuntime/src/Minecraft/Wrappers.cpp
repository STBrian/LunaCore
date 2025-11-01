#include "Minecraft/game_utils/game_functions.hpp"

extern "C" __attribute__((naked)) size_t __wrap_strlen(const char* str) {
    return GameStrlen(str);
}

extern "C" __attribute__((naked)) int __wrap_snprintf(char* str, unsigned int size, const char* fmt, ...) {
    asm volatile (
        "ldr r12, =0x1000c1\n"
        "bx r12\n"
    );
}