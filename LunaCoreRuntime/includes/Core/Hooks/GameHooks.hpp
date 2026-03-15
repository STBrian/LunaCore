#pragma once

#include "types.h"

#ifndef LEGACY_HOOKS
typedef struct alignas(4) {
    u32 r[13];
    u32 sp, lr;

    u32 returnIns[10];
    u32 returnData[2];

    u32 targetAddress;
    u32 selfHookPtr;
    u32 preHookBody[9];
    u32 preHookData[2];
} CoreHookContext;
#endif

#ifdef LEGACY_HOOKS
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
#endif

void hookFunction(u32 targetAddr, u32 callbackAddr);

void hookSomeFunctions();

void hookReturnOverwrite(CoreHookContext *ctx, u32 data);