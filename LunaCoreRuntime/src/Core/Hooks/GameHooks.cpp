#include "Core/Hooks/GameHooks.hpp"

#include <vector>
#include <mutex>
#include <memory>
#include <cstring>

#include <CTRPluginFramework.hpp>

#include "string_hash.hpp"
#include "lua_common.h"
#include "Core/Debug.hpp"
#include "Core/CrashHandler.hpp"
#include "Core/Event.hpp"
#include "CoreGlobals.hpp"
#include "CoreInit.hpp"

#include "game/world/item/Item.hpp"
#include "game/entity/Entity.hpp"
#include "game/gstd/gstd_string.hpp"

#include "Helpers/LuaObject.hpp"

namespace CTRPF = CTRPluginFramework;

#define COND_ALWAYS (u32)(0b1110 << 28)
#define INS_B COND_ALWAYS | (0b101 << 25) | (0b0 << 24)
#define INS_BL COND_ALWAYS | (0b101 << 25) | (0b1 << 24)

#define IS_INS_B(ins) ((((ins) & 0xFF000000) & INS_B) == ((ins) & 0xFF000000))
#define IS_INS_BL(ins) ((((ins) & 0xFF000000) & INS_BL) == ((ins) & 0xFF000000))

#define DECODE_TARGET_INS_B(ins, pc_actual) (((ins) & 0xFFFFFF) << 2) + ((pc_actual) + 8)
#define DECODE_TARGET_INS_BL(ins, pc_actual) (DECODE_TARGET_INS_B(ins, pc_actual))

#ifndef LEGACY_HOOKS
extern "C" void lc_setCoreHookState() {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOK;
}

static __attribute__((naked)) void hookBody() {
    asm volatile (          // r0: callbackPtr, r4 hookCtxPtr
        "mov r5, r0\n"      // Copy callbackPtr to r5
        "bl lc_setCoreHookState\n"
        "mov r0, r4\n"      // Copy hookCtxPtr to r0 (arg 1)
        "blx r5\n"          // Branch to callback
        "add r0, r4, #60\n"
        "bx r0\n"           // Jump to returnIns and return normal flow
    );
}

__attribute__((naked)) void hookReturnOverwrite(CoreHookContext *ctx, u32 returnCallback) {
    asm volatile ( // r0 contains hookCtxPtr, r1 callback
        "add r2, r0, #16\n"
        "ldmia r2, {r4-r12, sp, lr}\n"  // Restore registers
        "mov r2, r1\n"
        "ldr r1, [r0, #0x4]\n"
        "ldr r0, [r0]\n"
        "bx r2\n"
    );
}

// Hooks an ARMv7 function, length must be at least 2 instructions to hook
// and instructions cannot be pc-relative dependent
void newHookFunction(u32 targetAddr, u32 callbackAddr) {
    CoreHookContext* hookCtx2 = Core::alloc<CoreHookContext>();

    hookCtx2->targetAddress = targetAddr;
    hookCtx2->selfHookPtr = (u32)hookCtx2;
    hookCtx2->preHookBody[0] = 0xE52D4004; // str r4, [sp, #-4]!            this saves r4 to stack and decrements sp
    hookCtx2->preHookBody[1] = 0xE51F4010; // ldr r4, [pc, #-16]            loads the hookCtx pointer to r4
    hookCtx2->preHookBody[2] = 0xE8847FFF; // stmia r4, {r0-r12, sp, lr}    stores all possible registers
    hookCtx2->preHookBody[3] = 0xE49D5004; // ldr r5, [sp], #4              this copies original r4 to r5 and restores sp
    hookCtx2->preHookBody[4] = 0xE5845010; // str r5, [r4, #16]             fix r4 value
    hookCtx2->preHookBody[5] = 0xE584D034; // str sp, [r4, #52]             fix sp value
    hookCtx2->preHookBody[6] = 0xE59F0004; // ldr r0, [pc, #4]
    hookCtx2->preHookBody[7] = 0xE59F1004; // ldr r1, [pc, #4]
    hookCtx2->preHookBody[8] = 0xE12FFF11; // bx r1
    hookCtx2->preHookData[0] = callbackAddr;
    hookCtx2->preHookData[1] = (u32)hookBody;

    hookCtx2->returnIns[0] = 0xE8947FFF; // ldmia r4, {r0-r12, sp, lr}
    int insIdx = 1;
    int dataOffset = 0;
    for (int i = 0; i < 2; i++) {
        u32 targetIns = *((u32*)targetAddr + i);
        if (IS_INS_B(targetIns)) {
            hookCtx2->returnIns[insIdx] = 0xE59FF000 + sizeof(hookCtx2->returnIns) - 8 + 4 * dataOffset - 4 * insIdx; // ldr, pc, [pc, dataOffset]
            hookCtx2->returnData[dataOffset++] = DECODE_TARGET_INS_B(targetIns, (u32)((u32*)targetAddr + i));
        } else if (IS_INS_BL(targetIns)) {
            hookCtx2->returnIns[insIdx++] = 0xE1A0E00F; // mov lr, pc
            hookCtx2->returnIns[insIdx] = 0xE59FF000 + sizeof(hookCtx2->returnIns) - 8 + 4 * dataOffset - 4 * insIdx; // ldr, pc, [pc, dataOffset]
            hookCtx2->returnData[dataOffset++] = DECODE_TARGET_INS_BL(targetIns, (u32)((u32*)targetAddr + i));
        } else {
            hookCtx2->returnIns[insIdx] = targetIns;
        }
        insIdx++;
    }
    hookCtx2->returnIns[insIdx++] = 0xE51FF004; // ldr pc, [pc, #-0x4]
    hookCtx2->returnIns[insIdx++] = targetAddr + 4 * 2;

    *(u32*)targetAddr = 0xE51FF004; // ldr pc, [pc, #-0x4]
    *((u32*)targetAddr + 1) = (u32)&hookCtx2->preHookBody[0];
}

