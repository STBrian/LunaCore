#include "Minecraft/Hooks/GameHooks.hpp"

#include <vector>
#include <mutex>
#include <memory>
#include <cstring>

#include <CTRPluginFramework.hpp>

#include "string_hash.hpp"
#include "lua_common.h"
#include "lua_object.hpp"
#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Event.hpp"
#include "CoreGlobals.hpp"
#include "CoreInit.hpp"

#include "Minecraft/world/item/Item.hpp"
#include "Minecraft/world/entity/Entity.hpp"

namespace CTRPF = CTRPluginFramework;

#define BASE_OFF 0x100000

static std::vector<std::unique_ptr<CoreHookContext>> hooks;

extern "C" void lc_setCoreHookState() {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
}

static __attribute__((naked)) void hookBody() {
    asm volatile ( // r4 contains hookCtxPtr
        "mov r5, sp\n" // Save stack pointer
        "str r0, [r4, #0x0c]\n" // Store r0-r3 in hookCtxPtr->r0-r3
        "str r1, [r4, #0x10]\n"
        "str r2, [r4, #0x14]\n"
        "str r3, [r4, #0x18]\n"
        "str lr, [r4, #0x1c]\n"
        "str r5, [r4, #0x20]\n" // Store current sp
        "ldr r6, [r4, #0x4]\n" // Load callback function address
        "bl lc_setCoreHookState\n"
        "mov r0, r4\n" // Copy hookCtxPtr to r0 (arg 1)
        "blx r6\n"
        "mov sp, r5\n" // Restore stack pointer
        "add r6, r4, #0x24\n"
        "bx r6" // Jump to restoreIns and return normal flow
    );
}

__attribute__((naked)) void hookReturnOverwrite(CoreHookContext *ctx, u32 returnCallback) {
    asm volatile ( // r0 contains hookCtxPtr
        "ldr sp, [r0, #0x20]\n"
        "add sp, sp, #0x10\n"
        "ldmia sp!, {r4-r12, lr}\n"  // Restore stack pointer
        "mov r2, r1\n"
        "ldr r1, [r0, #0x10]\n"
        "ldr r0, [r0, #0x0c]\n"
        "bx r2\n"
    );
}

// Hooks an ARMv7 function, length must be at least 5 instructions to hook
// and instructions cannot be pc-relative dependent
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

static __attribute__((naked)) void RegisterItemOverwriteReturn() {
    asm volatile (
        "add sp, sp, #0x3c\n"
        "ldmia sp!, {r4-r11, pc}"
    );
}

static void RegisterItemsHook(CoreHookContext* ctx) {
    Game::Item* totemItem = reinterpret_cast<Game::Item*>(ctx->r0);
    totemItem->padding[6] = 1;
    Game::Item::mTotem = totemItem;

    {
        std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
        GameState.LoadingItems.store(true);
        Core::Event::TriggerEvent(Lua_global, "Game.Items.OnRegisterItems");
        GameState.LoadingItems.store(false);
    }

    reinterpret_cast<void(*)()>(0x0056e450)();
    GameState.LoadingItems.store(false);
    hookReturnOverwrite(ctx, (u32)RegisterItemOverwriteReturn);
}

static __attribute__((naked)) void RegisterItemsTexturesOverwriteReturn() {
    asm volatile (
        "add r0, sp, #0xa0\n"
        "ldr r4, =0x57c5a0\n"
        "blx r4\n"
        "add sp, sp, #0x124\n"
        "ldmia sp!, {r4-r11, pc}"
    );
}

static void RegisterItemsTexturesHook(CoreHookContext* ctx) {
    GameState.SettingItemsTextures.store(true);

    {
        std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
        Core::Event::TriggerEvent(Lua_global, "Game.Items.OnRegisterItemsTextures");
    }

    GameState.SettingItemsTextures.store(false);
    hookReturnOverwrite(ctx, (u32)RegisterItemsTexturesOverwriteReturn);
}

static __attribute__((naked)) void RegisterCreativeItemsOverwriteReturn() {
    asm volatile (
        "ldr r2, =0x56e108\n"
        "blx r2\n"
        "add r0, sp, #0x8\n"
        "ldr r2, =0x57836c\n"
        "bx r2"
    );
}

static void RegisterCreativeItemsHook(CoreHookContext* ctx) {
    GameState.LoadingCreativeItems.store(true);

    {
        std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
        Core::Event::TriggerEvent(Lua_global, "Game.Items.OnRegisterCreativeItems");
    }

    GameState.LoadingCreativeItems.store(false);
    hookReturnOverwrite(ctx, (u32)RegisterCreativeItemsOverwriteReturn);
}

static __attribute__((naked)) void EntitySpawnStartOverwriteReturn() {
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

    if (ctx->r2 != 0) {
        std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
        LuaObject::NewObject(Lua_global, "GameSpawnCoords", reinterpret_cast<void*>(ctx->r2)); // Pass the reference
        Core::Event::TriggerEvent(Lua_global, "Core.Event.OnGameEntitySpawnStart", 1);
    }

    hookReturnOverwrite(ctx, (u32)EntitySpawnStartOverwriteReturn);
}

static __attribute__((naked)) void EntitySpawnFinishedOverwriteReturn() {
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

static void EntitySpawnFinishedHook(CoreHookContext *ctx) {
    // Core::Debug::LogError("Trying to hook entity spawn");

    if (ctx->r0 != 0) {
        std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
        Game::Entity* entity = reinterpret_cast<Game::Entity*>(ctx->r0);
        LuaObject::NewObject(Lua_global, "GameEntity", entity);
        Core::Event::TriggerEvent(Lua_global, "Core.Event.OnGameEntitySpawn", 1);
    }

    hookReturnOverwrite(ctx, (u32)EntitySpawnFinishedOverwriteReturn);
}

static __attribute__((naked)) void RegisterRecipesOverwriteReturn() {
    asm volatile (
        "stmdb sp!,{r0,r4,r5,r6,r7,r8,r9,r10,r11,lr}\n"
        "ldr r1, =0x1b4c54\n"
        "vpush {d8}\n"
        "sub sp, sp, #0x230\n"
        "add r2, sp, #0x20c\n"
        "ldr pc, =0x001b48ac"
    );
}

static void RegisterRecipes(CoreHookContext* ctx) {
    GameState.LoadingRecipes.store(true);

    {
        std::lock_guard<CustomMutex> lock(Lua_Global_Mut);
        LuaObject::NewObject(Lua_global, "RecipesTable", (void*)ctx->r0);
        Core::Event::TriggerEvent(Lua_global, "Game.Recipes.OnRegisterRecipes", 1);
    }
    
    GameState.LoadingRecipes.store(false);
    hookReturnOverwrite(ctx, (u32)RegisterRecipesOverwriteReturn);
}

void hookSomeFunctions() {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOKING;
    hookFunction(0x0056c2a0, (u32)RegisterItemsHook);
    hookFunction(0x0056de70, (u32)RegisterItemsTexturesHook);
    hookFunction(0x00578358, (u32)RegisterCreativeItemsHook);
    hookFunction(0x001b4898, (u32)RegisterRecipes);
    //hookFunction(0x004df688, (u32)EntitySpawnStartHook); disabled as there is a weird memory leak ? idk why
    //hookFunction(0x004df7e0, (u32)EntitySpawnFinishedHook);
}