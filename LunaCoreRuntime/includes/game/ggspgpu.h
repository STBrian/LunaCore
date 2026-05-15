#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "types.h"
#include <3ds/svc.h>
#include <3ds/services/gspgpu.h>
#include <3ds/ipc.h>
#include <3ds/result.h>

#define _GSPGPU_FlushDataCachePTR ((Result(*)(const void*, u32))0x4C806C)
#define _gspSubmitGxCommandPTR ((Result(*)(u32, u32*))0x004c7e4c)
#define _GSPGPU_ReadHWRegsPTR ((Result(*)(u32, u32*, u8))0x004c8890)
#define _GSPGPU_WriteHWRegsPTR ((Result(*)(u32, const u32*, u8))0x004c890c)

#define _getGSPHandlePTR ((Handle*(*)(void))0x1241a8)
#define _gameGSPContextPTR 0x00a2fc40

inline void* ggspSharedMem() {
    return (void*)(_gameGSPContextPTR + 0x40);
}

inline u8* ggspThreadId() {
    return (u8*)(_gameGSPContextPTR + 0x74);
}

inline Handle gget_gspGpuHandle() {
    return *_getGSPHandlePTR();
}

inline Result gGSPGPU_FlushDataCache(const void* addr, u32 size) {
    return _GSPGPU_FlushDataCachePTR(addr, size);
}

inline Result ggspSubmitGxCommand(u32* gxCommand) {
    u32 tId = ((u32(*)(void))0x00124198)(); // I guess it's the gsp handler
    if (tId > 0)
        return _gspSubmitGxCommandPTR(tId + 0x58, gxCommand);
    else return -1;
}

static inline Result gGSPGPU_SetBufferSwap(u32 screenid, const GSPGPU_FramebufferInfo* framebufinfo) {
    u32* cmdbuf = getThreadCommandBuffer();
    
    cmdbuf[0] = IPC_MakeHeader(0x5, 8, 0);
    cmdbuf[1] = screenid;
    memcpy(&cmdbuf[2], framebufinfo, sizeof(GSPGPU_FramebufferInfo));

    Result ret = 0;
    if (R_FAILED(ret=svcSendSyncRequest(gget_gspGpuHandle()))) return ret;
    return cmdbuf[1];
}

static inline Result gGSPGPU_ReadHWRegs(u32 regAddr, u32* data, u8 size) {
    return _GSPGPU_ReadHWRegsPTR(regAddr, data, size);
}

static inline Result gGSPGPU_WriteHWRegs(u32 regAddr, const u32* data, u8 size) {
    return _GSPGPU_WriteHWRegsPTR(regAddr, data, size);
}

static inline void ggspWriteGxReg(u32 offset, u32 data) {
	gGSPGPU_WriteHWRegs(0x400000 + offset, &data, 4);
}

static inline void ggspReadGxReg(u32 offset, u32* data) {
	gGSPGPU_ReadHWRegs(0x400000 + offset, data, 4);
}

#ifdef __cplusplus
}
#endif