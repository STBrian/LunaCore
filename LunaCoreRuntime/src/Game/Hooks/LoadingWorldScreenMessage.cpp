#include "Game/Hooks/LoadingWorldScreenMessage.hpp"

#include <CTRPluginFramework.hpp>

#include "Core/Utils/GameState.hpp"
#include "Core/CrashHandler.hpp"
#include "lua_common.h"

#include "Core/Event.hpp"
#include "CoreGlobals.hpp"

namespace CTRPF = CTRPluginFramework;

extern GameState_s GameState;
bool eventJoinTriggered = false;

typedef int (*MBdetectStateFunc)(int *);

void LoadingWorldScreenMessageCallback(int *ptr1, int *ptr2) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    GameState.MainMenuLoaded.store(false);

    void (*LoadingWorldScreenMessageOriginal)(int*, int*) = (void(*)(int*,int*))(0x712ef0);
    MBdetectStateFunc *detectState = (MBdetectStateFunc*)(*ptr2 + 0x18);
    int state = detectState[0](ptr2);
    if (state == 0x10) {
        GameState.WorldLoaded.store(true);
        if (!eventJoinTriggered) {
            Core::CrashHandler::game_state = Core::CrashHandler::GAME_WORLD;
            CustomLockGuard Lock(Lua_Global_Mut);
            Core::Event::TriggerEvent(Lua_global, "OnPlayerJoinWorld");
            eventJoinTriggered = true;
        }
    }

    LoadingWorldScreenMessageOriginal(ptr1, ptr2);
    return;
}

void SetLoadingWorldScreenMessageCallback() {
    CTRPF::Process::Write32(0x9c5df4, (u32)LoadingWorldScreenMessageCallback);
}

void LeaveLevelPromptCallback(int *ptr1, int param2, int param3, u32 param4) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    void (*LeaveLevelPromptOriginal)(int*,int,int,u32) = (void(*)(int*,int,int,u32))(0x321d78);

    GameState.WorldLoaded.store(false);
    if (eventJoinTriggered) {
        Core::CrashHandler::game_state = Core::CrashHandler::GAME_MENU;
        CustomLockGuard Lock(Lua_Global_Mut);
        Core::Event::TriggerEvent(Lua_global, "OnPlayerLeaveWorld");
        eventJoinTriggered = false;
    }

    LeaveLevelPromptOriginal(ptr1, param2, param3, param4);
    return;
}

void SetLeaveLevelPromptCallback() {
    CTRPF::Process::Write32(0x9b3a70, (u32)LeaveLevelPromptCallback);
}