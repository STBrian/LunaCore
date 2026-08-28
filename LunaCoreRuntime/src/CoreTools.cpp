#include "CoreTools.hpp"

#include <3ds.h>
#include <string.h>
#include <string>

#include <CTRPluginFramework.hpp>

#include "Core/Filesystem.hpp"


Result HBLDR_SetTarget(Handle hbldrHandle, const char* path);
Result HBLDR_SetArgv(Handle hbldrHandle, const void* buffer, u32 size);
namespace CTRPluginFramework::ProcessImpl {
    void UnlockGameThreads(void);
}

void LunaCore::LaunchTool(const char* name) {
    Handle hbldrHandle;
    const char basePath[] = "/Minecraft 3DS/tools/";
    std::string toolPath(std::string("sdmc:") + basePath + name + ".3dsx");
    if (Core::Filesystem::FileExists(toolPath)) {
        svcConnectToPort(&hbldrHandle, "hb:ldr");
        if (R_FAILED(HBLDR_SetTarget(hbldrHandle, toolPath.c_str() + 5))) {
            svcCloseHandle(hbldrHandle);
            return;
        }

        char lowTitleId[] = "00000000";
        char gameMtype[5] = "sdmc";
        FS_MediaType mtype;
        u64 titleId = CTRPluginFramework::Process::GetTitleID();
        FSUSER_GetMediaType(&mtype);
        snprintf(lowTitleId, sizeof(lowTitleId), "%08X", (u32)titleId);
        if (mtype == FS_MediaType::MEDIATYPE_GAME_CARD)
            memcpy(gameMtype, "card", 5);
        else if (mtype == FS_MediaType::MEDIATYPE_NAND)
            memcpy(gameMtype, "nand", 5);

        u32 bufferSize = sizeof(u32) + (toolPath.length() + 1) + sizeof(lowTitleId) + sizeof(gameMtype);
        char argvBuf[bufferSize];
        u32 argc = 3;
        memcpy(argvBuf, &argc, sizeof(u32));
        char* dstPtr = argvBuf + sizeof(u32);
        memcpy(dstPtr, toolPath.c_str(), toolPath.length() + 1);
        dstPtr += toolPath.length() + 1;
        memcpy(dstPtr, lowTitleId, sizeof(lowTitleId));
        dstPtr += sizeof(lowTitleId);
        memcpy(dstPtr, gameMtype, sizeof(gameMtype));
        
        HBLDR_SetArgv(hbldrHandle, argvBuf, sizeof(argvBuf));

        svcCloseHandle(hbldrHandle);
        mtype = FS_MediaType::MEDIATYPE_SD;
        svcGetSystemInfo((s64*)&titleId, 0x10000, 0x100);

        APT_PrepareToDoApplicationJump(0, titleId, mtype);
        APT_DoApplicationJump("", 0, nullptr);
        CTRPluginFramework::ProcessImpl::UnlockGameThreads(); // without this the game will hang up
        svcExitProcess();
        for (;;);
    }
}

Result HBLDR_SetTarget(Handle hbldrHandle, const char* path)
{
	u32 pathLen = strlen(path) + 1;
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(2, 0, 2); //0x20002
	cmdbuf[1] = IPC_Desc_StaticBuffer(pathLen, 0);
	cmdbuf[2] = (u32)path;

	Result rc = svcSendSyncRequest(hbldrHandle);
	if (R_SUCCEEDED(rc)) rc = cmdbuf[1];
	return rc;
}

Result HBLDR_SetArgv(Handle hbldrHandle, const void* buffer, u32 size)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(3, 0, 2); //0x30002
	cmdbuf[1] = IPC_Desc_StaticBuffer(size, 1);
	cmdbuf[2] = (u32)buffer;

	Result rc = svcSendSyncRequest(hbldrHandle);
	if (R_SUCCEEDED(rc)) rc = cmdbuf[1];
	return rc;
}