#include "Game/Hooks/GameHooks.hpp"

#include <vector>
#include <memory>
#include <cstring>
#include <map>

#include <CTRPluginFramework.hpp>

#include "string_hash.hpp"
#include "lua_common.h"
#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Event.hpp"
#include "Game/world/item/Item.hpp"
#include "Game/world/entity/Entity.hpp"
#include "Core/Utils/GameState.hpp"
#include "CoreGlobals.hpp"
#include "CoreInit.hpp"

namespace CTRPF = CTRPluginFramework;

#define BASE_OFF 0x100000

extern GameState_s GameState;
static std::vector<std::unique_ptr<CoreHookContext>> hooks;

static __attribute((naked)) void hookBody() {
    asm volatile ( // r4 contains hookCtxPtr
        "mov r5, sp\n" // Save stack pointer
        "str r0, [r4, #0x0c]\n" // Store r0-r3 in hookCtxPtr->r0-r3
        "str r1, [r4, #0x10]\n"
        "str r2, [r4, #0x14]\n"
        "str r3, [r4, #0x18]\n"
        "str lr, [r4, #0x1c]\n"
        "str r5, [r4, #0x20]\n" // Store current sp
        "ldr r6, [r4, #0x4]\n" // Load callback function address
        "mov r0, r4\n" // Copy hookCtxPtr to r0 (arg 1)
        "blx r6\n"
        "mov sp, r5\n" // Restore stack pointer
        "add r6, r4, #0x24\n"
        "bx r6" // Jump to restoreIns and return normal flow
    );
}

void hookReturnOverwrite(CoreHookContext *ctx, u32 returnCallback) {
    asm volatile ( // r0 contains hookCtxPtr
        "ldr sp, [r0, #0x20]\n"
        "add sp, sp, #0x10\n"
        "ldmia sp!, {r4-r12, lr}\n"
        "mov r2, r1\n"
        "ldr r1, [r0, #0x10]\n"
        "ldr r0, [r0, #0x0c]\n"
        "bx r2\n"
    );
}

// Hooks an ARMv7 function, length must be at least 5 instructions to hook
// and instructions cannot be relative dependent
void hookFunction(u32 targetAddr, u32 callbackAddr) {
    const u32 asmData[] = {
        0xE92D5FFF, // stmdb sp!, {r0-r12, lr}
        0xE59F4004, // ldr r4, [pc, #0x4] pc is already 2 ins ahead
        0xE5945000, // ldr r5, [r4, #0x0]
        0xE12FFF15, // bx r5
        // hookCtxPtr
    };
    auto hookCtx = std::make_unique<CoreHookContext>();
    hookCtx->wrapCallbackAddress = (u32)hookBody;
    hookCtx->targetAddress = targetAddr;
    hookCtx->callbackAddress = callbackAddr;

    hookCtx->restoreIns = 0xE8BD5FFF; // ldmia sp!, {r0-r12, lr}
    hookCtx->jmpIns = 0xE51FF004; // ldr pc, [pc, #-0x4] pc is already 2 ins ahead
    hookCtx->returnAddress = targetAddr + 4 * 5;

    hookCtx->overwrittenIns[0] = *(u32*)targetAddr;
    hookCtx->overwrittenIns[1] = *((u32*)targetAddr + 1);
    hookCtx->overwrittenIns[2] = *((u32*)targetAddr + 2);
    hookCtx->overwrittenIns[3] = *((u32*)targetAddr + 3);
    hookCtx->overwrittenIns[4] = *((u32*)targetAddr + 4);

    CoreHookContext *hookCtxPtr = hookCtx.get();
    hooks.push_back(std::move(hookCtx));

    *(u32*)targetAddr = asmData[0];
    *((u32*)targetAddr + 1) = asmData[1];
    *((u32*)targetAddr + 2) = asmData[2];
    *((u32*)targetAddr + 3) = asmData[3];
    *((u32*)targetAddr + 4) = (u32)hookCtxPtr;
}

static __attribute((naked)) void RegisterItemOverwriteReturn() {
    asm volatile (
        "add sp, sp, #0x3c\n"
        "ldmia sp!, {r4-r11, pc}"
    );
}

static void RegisterItemsHook(CoreHookContext* ctx) {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    Game::Item* totemItem = reinterpret_cast<Game::Item*>(ctx->r0);
    totemItem->padding[6] = 1;
    Game::Item::mTotem = totemItem;

    Lua_Global_Mut.lock();

    GameState.LoadingItems.store(true);
    Core::Event::TriggerEvent(Lua_global, "OnGameItemsRegister");
    GameState.LoadingItems.store(false);

    Lua_Global_Mut.unlock();

    reinterpret_cast<void(*)()>(0x0056e450)();
    GameState.LoadingItems.store(false);
    hookReturnOverwrite(ctx, (u32)RegisterItemOverwriteReturn);
}