static __attribute__((naked)) void RegisterItemOverwriteReturn() {
    asm volatile (
        "add sp, sp, #0x3c\n"
        "ldmia sp!, {r4-r11, pc}"
    );
}

static void RegisterItemsHook(CoreHookContext* ctx) {
    Game::Item* totemItem = reinterpret_cast<Game::Item*>(ctx->r[0]);
    totemItem->padding[6] = 1;
    Game::Item::mTotem = totemItem;

    {
        std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
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
        std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
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
        std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
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

    if (ctx->r[2] != 0) {
        std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
        LuaObjectUtils::NewObject(Lua_global, "GameSpawnCoords", reinterpret_cast<void*>(ctx->r[2])); // Pass the reference
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

    if (ctx->r[0] != 0) {
        std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
        Game::Entity* entity = reinterpret_cast<Game::Entity*>(ctx->r[0]);
        LuaObjectUtils::NewObject(Lua_global, "GameEntity", entity);
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
        std::lock_guard<Core::Mutex> lock(Lua_Global_Mut);
        LuaObjectUtils::NewObject(Lua_global, "RecipesTable", (void*)ctx->r[0]);
        Core::Event::TriggerEvent(Lua_global, "Game.Recipes.OnRegisterRecipes", 1);
    }
    
    GameState.LoadingRecipes.store(false);
    hookReturnOverwrite(ctx, (u32)RegisterRecipesOverwriteReturn);
}

#define MAX_CALLS 10
#define WINDOW_MS 1000
#define BAN_MS 10000

static u64 window_start = 0;
static u64 ban_until = 0;
static int call_count = 0;
static bool allow_call(void) {
    u64 now = osGetTime();
    if (now < ban_until)
        return false;
    if (now - window_start > WINDOW_MS) {
        window_start = now;
        call_count = 0;
    }
    call_count++;
    if (call_count >= MAX_CALLS) {
        ban_until = now + BAN_MS;
        return false;
    }
    return true;
}

extern "C" CoreHookContext* DumpRegisters(CoreHookContext* ctx) {
    Core::Debug::LogRawf("\tSP: %08X\n", ctx->sp);
    Core::Debug::LogRawf("\tLR: %08X\n", ctx->lr);
    Core::Debug::LogRawf("\tR0: %08X \tR1: %08X\n", ctx->r[0], ctx->r[1]);
    Core::Debug::LogRawf("\tR2: %08X \tR3: %08X\n", ctx->r[2], ctx->r[3]);
    Core::Debug::LogRawf("\tR4: %08X \tR5: %08X\n", ctx->r[4], ctx->r[5]);
    Core::Debug::LogRawf("\tR6: %08X \tR7: %08X\n", ctx->r[6], ctx->r[7]);
    Core::Debug::LogRawf("\tR8: %08X \tR9: %08X\n", ctx->r[8], ctx->r[9]);
    Core::Debug::LogRawf("\tR10: %08X\tR11: %08X\n", ctx->r[10], ctx->r[11]);
    Core::Debug::LogRawf("\tR12: %08X\n", ctx->r[12]);
    return ctx;
}

extern "C" void GameDebugLogfHandler(u32 ukn1, u32 ukn2, const char* author, u32 ukn3, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    if (allow_call() && strcmp(author, "BuildGeometry") != 0 && strcmp(author, "loadChunk") != 0) {
        char buffer[0x200] = {0};
        int res = vsnprintf(buffer, sizeof(buffer), fmt, ap);
        if (buffer[res-1] == '\n') // Remove the new line cause Log already adds it
            buffer[res-1] = '\0';
        //Core::Debug::Message(CTRPF::Utils::Format("%X, %X, %X", ukn1, ukn2, ukn3));
        if (ukn2 == 2)
            Core::Debug::LogInfo("[Game.Info] " + std::string(author) + ": " + buffer);
        else if (ukn2 == 8)
            Core::Debug::LogInfo("[Game.Warn] " + std::string(author) + ": " + buffer);
        else
            Core::Debug::LogInfo(' ' + std::string(author) + ": " + buffer);
    }

    va_end(ap);
    return;
}

static __attribute__((naked)) void GameDebugLogfHook(CoreHookContext* ctx) {
    asm volatile (
        #ifdef DUMPLOGREGISTERS
        "bl DumpRegisters\n"
        #endif
        "ldmia r0, {r0-r12, sp, lr}\n" // We will just overwrite the whole function
        "b GameDebugLogfHandler\n"
    );
}

void hookSomeFunctions() {
    Core::CrashHandler::core_state = Core::CrashHandler::CORE_HOOKING;
    newHookFunction(0x0056c2a0, (u32)RegisterItemsHook);
    newHookFunction(0x0056de70, (u32)RegisterItemsTexturesHook);
    newHookFunction(0x00578358, (u32)RegisterCreativeItemsHook);
    newHookFunction(0x001b4898, (u32)RegisterRecipes);
    newHookFunction(0x00114f50, (u32)GameDebugLogfHook);
    //hookFunction(0x004df688, (u32)EntitySpawnStartHook); disabled as there is a weird memory leak ? idk why
    //hookFunction(0x004df7e0, (u32)EntitySpawnFinishedHook);
}
#endif