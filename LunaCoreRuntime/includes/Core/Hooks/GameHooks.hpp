#pragma once

#include "types.h"

#ifndef __cplusplus
#define _ASSERT(cond, msg) \
    _Static_assert(cond, msg)

#else
#define _ASSERT(cond, msg) \
    static_assert(cond, msg)

#endif

typedef struct alignas(4) {
    u32 r[13];
    u32 sp, lr;
    u32 reserved;
    float s[32];

    // -------------------
    // Modifying here is fine
    // -------------------

    u32 returnIns[12];
    u32 returnData[2];

    // -------------------
    // Modifying here is fine
    // -------------------
    u32 callbackAddress;

    u32 targetAddress;
    u32 selfHookPtr;
    u32 preHookBody[11];
    u32 preHookData[2];

    #ifdef __cplusplus
    double* d(int i) {
        return reinterpret_cast<double*>(&s[i*2]);
    }
    #endif
} CoreHookContext;

_ASSERT(sizeof(CoreHookContext), "hi");
_ASSERT(offsetof(CoreHookContext, s) % 8 == 0, "s is not aligned");

#ifdef __cplusplus
extern "C" {
#endif

void* hookFunction(u32 targetAddr, u32 callbackAddr);

void hookSomeFunctions();

void hookReturnOverride(CoreHookContext *ctx, u32 data);

#ifdef __cplusplus
}
#endif