static __attribute((naked)) void RegisterItemsTexturesOverwriteReturn() {
    asm volatile (
        "add r0, sp, #0xa0\n"
        "ldr r4, =0x57c5a0\n"
        "blx r4\n"
        "add sp, sp, #0x124\n"
        "ldmia sp!, {r4-r11, pc}"
    );
}

static void RegisterItemsTexturesHook(CoreHookContext* ctx) {
    Core::CrashHandler::CoreState lastcState = Core::CrashHandler::core_state;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    GameState.SettingItemsTextures.store(true);

    Lua_Global_Mut.lock();
    Core::Event::TriggerEvent(Lua_global, "OnGameItemsRegisterTexture");
    Lua_Global_Mut.unlock();

    GameState.SettingItemsTextures.store(false);
    Core::CrashHandler::core_state = lastcState;
    hookReturnOverwrite(ctx, (u32)RegisterItemsTexturesOverwriteReturn);
}

static __attribute((naked)) void RegisterCreativeItemsOverwriteReturn() {
    asm volatile (
        "ldr r2, =0x56e108\n"
        "blx r2\n"
        "add r0, sp, #0x8\n"
        "ldr r2, =0x57836c\n"
        "bx r2"
    );
}

static void RegisterCreativeItemsHook(CoreHookContext* ctx) {
    Core::CrashHandler::CoreState lastcState = Core::CrashHandler::core_state;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
    GameState.LoadingCreativeItems.store(true);

    Lua_Global_Mut.lock();
    Core::Event::TriggerEvent(Lua_global, "OnGameCreativeItemsRegister");
    Lua_Global_Mut.unlock();

    GameState.LoadingCreativeItems.store(false);
    Core::CrashHandler::core_state = lastcState;
    hookReturnOverwrite(ctx, (u32)RegisterCreativeItemsOverwriteReturn);
}


//         005f6994 E2 84 0F A2     add        r0,r4,#0x288
//         005f6998 E5 95 10 00     ldr        r1,[r5,#0x0]
//         005f699c ED DF 0A 32     vldr.32    s1,[pc,#0xc8]=>FLOAT_005f6a6c                    = 0.5
//         005f69a0 E5 84 11 C0     str        r1,[r4,#0x1c0]
//         005f69a4 E5 95 10 04     ldr        r1,[r5,#0x4]


// static __attribute__((naked)) void EntitySpawnStartedOverwriteReturn() {
//     asm volatile (
//         "add r0, r4, #0x288\n"
//         "ldr r1, [r5, #0x0]\n"
//         "str r1, [r4, #0x1c0]\n"
//         "ldr r1, =0x005f6a6c\n"
//         "vldr.32 s1, [r1]\n"
//         "ldr r1, [r5, #0x4]\n"
//         "ldr pc, =0x005f69a8\n"
//     );
// }

// 0x004df6e0
// 0x004df6f4 < maybe better
// 0x005f6994 < best, $r1 contains spawn coords, changing them works

//         004df688 E5 9F A1 D0     ldr        r10,[DAT_004df860]                               = 00B0AC0Ch
//         004df68c E5 9A 00 10     ldr        r0,[r10,#0x10]=>DAT_00c0ac1c
//         004df690 E1 A0 20 00     cpy        r2,r0
//         004df694 E5 90 10 04     ldr        r1,[r0,#0x4]
//         004df698 E3 51 00 00     cmp        r1,#0x0


// 0x004df688 < r2 contains x,y,z

static __attribute((naked)) void EntitySpawnStartOverwriteReturn() {
    asm volatile (
        "ldr r10, =0x00b0ac0c\n"
        "ldr r0, [r10, #0x10]\n"
        "cpy r2, r0\n"
        "ldr r1, [r0, #0x4]\n"
        "cmp r1, #0x0\n"
        "ldr pc, =0x004df69c\n"
    );
}

