#pragma once

#include "types.h"

#ifndef __cplusplus
#define _ASSERT(cond, msg) \
    _Static_assert(cond, msg)

#else
#define _ASSERT(cond, msg) \
    static_assert(cond, msg)

#endif

struct alignas(4) CoreHookContext {
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
};

_ASSERT(sizeof(CoreHookContext), "hi");
_ASSERT(offsetof(CoreHookContext, s) % 8 == 0, "s is not aligned");

#ifdef __cplusplus
extern "C" {
#endif

/*  A normal hook allows to read and modify registers value before executing the target instruction by
    passing a context to the callback function.

    It overrides a total of two instructions from the target address.

    A hook cannot override relative instructions (i.e. instructions that load data using the PC register),
    with the exception of BL and B instructions.

    Only hooks an ARM function.
*/
void* CoreCreateHook(u32 targetAddr, u32 callbackAddr);

void CoreHookReturnOverride(CoreHookContext *ctx, u32 data);

void CoreHooksInit();

/*  A light hook only allows to execute the callback at the desired address.
    If a light hook doesn't work properly, try using a normal hook. 

    It overrides LR so if a later instruction needs an unmodified LR value, it will lead to
    unexpected behavior.

    A light hook cannot override relative instructions (i.e. instructions that load data using the PC register),
    with the exception of BL instructions. 
    
    Only hooks an ARM function.
*/
void CoreCreateLightHook(u32 targetAddr, u32 callback, void* passValue);

void CoreCreateEventHook(u32 targetAddr, const char* event);

#ifdef __cplusplus
}
#endif