#pragma once

#include <cstdint>

typedef uint32_t u32;

typedef struct alignas(4) cHkCtx_s {
    u32 wrapCallbackAddress;
    u32 callbackAddress;
    u32 targetAddress;

    u32 r0, r1, r2, r3, lr, sp;

    u32 restoreIns;
    u32 overwrittenIns[5];
    u32 jmpIns;
    u32 returnAddress;
} CoreHookContext;

void hookFunction(u32 targetAddr, u32 callbackAddr);

void hookSomeFunctions();

__attribute((naked)) void hookReturnOverwrite(CoreHookContext *ctx, void *data);