static void EntitySpawnStartHook(CoreHookContext *ctx) {
    Core::CrashHandler::CoreState lastcState = Core::CrashHandler::core_state;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;

    float spawnX = 0.0f;
    float spawnY = 0.0f;
    float spawnZ = 0.0f;

    // Core::Debug::LogMessage(CTRPF::Utils::Format("Entity spawn started, r1: 0x%08X", ctx->r1), true);
    if (ctx->r2 != 0x0) {
        spawnX = *reinterpret_cast<float*>(ctx->r2);
        spawnY = *reinterpret_cast<float*>(ctx->r2 + 4);
        spawnZ = *reinterpret_cast<float*>(ctx->r2 + 8);
        // Core::Debug::LogMessage(CTRPF::Utils::Format("Entity spawn started at (%f, %f, %f)", spawnX, spawnY, spawnZ), true);
        
        // Set y to 20
        // *reinterpret_cast<float*>(ctx->r2 + 4) = 20.0f;
    }
    
    std::map<std::string, std::any> eventData = {
        {"x", spawnX},
        {"y", spawnY},
        {"z", spawnZ}
    };

    Lua_Global_Mut.lock();
    std::map<std::string, std::any> eventReturn = Core::Event::TriggerEvent(Lua_global, "OnGameEntitySpawnStart", eventData);
    Lua_Global_Mut.unlock();

    Core::Debug::LogMessage(CTRPF::Utils::Format("Return object size: %zu", eventReturn.size()), true);

    if (eventReturn.find("x") != eventReturn.end()) {
        Core::Debug::LogMessage(CTRPF::Utils::Format("Entity spawn start event returned x: %f", std::any_cast<float>(eventReturn["x"])), true);
        *reinterpret_cast<float*>(ctx->r2) = std::any_cast<float>(eventReturn["x"]);
    }
    if (eventReturn.find("y") != eventReturn.end()) {
        Core::Debug::LogMessage(CTRPF::Utils::Format("Entity spawn start event returned y: %f", std::any_cast<float>(eventReturn["y"])), true);
        *reinterpret_cast<float*>(ctx->r2 + 4) = std::any_cast<float>(eventReturn["y"]);
    }
    if (eventReturn.find("z") != eventReturn.end()) {
        Core::Debug::LogMessage(CTRPF::Utils::Format("Entity spawn start event returned z: %f", std::any_cast<float>(eventReturn["z"])), true);
        *reinterpret_cast<float*>(ctx->r2 + 8) = std::any_cast<float>(eventReturn["z"]);
    }

    Core::CrashHandler::core_state = lastcState;
    hookReturnOverwrite(ctx, (u32)EntitySpawnStartOverwriteReturn);
}

static __attribute((naked)) void EntitySpawnFinishedOverwriteReturn() {
    asm volatile (
        "ldr r0, [sp, #0x190]\n"
        "ldr r2, =0x004df7f8\n"
        "cmp r0, #0x0\n"
        "blxeq r2\n"
        "ldr r1, [r0, #0x0]\n"
        "ldr r1, [r1, #0x24]\n"
        "ldr pc, =0x004df7f8\n"
    );
}

// x/x $sp+0x190 -> entity
// then e.g. if the result is 0x34715d40:
// x/u 0x34715ac8+0x278
// which will get the unsigned int ID of the entity

static void EntitySpawnFinishedHook(CoreHookContext *ctx) {
    // Core::Debug::LogError("Trying to hook entity spawn");
    Core::CrashHandler::CoreState lastcState = Core::CrashHandler::core_state;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;

    if (ctx->r0 == 0x0) {
        // Core::Debug::LogError("Entity spawn hook called with r0 == 0, skipping");
    } else {
        Game::Entity* entity = reinterpret_cast<Game::Entity*>(ctx->r0);
        // Core::Debug::LogMessage(CTRPF::Utils::Format("Entity spawn detected, r0: 0x%08X", ctx->r0), true);
        // Core::Debug::LogMessage(CTRPF::Utils::Format("id %d, x %f, y %f, z %f", entity->entityTypeId, entity->x, entity->y, entity->z), true);
    }

    Lua_Global_Mut.lock();
    Core::Event::TriggerEvent(Lua_global, "OnGameEntitySpawn");
    Lua_Global_Mut.unlock();

    Core::CrashHandler::core_state = lastcState;

    hookReturnOverwrite(ctx, (u32)EntitySpawnFinishedOverwriteReturn);
    // Core::Debug::LogError("Successfully hooked entity spawn");
}

static __attribute((naked)) void LoadGameOverwriteReturn() {
    asm volatile (
        "stmdb sp!, {r4-r11, lr}\n"
        "vpush {d8, d9}\n"
        "cpy r4, r0\n"
        "sub sp, sp, #0xfc\n"
        "ldr r2, =0x22de4c\n"
        "ldr r1, [r2, #0x0]\n"
        "ldr r2, =0x22daa0\n"
        "bx r2\n"
    );
}

static void LoadGame(CoreHookContext *ctx) {
    Core::CrashHandler::CoreState lastcState = Core::CrashHandler::core_state;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;

    Core::InitCore();

    Lua_Global_Mut.lock();
    Core::Event::TriggerEvent(Lua_global, "OnGameLoad");
    Lua_Global_Mut.unlock();

    Core::CrashHandler::core_state = lastcState;
    hookReturnOverwrite(ctx, (u32)LoadGameOverwriteReturn);
}

void hookSomeFunctions() {
    Core::CrashHandler::CoreState lastcState = Core::CrashHandler::core_state;
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOKING;
    hookFunction(0x0056c2a0, (u32)RegisterItemsHook);
    hookFunction(0x0056de70, (u32)RegisterItemsTexturesHook);
    hookFunction(0x00578358, (u32)RegisterCreativeItemsHook);
    //hookFunction(0x005f65a8, (u32)EntitySpawnHook);
    // hookFunction(0x005f6994, (u32)EntitySpawnStartedHook);
    hookFunction(0x004df688, (u32)EntitySpawnStartHook);
    hookFunction(0x004df7e0, (u32)EntitySpawnFinishedHook);
    //hookFunction(0x0022da8c, (u32)LoadGame);
    Core::CrashHandler::core_state = lastcState;
}